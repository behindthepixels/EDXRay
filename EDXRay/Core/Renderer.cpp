#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Primitive.h"
#include "TriangleMesh.h"
#include "Light.h"
#include "Ray.h"
#include "../Integrators/DirectLighting.h"
#include "../Integrators/PathTracing.h"
#include "../Integrators/BidirectionalPathTracing.h"
#include "../Sampler/RandomSampler.h"
#include "../Sampler/SobolSampler.h"
#include "../Tracer/BVH.h"
#include "../Tracer/BVHBuildTask.h"
#include "Film.h"
#include "DifferentialGeom.h"
#include "Graphics/Color.h"
#include "RenderTask.h"
#include "Config.h"

#include "Graphics/ObjMesh.h"

namespace EDX
{
	namespace RayTracer
	{
		Renderer::Renderer()
		{
			// Initialize scene
			mpCamera.Reset(new Camera());
			mpScene.Reset(new Scene);
			QueuedThreadPool::Instance()->Create(GetNumberOfCores());
		}

		Renderer::~Renderer()
		{
			QueuedThreadPool::DeleteInstance();
		}

		void Renderer::InitComponent()
		{
			mpCamera->Init(mJobDesc.CameraParams.Pos,
				mJobDesc.CameraParams.Target,
				mJobDesc.CameraParams.Up,
				mJobDesc.ImageWidth,
				mJobDesc.ImageHeight,
				mJobDesc.CameraParams.CalcFieldOfView(),
				mJobDesc.CameraParams.NearClip,
				mJobDesc.CameraParams.FarClip,
				mJobDesc.CameraParams.CalcCircleOfConfusionRadius(),
				mJobDesc.CameraParams.FocusPlaneDist,
				mJobDesc.CameraParams.Vignette);

			Filter* pFilter;
			switch (mJobDesc.FilterType)
			{
			case EFilterType::Box:
				pFilter = new BoxFilter;
				break;
			case EFilterType::Gaussian:
				pFilter = new GaussianFilter;
				break;
			case EFilterType::MitchellNetravali:
				pFilter = new MitchellNetravaliFilter;
				break;
			default:
				pFilter = new GaussianFilter;
				break;
			}

			mpFilm.Reset(new Film);
			mpFilm->Init(mJobDesc.ImageWidth, mJobDesc.ImageHeight, pFilter);

			switch (mJobDesc.SamplerType)
			{
			case ESamplerType::Random:
				mpSampler.Reset(new RandomSampler);
				break;
			case ESamplerType::Sobol:
				mpSampler.Reset(new SobolSampler(mJobDesc.ImageWidth, mJobDesc.ImageHeight));
				break;
			case ESamplerType::Metropolis:
				mpSampler.Reset(new RandomSampler);
				break;
			}

			switch (mJobDesc.IntegratorType)
			{
			case EIntegratorType::DirectLighting:
				mpIntegrator.Reset(new DirectLightingIntegrator(mJobDesc.MaxPathLength));
				break;
			case EIntegratorType::PathTracing:
				mpIntegrator.Reset(new PathTracingIntegrator(mJobDesc.MaxPathLength));
				break;
			case EIntegratorType::BidirectionalPathTracing:
				mpIntegrator.Reset(new BidirPathTracingIntegrator(mJobDesc.MaxPathLength, mpCamera.Get(), mpFilm.Get()));
				break;
			case EIntegratorType::MultiplexedMLT:
				mpIntegrator.Reset(new BidirPathTracingIntegrator(mJobDesc.MaxPathLength, mpCamera.Get(), mpFilm.Get()));
				break;
			case EIntegratorType::StochasticPPM:
				mpIntegrator.Reset(new BidirPathTracingIntegrator(mJobDesc.MaxPathLength, mpCamera.Get(), mpFilm.Get()));
				break;
			}

			//BakeSamples();
			//mpScene->InitAccelerator();

			mTaskSync.Init(mJobDesc.ImageWidth, mJobDesc.ImageHeight);
			mTaskSync.SetAbort(false);
		}

