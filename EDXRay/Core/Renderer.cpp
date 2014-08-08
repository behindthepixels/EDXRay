#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
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
						RayDifferential ray;
						mpCamera->GenRayDifferential(sample, &ray);

						DifferentialGeom diffGeom;
						Color L;
						if (mpScene->Intersect(ray, &diffGeom))
						{
							mpScene->PostIntersect(ray, &diffGeom);
							L = Color(diffGeom.mTexcoord.u, diffGeom.mTexcoord.v, 0.0f);
						}
						else
						{
							L = Color::BLACK;
						}

						mpFilm->AddSample(x, y, L);
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

				// Sync barrier after render
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