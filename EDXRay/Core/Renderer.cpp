#include "Renderer.h"
#include "Camera.h"
#include "Config.h"

namespace EDX
{
	namespace RayTracer
	{

		void Renderer::Initialize(const RenderJobParams& params)
		{
			if (!mpCamera)
			{
				mpCamera = new Camera();
			}
			mpCamera->Init(params.CameraParams.Pos,
				params.CameraParams.Target,
				params.CameraParams.Up,
				params.ImageWidth,
				params.ImageHeight,
				params.CameraParams.FieldOfView,
				params.CameraParams.NearClip,
				params.CameraParams.FarClip);


		}

		void Renderer::RenderFrame()
		{

		}

		void Renderer::RenderImage()
		{

		}
	}
}