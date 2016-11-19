#pragma once

#include "../Core/Sampler.h"
#include "Core/Random.h"

namespace EDX
{
	namespace RayTracer
	{
		class RandomSampler : public Sampler
		{
		private:
			RandomGen mRandom;

		public:
			RandomSampler() = default;

			RandomSampler(const int seed)
				: mRandom(seed)
			{
			}

			void GenerateSamples(
				const int pixelX,
				const int pixelY,
				CameraSample* pSamples,
				RandomGen& random) override;
			void AdvanceSampleIndex() override;

			void StartPixel(const int pixelX, const int pixelY) override;
			float Get1D() override;
			Vector2 Get2D() override;
			Sample GetSample() override;

			UniquePtr<Sampler> Clone(const int seed) const override;
		};
	}
}