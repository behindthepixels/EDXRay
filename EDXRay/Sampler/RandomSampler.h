#pragma once

#include "../Core/Sampler.h"

namespace EDX
{
	namespace RayTracer
	{
		class RandomSampler : public Sampler
		{
		public:
			void GenerateSamples(
				const int pixelX,
				const int pixelY,
				SampleBuffer* pSamples,
				RandomGen& random) override;
			void AdvanceSampleIndex() override;
		};
	}
}