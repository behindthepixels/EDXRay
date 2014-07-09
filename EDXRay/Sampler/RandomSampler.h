#pragma once

#include "../Core/Sampler.h"

namespace EDX
{
	namespace RayTracer
	{
		class RandomSampler : public Sampler
		{
		public:
			void GenerateSamples(Sample* pSamples, RandomGen& random);
		};
	}
}