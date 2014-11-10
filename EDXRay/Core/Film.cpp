#include "Film.h"
#include "Math/EDXMath.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		const float Film::INV_GAMMA = 0.454545f;

		void Film::Init(int width, int height, Filter* pFilter)
		{
			Resize(width, height);
			mpFilter = pFilter;
		}

		void Film::Resize(int width, int height)
		{
			mWidth = width;
			mHeight = height;

			mPixelBuffer.Free();
			mPixelBuffer.Init(Vector2i(width, height));
			mAccumulateBuffer.Free();
			mAccumulateBuffer.Init(Vector2i(width, height));
			mSampleCount = 0;
		}

		void Film::Release()
		{
			mPixelBuffer.Free();
			mAccumulateBuffer.Free();
		}

		void Film::Clear()
		{
			mSampleCount = 0;
			mPixelBuffer.Clear();
			mAccumulateBuffer.Clear();
		}

		void Film::AddSample(float x, float y, const Color& sample)
		{
			x -= 0.5f;
			y -= 0.5f;
			int minX = Math::CeilToInt(x - mpFilter->GetRadius());
			int maxX = Math::FloorToInt(x + mpFilter->GetRadius());
			int minY = Math::CeilToInt(y - mpFilter->GetRadius());
			int maxY = Math::FloorToInt(y + mpFilter->GetRadius());
			minX = Math::Max(0, minX);
			maxX = Math::Min(maxX, mWidth - 1);
			minY = Math::Max(0, minY);
			maxY = Math::Min(maxY, mHeight - 1);

			for (auto i = minY; i <= maxY; i++)
			{
				for (auto j = minX; j <= maxX; j++)
				{
					int rowAdd = mHeight - 1 - i;
					int colAdd = j;
					Pixel& pixel = mAccumulateBuffer[Vector2i(colAdd, rowAdd)];

					float weight = mpFilter->Eval(j - x, i - y);
					pixel.weight += weight;
					pixel.color += weight * sample;
				}
			}
		}

		void Film::ScaleToPixel()
		{
			float fScale = 1.0f / float(mSampleCount);

			for (int y = 0; y < mHeight; y++)
			{
				for (int x = 0; x < mWidth; x++)
				{
					const Pixel& pixel = mAccumulateBuffer[Vector2i(x, y)];
					mPixelBuffer[y * mWidth + x] = Math::Pow(pixel.color / pixel.weight, INV_GAMMA);
				}
			}
		}

		// Ray Histogram Fusion film implementation
		void FilmRHF::Init(int width, int height, Filter* pFilter)
		{
			Film::Init(width, height, pFilter);
			mInputBuffer.Init(Vector2i(width, height));
			mDenoisedPixelBuffer.Init(Vector2i(width, height));
			mSampleHistogram.Init(width, height);
			mRHFSampleCount.Init(width, height);

			mMaxDist = 1.0f;
			mHalfPatchSize = 1;
			mHalfWindowSize = 6;
			mTempBuffer.Init(Vector2i(2 * mHalfPatchSize + 1, 2 * mHalfPatchSize + 1));
			mRunRHF = false;
		}

		void FilmRHF::Resize(int width, int height)
		{
			Film::Resize(width, height);

			mInputBuffer.Init(Vector2i(width, height));
			mDenoisedPixelBuffer.Init(Vector2i(width, height));
			mSampleHistogram.Init(width, height);
			mRHFSampleCount.Init(width, height);
		}

		void FilmRHF::Clear()
		{
			Film::Clear();

			mInputBuffer.Clear();
			mDenoisedPixelBuffer.Clear();
			mSampleHistogram.Clear();
			mRHFSampleCount.Clear();
		}

		const float FilmRHF::Histogram::MAX_VAL = 7.5f;

		void FilmRHF::AddSample(float x, float y, const Color& sample)
		{
			x -= 0.5f;
			y -= 0.5f;
			int minX = Math::CeilToInt(x - mpFilter->GetRadius());
			int maxX = Math::FloorToInt(x + mpFilter->GetRadius());
			int minY = Math::CeilToInt(y - mpFilter->GetRadius());
			int maxY = Math::FloorToInt(y + mpFilter->GetRadius());
			minX = Math::Max(0, minX);
			maxX = Math::Min(maxX, mWidth - 1);
			minY = Math::Max(0, minY);
			maxY = Math::Min(maxY, mHeight - 1);

			for (auto i = minY; i <= maxY; i++)
			{
				for (auto j = minX; j <= maxX; j++)
				{
					int rowAdd = mHeight - 1 - i;
					int colAdd = j;
					Pixel& pixel = mAccumulateBuffer[Vector2i(colAdd, rowAdd)];

					float weight = mpFilter->Eval(j - x, i - y);
					Color weightedSample = weight * sample;

					pixel.weight += weight;
					pixel.color += weightedSample;

					Color normalizedSample = Math::Pow(weightedSample, INV_GAMMA) / Histogram::MAX_VAL;

					const float S = 2.0f;
					normalizedSample.r = Math::Min(S, normalizedSample.r);
					normalizedSample.g = Math::Min(S, normalizedSample.g);
					normalizedSample.b = Math::Min(S, normalizedSample.b);

					Vector3 bin = float(Histogram::NUM_BINS - 2) * Vector3(normalizedSample.r, normalizedSample.g, normalizedSample.b);
					Vector3i binLow = Math::FloorToInt(bin);

					for (auto d = 0; d < 3; d++)
					{
						float weightH = 0.0f, weightL = 0.0f;
						if (binLow[d] < Histogram::NUM_BINS - 2)
						{
							weightH = bin[d] - binLow[d];
						}
						else
						{
							weightH = (normalizedSample[d] - 1) / (S - 1.0f);
						}

						weightL = 1.0f - weightH;
						mSampleHistogram.histogramWeights[binLow[d]][Vector2i(colAdd, rowAdd)][d] += weightL;
						mSampleHistogram.histogramWeights[binLow[d] + 1][Vector2i(colAdd, rowAdd)][d] += weightH;
						mSampleHistogram.totalWeight[Vector2i(colAdd, rowAdd)][d] += weightL + weightH;
					}
				}
			}
		}

		void FilmRHF::ScaleToPixel()
		{
			float fScale = 1.0f / float(mSampleCount);

			for (int y = 0; y < mHeight; y++)
			{
				for (int x = 0; x < mWidth; x++)
				{
					const Pixel& pixel = mAccumulateBuffer[Vector2i(x, y)];
					if (mRunRHF)
						mInputBuffer[y * mWidth + x] = Math::Pow(pixel.color / pixel.weight, INV_GAMMA);
					else
						mPixelBuffer[y * mWidth + x] = Math::Pow(pixel.color / pixel.weight, INV_GAMMA);
				}
			}

			if (!mRunRHF)
				return;

			mDenoisedPixelBuffer.Clear();
			mRHFSampleCount.Clear();
			for (int y = 0; y < mHeight; y++)
			{
				for (int x = 0; x < mWidth; x++)
				{
					const auto halfPatchSize = Math::Min(mHalfPatchSize, Math::Min(Math::Min(x, y), Math::Min(mWidth - 1 - x, mHeight - 1 - y)));
					const auto minX = Math::Max(x - mHalfWindowSize, halfPatchSize);
					const auto minY = Math::Max(y - mHalfWindowSize, halfPatchSize);
					const auto maxX = Math::Min(x + mHalfWindowSize, mWidth - 1 - halfPatchSize);
					const auto maxY = Math::Min(y + mHalfWindowSize, mHeight - 1 - halfPatchSize);

					auto num = 0;
					mTempBuffer.Clear();
					for (auto i = minY; i <= maxY; i++)
					{
						for (auto j = minX; j <= maxX; j++)
						{
							float dist = ChiSquareDistance(Vector2i(x, y), Vector2i(j, i), halfPatchSize);
							if (dist < mMaxDist)
							{
								for (auto h = -halfPatchSize; h <= halfPatchSize; h++)
									for (auto w = -halfPatchSize; w <= halfPatchSize; w++)
										mTempBuffer[Vector2i(w + halfPatchSize, h + halfPatchSize)] += mInputBuffer[Vector2i(j + w, i + h)];

								num++;
							}
						}
					}

					if (num > 0)
					{
						for (auto h = -halfPatchSize; h <= halfPatchSize; h++)
						{
							for (auto w = -halfPatchSize; w <= halfPatchSize; w++)
							{
								mDenoisedPixelBuffer[Vector2i(x + w, y + h)] += mTempBuffer[Vector2i(w + halfPatchSize, h + halfPatchSize)] / float(num);
								mRHFSampleCount[Vector2i(x + w, y + h)]++;
							}
						}
					}
				}
			}

			for (int y = 0; y < mHeight; y++)
				for (int x = 0; x < mWidth; x++)
					mPixelBuffer[Vector2i(x, y)] = mDenoisedPixelBuffer[Vector2i(x, y)] / float(mRHFSampleCount[Vector2i(x, y)]);
		}

		float FilmRHF::ChiSquareDistance(const Vector2i& coord0, const Vector2i& coord1, const int halfPatchSize)
		{
			auto PixelWiseDist = [this](const Vector2i& c0, const Vector2i& c1) -> float
			{
				int normFactor = 0;
				float ret = 0.0f;

				for (auto binIdx = 0; binIdx < Histogram::NUM_BINS; binIdx++)
				{
					for (auto d = 0; d < 3; d++)
					{
						const float histo0 = mSampleHistogram.histogramWeights[binIdx][c0][d];
						const float histo1 = mSampleHistogram.histogramWeights[binIdx][c1][d];
						const float sum = histo0 + histo1;
						if (sum > 1.0f)
						{
							const float weight0 = mSampleHistogram.totalWeight[c0][d];
							const float weight1 = mSampleHistogram.totalWeight[c1][d];
							const float diff = weight1 * histo0 - weight0 * histo1;

							ret += diff * diff / ((weight0 * weight1) * sum);
							normFactor++;
						}
					}
				}

				return ret / (normFactor + 1e-4f);
			};

			float patchWiseDist = 0.0f;
			for (auto i = -halfPatchSize; i <= halfPatchSize; i++)
			{
				for (auto j = -halfPatchSize; j <= halfPatchSize; j++)
				{
					Vector2i c0 = coord0 + Vector2i(i, j);
					Vector2i c1 = coord1 + Vector2i(i, j);

					patchWiseDist += PixelWiseDist(c0, c1);
				}
			}

			return patchWiseDist;
		}
	}
}