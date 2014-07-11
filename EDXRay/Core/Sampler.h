#pragma once

#include "EDXPrerequisites.h"
#include "Math/Vec2.h"

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
			float* p1D;
			Vector2* p2D;

			int count1D, count2D;

			Sample()
				: count1D(0)
				, count2D(0)
			{
			}

			inline int Request1DArray(int count) { count1D += count; return count1D - 1; }
			inline int Request2DArray(int count) { count2D += count; return count2D - 1; }
			void Validate();
		};

		class Sampler
		{
		public:
			virtual void GenerateSamples(Sample* pSamples, RandomGen& random) = 0;
		};
	}
}