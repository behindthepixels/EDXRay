
#include "SobolSampler.h"
#include "SobolMatrices.h"

#include "Math/Vec2.h"
#include "RNG/Random.h"

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

			int sobolIndex = EnumerateSampleIndex(pixelX, pixelY);

			int dimension = 0;
			pSamples->imageX = SobolSample(sobolIndex, dimension++) * mResolution - pixelX;
			pSamples->imageY = SobolSample(sobolIndex, dimension++) * mResolution - pixelY;
			pSamples->lensU = SobolSample(sobolIndex, dimension++);
			pSamples->lensV = SobolSample(sobolIndex, dimension++);
			pSamples->time = SobolSample(sobolIndex, dimension++);

			for (auto i = 0; i < pSamples->count1D; i++)
			{
				pSamples->p1D[i] = SobolSample(sobolIndex, dimension++);
			}

			for (auto i = 0; i < pSamples->count2D; i++)
			{
				pSamples->p2D[i].u = SobolSample(sobolIndex, dimension++);
				pSamples->p2D[i].v = SobolSample(sobolIndex, dimension++);
			}
		}

		void SobolSampler::AdvanceSampleIndex()
		{
			mSampleIndex++;
		}

		int SobolSampler::EnumerateSampleIndex(const uint pixelX, const uint pixelY) const
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

		float SobolSampler::SobolSample(const int index, const int dimension) const
		{
			assert(dimension < NumSobolDimensions);
			uint32 v = mScramble;
			int _index = index;
			for (int i = dimension * SobolMatrixSize + SobolMatrixSize - 1; _index != 0; _index >>= 1, i--)
			{
				if (_index & 1)
					v ^= SobolMatrices32[i];
			}

			return v * 2.3283064365386963e-10f; /* 1 / 2^32 */
		}
	}
}