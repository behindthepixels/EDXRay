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
			PathTracing,
			BidirectionalPathTracing,
			MultiplexedMLT,
			StochasticPPM
		};

		enum class ESamplerType
		{
			Random,
			Sobol,
			Metropolis
		};

		enum class EFilterType
		{
			Box,
			Gaussian,
			MitchellNetravali
		};

		struct RenderJobDesc
		{
			CameraParameters	CameraParams;
			EIntegratorType		IntegratorType;
			ESamplerType		SamplerType;
			EFilterType			FilterType;
			bool				AdaptiveSample;
			bool				UseRHF;
			uint				ImageWidth, ImageHeight;
			uint				SamplesPerPixel;
			uint				MaxPathLength;
			Array<String>		ModelPaths;

			RenderJobDesc()
			{
				ImageWidth = 1280;
				ImageHeight = 800;

				CameraParams.Pos = Vector3::ZERO;
				CameraParams.Target = -Vector3::UNIT_Z;
				CameraParams.Up = Vector3::UNIT_Y;
				CameraParams.NearClip = 1.0f;
				CameraParams.FarClip = 1000.0f;
				CameraParams.FocusPlaneDist = float(Math::EDX_INFINITY);
				CameraParams.FocalLengthMilliMeters = 50.0f;
				CameraParams.FStop = 22.0f;

				IntegratorType = EIntegratorType::BidirectionalPathTracing;
				SamplerType = ESamplerType::Random;
				FilterType = EFilterType::Gaussian;
				AdaptiveSample = false;
				UseRHF = false;
				SamplesPerPixel = 4096;
				MaxPathLength = 8;
			}
		};
	}
}