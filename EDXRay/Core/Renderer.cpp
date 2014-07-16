#include "Renderer.h"
#include "Camera.h"
#include "Sampler.h"
#include "Film.h"
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


		}

		void Renderer::RenderFrame()
		{
			for (auto y = 0; y < mJobDesc.ImageHeight; y++)
			{
				for (auto x = 0; x < mJobDesc.ImageWidth; x++)
				{
					Sample* 
					Ray ray;
					mpCamera->GenerateRay()
				}
			}
		}

		void Renderer::RenderImage()
		{

		}
	}
}