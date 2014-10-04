#pragma once

#include "EDXPrerequisites.h"
#include "Math/Vector.h"
#include "Camera.h"

namespace EDX
{
	namespace RayTracer
	{
		enum class EIntegratorType
		{
			DirectLighting,
			AmbientOcclusion,
			PathTracing,
			BidirectionalPathTracing
		};

		enum class ESamplerType
		{
			Random,
			Sobol,
			Metropolis
		};

		struct RenderJobDesc
		{
			CameraParameters	CameraParams;
			EIntegratorType		IntegratorType;
			ESamplerType		SamplerType;
			uint				ImageWidth, ImageHeight;
			uint				SamplesPerPixel;
			vector<string>		ModelPaths;

			RenderJobDesc()
			{
				ImageWidth = 1280;
				ImageHeight = 800;

				CameraParams.Pos = Vector3::ZERO;
				CameraParams.Target = Vector3::UNIT_Z;
				CameraParams.Up = Vector3::UNIT_Y;
				CameraParams.FieldOfView = 45.0f;
				CameraParams.NearClip = 1.0f;
				CameraParams.FarClip = 1000.0f;
				CameraParams.FocusPlaneDist = 0.0f;
				CameraParams.LensRadius = 0.0f;

				IntegratorType = EIntegratorType::DirectLighting;
				SamplerType = ESamplerType::Random;
				SamplesPerPixel = 64;
			}
		};
	}
}