		void Renderer::Resize(int width, int height)
		{
			mJobDesc.ImageWidth = width;
			mJobDesc.ImageHeight = height;

			if (mpCamera)
				mpCamera->Resize(width, height);
			if (mpFilm)
				mpFilm->Resize(width, height);

			mTaskSync.Init(width, height);
		}

		void Renderer::RenderFrame(Sampler* pSampler, RandomGen& random, MemoryPool& memory)
		{
			RenderTile* pTask;
			auto pSampleBuf = &pSampler->GetSampleBuffer();
			while (mTaskSync.GetNextTask(pTask))
			{
				for (auto y = pTask->minY; y < pTask->maxY; y++)
				{
					for (auto x = pTask->minX; x < pTask->maxX; x++)
					{
						if (mTaskSync.Aborted())
							return;

						pSampler->StartPixel(x, y);
						pSampler->GenerateSamples(x, y, pSampleBuf, random);
						pSampleBuf->imageX += x;
						pSampleBuf->imageY += y;

						RayDifferential ray;
						Color L = Color::BLACK;
						if (mpCamera->GenRayDifferential(*pSampleBuf, &ray))
						{
							L = mpIntegrator->Li(ray, mpScene.Get(), pSampler, random, memory);
						}

						mpFilm->AddSample(pSampleBuf->imageX, pSampleBuf->imageY, L);
						memory.FreeAll();
					}
				}
			}
		}

		void Renderer::RenderImage(int threadId, RandomGen& random, MemoryPool& memory)
		{
			UniquePtr<Sampler> pTileSampler(mpSampler->Clone());

			for (auto i = 0; i < mJobDesc.SamplesPerPixel; i++)
			{
				// Sync barrier before render
				mTaskSync.SyncThreadsPreRender(threadId);

				RenderFrame(pTileSampler.Get(), random, memory);

				// Sync barrier after render
				mTaskSync.SyncThreadsPostRender(threadId);

				pTileSampler->AdvanceSampleIndex();

				// One thread only
				if (threadId == 0)
				{
					mpFilm->IncreSampleCount();
					mpFilm->ScaleToPixel();
					mTaskSync.ResetTasks();

					mFrameTime = mTimer.GetElapsedTime();
				}

				if (mTaskSync.Aborted())
					break;
			}
		}

		void Renderer::BakeSamples()
		{
			mpIntegrator->RequestSamples(mpScene.Get(), &mpSampler->GetSampleBuffer());
		}

		void Renderer::QueueRenderTasks()
		{
			mpFilm->Clear();
			mTaskSync.SetAbort(false);

			for (auto i = 0; i < QueuedThreadPool::Instance()->GetNumThreads(); i++)
			{
				mTasks.Add(MakeUnique<QueuedRenderTask>(this, i));
				QueuedThreadPool::Instance()->AddQueuedWork(mTasks[i].Get());
			}
		}

		void Renderer::StopRenderTasks()
		{
			mTaskSync.SetAbort(true);
			QueuedThreadPool::Instance()->JoinAllThreads();
			mTasks.Clear();
		}

		void Renderer::SetJobDesc(const RenderJobDesc& jobDesc)
		{
			mJobDesc = jobDesc;

			mpCamera->Init(mJobDesc.CameraParams.Pos,
				mJobDesc.CameraParams.Target,
				mJobDesc.CameraParams.Up,
				mJobDesc.ImageWidth,
				mJobDesc.ImageHeight,
				mJobDesc.CameraParams.CalcFieldOfView(),
				mJobDesc.CameraParams.NearClip,
				mJobDesc.CameraParams.FarClip,
				mJobDesc.CameraParams.CalcCircleOfConfusionRadius(),
				mJobDesc.CameraParams.FocusPlaneDist);
		}

		Film* Renderer::GetFilm()
		{
			return mpFilm.Get();
		}
	}
}