
#include "SobolSampler.h"
#include "SobolMatrices.h"

#include "Math/Vec2.h"

namespace EDX
{
	namespace RayTracer
	{
		void SobolSampler::GenerateSamples(
			const int pixelX,
			const int pixelY,
			SampleBuffer* pSamples,
			RandomGen& random)
		{
			assert(pSamples);

			pSamples->imageX = SobolSample(mSobolIndex, mDimension++) * mResolution - pixelX;
			pSamples->imageY = SobolSample(mSobolIndex, mDimension++) * mResolution - pixelY;
			pSamples->lensU = SobolSample(mSobolIndex, mDimension++);
			pSamples->lensV = SobolSample(mSobolIndex, mDimension++);
			pSamples->time = SobolSample(mSobolIndex, mDimension++);

			for (auto i = 0; i < pSamples->count1D; i++)
			{
				pSamples->p1D[i] = SobolSample(mSobolIndex, mDimension++);
			}

			for (auto i = 0; i < pSamples->count2D; i++)
			{
				pSamples->p2D[i].u = SobolSample(mSobolIndex, mDimension++);
				pSamples->p2D[i].v = SobolSample(mSobolIndex, mDimension++);
			}
		}

		void SobolSampler::AdvanceSampleIndex()
		{
			mSampleIndex++;
		}

		void SobolSampler::StartPixel(const int pixelX, const int pixelY)
		{
			mSobolIndex = EnumerateSampleIndex(pixelX, pixelY);
			mDimension = 0;
		}

		float SobolSampler::Get1D()
		{
			return SobolSample(mSobolIndex, mDimension++);
		}

		Vector2 SobolSampler::Get2D()
		{
			int dim = mDimension;
			Vector2 ret = Vector2(SobolSample(mSobolIndex, dim),
				SobolSample(mSobolIndex, dim + 1));
			mDimension += 2;

			return ret;
		}

		Sample SobolSampler::GetSample()
		{
			int dim = mDimension;

			Sample ret;
			ret.u = SobolSample(mSobolIndex, dim);
			ret.v = SobolSample(mSobolIndex, dim + 1);
			ret.w = SobolSample(mSobolIndex, dim + 2);

			mDimension += 3;

			return ret;
		}

		Sampler* SobolSampler::Clone() const
		{
			auto ret = new SobolSampler(mResolution, mLogTwoResolution, mScramble);
			ret->GetSampleBuffer().count1D = mSample.count1D;
			ret->GetSampleBuffer().count2D = mSample.count2D;

			return ret;
		}

		uint64 SobolSampler::EnumerateSampleIndex(const uint pixelX, const uint pixelY) const
		{
			if (mLogTwoResolution == 0)
				return 0;

			const uint m2 = mLogTwoResolution << 1;
			uint64 sampleIndex = mSampleIndex;
			uint64 index = uint64(sampleIndex) << m2;

			uint64 delta = 0;
			for (int c = 0; sampleIndex; sampleIndex >>= 1, c++)
			{
				if (sampleIndex & 1)  // Add flipped column m + c + 1.
					delta ^= VdCSobolMatrices[mLogTwoResolution - 1][c];
			}

			// Flipped b
			uint64 b = (((uint64)pixelX << mLogTwoResolution) | pixelY) ^ delta;

			for (int c = 0; b; b >>= 1, c++)
				if (b & 1)  // Add column 2 * m - c.
					index ^= VdCSobolMatricesInv[mLogTwoResolution - 1][c];

			return index;
		}

		float SobolSampler::SobolSample(const int64 index, const int dimension) const
		{
			if (dimension < NumSobolDimensions)
			{
				uint32 v = mScramble;
				int64 _index = index;
				for (int i = dimension * SobolMatrixSize + SobolMatrixSize - 1; _index != 0; _index >>= 1, i--)
				{
					if (_index & 1)
						v ^= SobolMatrices32[i];
				}

				return v * 2.3283064365386963e-10f; /* 1 / 2^32 */
			}
			else
				return mRandom.Float();
		}
	}
}