#pragma once

#include "../Core/Sampler.h"
#include "RNG/Random.h"

namespace EDX
{
	namespace RayTracer
	{
		class SobolSampler : public Sampler
		{
		private:
			int mResolution;
			int mLogTwoResolution;
			uint64 mSampleIndex;
			uint64 mSobolIndex;
			uint mDimension;
			uint64 mScramble;
			mutable RandomGen mRandom;

		public:
			SobolSampler(const int resX, const int resY)
				: mSampleIndex(0)
				, mSobolIndex(0)
				, mDimension(0)
				, mScramble(0)
			{
				mResolution =
					Math::RoundUpPowOfTwo(
						Math::Max(resX, resY)
					);

				mLogTwoResolution = Math::FloorLog2(mResolution);
				assert(1 << mLogTwoResolution == mResolution);
			}

			SobolSampler(const int resolution, const int log2Res, const uint64 scramble)
				: mSampleIndex(0)
				, mSobolIndex(0)
				, mDimension(0)
				, mResolution(resolution)
				, mLogTwoResolution(log2Res)
				, mScramble(scramble)
			{
			}

			void GenerateSamples(
				const int pixelX,
				const int pixelY,
				SampleBuffer* pSamples,
				RandomGen& random) override;
			void AdvanceSampleIndex() override;

			void StartPixel(const int pixelX, const int pixelY) override;
			float Get1D() override;
			Vector2 Get2D() override;
			Sample GetSample() override;

			Sampler* Clone() const override;

		private:
			uint64 EnumerateSampleIndex(const uint pixelX, const uint pixelY) const;
			float SobolSample(const int64 index, const int dimension) const;
		};
	}
}