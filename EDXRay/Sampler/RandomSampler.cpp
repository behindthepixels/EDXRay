
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
			SampleBuffer* pSamples,
			RandomGen& random)
		{
			Assert(pSamples);

			pSamples->imageX = random.Float();
			pSamples->imageY = random.Float();
			pSamples->lensU = random.Float();
			pSamples->lensV = random.Float();
			pSamples->time = random.Float();

			for (auto i = 0; i < pSamples->count1D; i++)
			{
				pSamples->p1D[i] = random.Float();
			}

			for (auto i = 0; i < pSamples->count2D; i++)
			{
				pSamples->p2D[i].u = random.Float();
				pSamples->p2D[i].v = random.Float();
			}
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

		Sampler* RandomSampler::Clone() const
		{
			auto ret = new RandomSampler();
			ret->GetSampleBuffer().count1D = mSample.count1D;
			ret->GetSampleBuffer().count2D = mSample.count2D;

			return ret;
		}
	}
}