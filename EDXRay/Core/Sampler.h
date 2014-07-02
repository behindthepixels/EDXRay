#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		struct CameraSample
		{
			float imageX, imageY;
			float lensU, lensV;
			float time;
		};

		struct Sample : public CameraSample
		{
			float* pSamples1D;
			Vector2* pSamples2D;
		};

		class SobolSampler
		{
		public:
			void GenerateSamples(Sample* pSamples);
		};
	}
}