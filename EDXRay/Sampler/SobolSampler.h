#pragma once

#include "../Core/Sampler.h"

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
			uint64 mScramble;

		public:
			SobolSampler(const int resX, const int resY)
				: mSampleIndex(0)
				, mScramble(0)
			{
				mResolution =
					Math::RoundUpPowTwo(
						Math::Max(resX, resY)
					);

				mLogTwoResolution = Math::FloorLog2(mResolution);
				assert(1 << mLogTwoResolution == mResolution);
			}

			void GenerateSamples(
				const int pixelX,
				const int pixelY,
				SampleBuffer* pSamples,
				RandomGen& random) override;
			void AdvanceSampleIndex() override;

		private:
			uint64 EnumerateSampleIndex(const uint pixelX, const uint pixelY) const;
			float SobolSample(const int64 index, const int dimension) const;
		};
	}
}