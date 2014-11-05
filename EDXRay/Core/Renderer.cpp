#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "../Integrators/DirectLighting.h"
#include "../Integrators/PathTracing.h"
#include "Sampler.h"
#include "../Sampler/RandomSampler.h"
#include "Film.h"
#include "DifferentialGeom.h"
#include "Graphics/Color.h"
#include "RenderTask.h"
#include "Config.h"

namespace EDX
{
	namespace RayTracer
	{
		void Renderer::Initialize(const RenderJobDesc& desc)
		{
			mJobDesc = desc;

			// Initalize camera
			if (!mpCamera)
			{
				mpCamera = new Camera();
			}
			mpCamera->Init(desc.CameraParams.Pos,
				desc.CameraParams.Target,
				desc.CameraParams.Up,
				desc.ImageWidth,
				desc.ImageHeight,
				desc.CameraParams.FieldOfView,
				desc.CameraParams.NearClip,
				desc.CameraParams.FarClip,
				desc.CameraParams.LensRadius,
				desc.CameraParams.FocusPlaneDist);

			// Initialize scene
			mpScene = new Scene;
			mpIntegrator = new PathTracingIntegrator(8);

			mpFilm = new Film;
			mpFilm->Init(desc.ImageWidth, desc.ImageHeight, new GaussianFilter);

			mpSampler = new RandomSampler;

			mTaskSync.Init(desc.ImageWidth, desc.ImageHeight);

			mTaskSync.SetAbort(false);
			ThreadScheduler::Instance()->InitAndLaunchThreads();
		}

		Renderer::~Renderer()
		{
			ThreadScheduler::DeleteInstance();
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

						mpSampler->GenerateSamples(pSampleBuf, random);
						pSampleBuf->imageX += x;
						pSampleBuf->imageY += y;

						RayDifferential ray;
						mpCamera->GenRayDifferential(*pSampleBuf, &ray);

						Color L = mpIntegrator->Li(ray, mpScene.Ptr(), pSampleBuf, random, memory);

						mpFilm->AddSample(pSampleBuf->imageX, pSampleBuf->imageY, L);
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
					mpFilm->IncreSampleCount();
					mpFilm->ScaleToPixel();
					mTaskSync.ResetTasks();
					memory.FreeAll();
				}

				if (mTaskSync.Aborted())
					return;
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

		void Renderer::SetCameraParams(const CameraParameters& params)
		{
			mJobDesc.CameraParams = params;

			mpCamera->Init(mJobDesc.CameraParams.Pos,
				mJobDesc.CameraParams.Target,
				mJobDesc.CameraParams.Up,
				mJobDesc.ImageWidth,
				mJobDesc.ImageHeight,
				mJobDesc.CameraParams.FieldOfView,
				mJobDesc.CameraParams.NearClip,
				mJobDesc.CameraParams.FarClip,
				mJobDesc.CameraParams.LensRadius,
				mJobDesc.CameraParams.FocusPlaneDist);
		}

		const Color* Renderer::GetFrameBuffer() const
		{
			return mpFilm->GetPixelBuffer();
		}
	}
}