#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "../Integrators/DirectLighting.h"
#include "../Integrators/PathTracing.h"
#include "../Integrators/BidirectionalPathTracing.h"
#include "Sampler.h"
#include "../Sampler/RandomSampler.h"
#include "../Sampler/SobolSampler.h"
#include "Film.h"
#include "DifferentialGeom.h"
#include "Graphics/Color.h"
#include "RenderTask.h"
#include "Config.h"

namespace EDX
{
	namespace RayTracer
	{
		Renderer::Renderer()
		{
			// Initialize scene
			mpCamera = new Camera();
			mpScene = new Scene;
			ThreadScheduler::Instance()->InitAndLaunchThreads();
		}

		Renderer::~Renderer()
		{
			ThreadScheduler::DeleteInstance();
		}

		void Renderer::InitComponent()
		{
			//mpCamera->Init(mJobDesc.CameraParams.Pos,
			//	mJobDesc.CameraParams.Target,
			//	mJobDesc.CameraParams.Up,
			//	mJobDesc.ImageWidth,
			//	mJobDesc.ImageHeight,
			//	mJobDesc.CameraParams.mLensSettings.CalcFieldOfView(),
			//	mJobDesc.CameraParams.NearClip,
			//	mJobDesc.CameraParams.FarClip,
			//	mJobDesc.CameraParams.mLensSettings.CalcLensRadius(),
			//	mJobDesc.CameraParams.FocusPlaneDist);

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
			}

			mpFilm = mJobDesc.UseRHF ? new FilmRHF : new Film;
			mpFilm->Init(mJobDesc.ImageWidth, mJobDesc.ImageHeight, new GaussianFilter);

			switch (mJobDesc.SamplerType)
			{
			case ESamplerType::Random:
				mpSampler = new RandomSampler;
				break;
			case ESamplerType::Sobol:
				mpSampler = new SobolSampler(mJobDesc.ImageWidth, mJobDesc.ImageHeight);
				break;
			case ESamplerType::Metropolis:
				mpSampler = new RandomSampler;
				break;
			}

			switch (mJobDesc.IntegratorType)
			{
			case EIntegratorType::DirectLighting:
				mpIntegrator = new DirectLightingIntegrator(mJobDesc.MaxPathLength);
				break;
			case EIntegratorType::PathTracing:
				mpIntegrator = new PathTracingIntegrator(mJobDesc.MaxPathLength);
				break;
			case EIntegratorType::BidirectionalPathTracing:
				mpIntegrator = new BidirPathTracingIntegrator(mJobDesc.MaxPathLength, mpCamera.Ptr(), mpFilm.Ptr());
				break;
			case EIntegratorType::MultiplexedMLT:
				mpIntegrator = new BidirPathTracingIntegrator(mJobDesc.MaxPathLength, mpCamera.Ptr(), mpFilm.Ptr());
				break;
			case EIntegratorType::StochasticPPM:
				mpIntegrator = new BidirPathTracingIntegrator(mJobDesc.MaxPathLength, mpCamera.Ptr(), mpFilm.Ptr());
				break;
			}

			BakeSamples();
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

		void Renderer::RenderFrame(SampleBuffer* pSampleBuf, RandomGen& random, MemoryArena& memory)
		{
			RenderTile* pTask;
			while (mTaskSync.GetNextTask(pTask))
			{
				for (auto y = pTask->minY; y < pTask->maxY; y++)
				{
					for (auto x = pTask->minX; x < pTask->maxX; x++)
					{
						if (mTaskSync.Aborted())
							return;

						mpSampler->GenerateSamples(x, y, pSampleBuf, random);
						pSampleBuf->imageX += x;
						pSampleBuf->imageY += y;

						RayDifferential ray;
						mpCamera->GenRayDifferential(*pSampleBuf, &ray);

						Color L = mpIntegrator->Li(ray, mpScene.Ptr(), pSampleBuf, random, memory);

						mpFilm->AddSample(pSampleBuf->imageX, pSampleBuf->imageY, L);
						memory.FreeAll();
					}
				}
			}
		}

		void Renderer::RenderImage(int threadId, RandomGen& random, MemoryArena& memory)
		{
			SampleBuffer* pSampleBuf = mpSampleBuf->Duplicate(1);

			for (auto i = 0; i < mJobDesc.SamplesPerPixel; i++)
			{
				// Sync barrier before render
				mTaskSync.SyncThreadsPreRender(threadId);

				RenderFrame(pSampleBuf, random, memory);

				// Sync barrier after render
				mTaskSync.SyncThreadsPostRender(threadId);

				// One thread only
				if (threadId == 0)
				{
					mpSampler->AdvanceSampleIndex();
					mpFilm->IncreSampleCount();
					mpFilm->ScaleToPixel();
					mTaskSync.ResetTasks();
				}

				if (mTaskSync.Aborted())
					break;
			}

			SafeDeleteArray(pSampleBuf);
		}

		void Renderer::BakeSamples()
		{
			mpSampleBuf = new SampleBuffer;
			mpIntegrator->RequestSamples(mpScene.Ptr(), mpSampleBuf.Ptr());
		}

		void Renderer::QueueRenderTasks()
		{
			mpFilm->Clear();
			mTaskSync.SetAbort(false);

			for (auto i = 0; i < ThreadScheduler::Instance()->GetThreadCount(); i++)
			{
				mTasks.push_back(new RenderTask(this));
				ThreadScheduler::Instance()->AddTasks(Task((Task::TaskFunc)&RenderTask::_Render, mTasks[i]));
			}
		}

		void Renderer::StopRenderTasks()
		{
			mTaskSync.SetAbort(true);
			ThreadScheduler::Instance()->JoinAllTasks();
		}

		void Renderer::SetJobDesc(const RenderJobDesc& jobDesc)
		{
			mJobDesc = jobDesc;

			mpCamera->Init(mJobDesc.CameraParams.Pos,
				mJobDesc.CameraParams.Target,
				mJobDesc.CameraParams.Up,
				mJobDesc.ImageWidth,
				mJobDesc.ImageHeight,
				mJobDesc.CameraParams.mLensSettings.CalcFieldOfView(),
				mJobDesc.CameraParams.NearClip,
				mJobDesc.CameraParams.FarClip,
				mJobDesc.CameraParams.mLensSettings.CalcLensRadius(),
				mJobDesc.CameraParams.FocusPlaneDist);
		}

		Film* Renderer::GetFilm()
		{
			return mpFilm.Ptr();
		}
	}
}