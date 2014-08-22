#pragma once

#include "../Core/Sampler.h"

namespace EDX
{
	namespace RayTracer
	{
		class RandomSampler : public Sampler
		{
		public:
			void GenerateSamples(SampleBuffer* pSamples, RandomGen& random);
		};
	}
}