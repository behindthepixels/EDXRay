
#include "RandomSampler.h"

#include "Math/Vec2.h"
#include "Core/Random.h"

namespace EDX
{
	namespace RayTracer
	{
		void RandomSampler::GenerateSamples(
			const int pixelX,
			const int pixelY,
			CameraSample* pSamples,
			RandomGen& random)
		{
			Assert(pSamples);

			pSamples->imageX = random.Float();
			pSamples->imageY = random.Float();
			pSamples->lensU = random.Float();
			pSamples->lensV = random.Float();
			pSamples->time = random.Float();
		}

		void RandomSampler::AdvanceSampleIndex()
		{
		}

		void RandomSampler::StartPixel(const int pixelX, const int pixelY)
		{
		}

		float RandomSampler::Get1D()
		{
			return mRandom.Float();
		}

		Vector2 RandomSampler::Get2D()
		{
			return Vector2(mRandom.Float(), mRandom.Float());
		}

		Sample RandomSampler::GetSample()
		{
			return Sample(mRandom);
		}

		UniquePtr<Sampler> RandomSampler::Clone(const int seed) const
		{
			return MakeUnique<RandomSampler>(seed);
		}
	}
}