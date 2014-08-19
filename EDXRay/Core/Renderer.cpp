#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "../Integrators/DirectLighting.h"
#include "Sampler.h"
#include "Film.h"
#include "DifferentialGeom.h"
#include "Graphics/Color.h"
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
			mpIntegrator = new DirectLightingIntegrator;

			mpFilm = new Film;
			mpFilm->Init(desc.ImageWidth, desc.ImageHeight);

			mTilescheduler.Init(desc.ImageWidth, desc.ImageHeight);

			mThreads.resize(DetectCPUCount());
			uint id = 0;
			for (auto& it : mThreads)
			{
				it.Init(this, id++);
				it.mpScheduler = &mThreadScheduler;
			}
		}

		Renderer::~Renderer()
		{
		}

		void Renderer::RenderFrame(RandomGen& random, MemoryArena& memory)
		{
			RenderTile* pTask;
			while (mTilescheduler.GetNextTask(pTask))
			{
				for (auto y = pTask->minY; y < pTask->maxY; y++)
				{
					for (auto x = pTask->minX; x < pTask->maxX; x++)
					{
						Sample sample;
						sample.imageX = x;
						sample.imageY = y;
						RayDifferential ray;
						mpCamera->GenRayDifferential(sample, &ray);

						Color L = mpIntegrator->Li(ray, mpScene.Ptr(), &sample, random, memory);

						mpFilm->AddSample(x, y, L);
					}
				}
			}
		}

		void Renderer::RenderImage(int threadId, RandomGen& random, MemoryArena& memory)
		{
			for (auto i = 0; i < mJobDesc.SamplesPerPixel; i++)
			{
				// Sync barrier before render
				mTilescheduler.SyncThreadsPreRender(threadId);

				RenderFrame(random, memory);

				// Sync barrier after render
				mTilescheduler.SyncThreadsPostRender(threadId);

				// One thread only
				if (threadId == 0)
				{
					mpFilm->IncreSampleCount();
					mpFilm->ScaleToPixel();
					mTilescheduler.ResetTasks();
				}
			}
		}

		void Renderer::LaunchRenderThreads()
		{
			for (auto& it : mThreads)
			{
				it.Launch();
			}

			mThreadScheduler.AddTasks(Task((Task::TaskFunc)&RenderTask::Run));
		}

		const Color* Renderer::GetFrameBuffer() const
		{
			return mpFilm->GetPixelBuffer();
		}
	}
}