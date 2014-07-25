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

		void Renderer::RenderImage()
		{
			for (auto i = 0; i < mJobDesc.SamplesPerPixel; i++)
			{
				RenderFrame();
				mpFilm->IncreSampleCount();
				mpFilm->ScaleToPixel();
			}
		}

		const Color* Renderer::GetFrameBuffer() const
		{
			return mpFilm->GetPixelBuffer();
		}
	}
}