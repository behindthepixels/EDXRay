#include "Renderer.h"
#include "Camera.h"
#include "Sampler.h"
#include "Film.h"
#include "Graphics/Color.h"
#include "Config.h"

namespace EDX
{
	namespace RayTracer
	{
		void Renderer::Initialize(const RenderJobDesc& desc)
		{
			mJobDesc = desc;

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

			mpFilm = new Film;
			mpFilm->Init(desc.ImageWidth, desc.ImageHeight);

			mTaskScheduler.Init(desc.ImageWidth, desc.ImageHeight);

			mThreads.resize(DetectCPUCount());
			uint id = 0;
			for (auto& it : mThreads)
			{
				it.Init(this, id++);
			}
		}

		Renderer::~Renderer()
		{
		}

		void Renderer::RenderFrame()
		{
			RenderTask* pTask;
			while (mTaskScheduler.GetNextTask(pTask))
			{
				for (auto y = pTask->minY; y < pTask->maxY; y++)
				{
					for (auto x = pTask->minX; x < pTask->maxX; x++)
					{
						Sample sample;
						sample.imageX = x;
						sample.imageY = y;
						Ray ray;
						mpCamera->GenerateRay(sample, &ray);

						mpFilm->AddSample(x, y, Color(ray.mDir.x, ray.mDir.y, ray.mDir.z));
					}
				}
			}
		}

		void Renderer::RenderImage(int threadId)
		{
			for (auto i = 0; i < mJobDesc.SamplesPerPixel; i++)
			{
				// Sync barrier before render
				mTaskScheduler.SyncThreadsPreRender(threadId);

				RenderFrame();

				// Sync barrier afrer render
				mTaskScheduler.SyncThreadsPostRender(threadId);

				// One thread only
				if (threadId == 0)
				{
					mpFilm->IncreSampleCount();
					mpFilm->ScaleToPixel();
					mTaskScheduler.ResetTasks();
				}
			}
		}

		void Renderer::LaunchRenderThreads()
		{
			for (auto& it : mThreads)
			{
				it.Launch();
			}
		}

		const Color* Renderer::GetFrameBuffer() const
		{
			return mpFilm->GetPixelBuffer();
		}
	}
}