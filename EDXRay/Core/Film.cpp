#include "Film.h"
#include "Math/EDXMath.h"
#include "Graphics/Color.h"

#include <ppl.h>
using namespace concurrency;

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
			parallel_for(0, mHeight, [this](int y)
			{
				for (int x = 0; x < mWidth; x++)
				{
					Pixel pixel = mAccumulateBuffer[Vector2i(x, y)];
					pixel.color.r = Math::Max(0.0f, pixel.color.r);
					pixel.color.g = Math::Max(0.0f, pixel.color.g);
					pixel.color.b = Math::Max(0.0f, pixel.color.b);
					mPixelBuffer[y * mWidth + x] = Math::Pow(pixel.color / pixel.weight, INV_GAMMA);
				}
			});
		}

		// Ray Histogram Fusion film implementation
		void FilmRHF::Init(int width, int height, Filter* pFilter)
		{
			Film::Init(width, height, pFilter);
			mInputBuffer.Init(Vector2i(width, height));
			mDenoisedPixelBuffer.Init(Vector2i(width, height));
			mSampleHistogram.Init(width, height);
			mRHFSampleCount.Init(Vector2i(width, height));

			mMaxDist = 0.7f;
			mHalfPatchSize = 1;
			mHalfWindowSize = 6;
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

					weightedSample.r = Math::Max(0.0f, weightedSample.r);
					weightedSample.g = Math::Max(0.0f, weightedSample.g);
					weightedSample.b = Math::Max(0.0f, weightedSample.b);
					Color normalizedSample = Math::Pow(weightedSample, INV_GAMMA) / Histogram::MAX_VAL;

					const float S = 2.0f;
					normalizedSample.r = Math::Min(S, normalizedSample.r);
					normalizedSample.g = Math::Min(S, normalizedSample.g);
					normalizedSample.b = Math::Min(S, normalizedSample.b);

					Vector3 bin = float(Histogram::NUM_BINS - 2) * Vector3(normalizedSample.r, normalizedSample.g, normalizedSample.b);
					Vector3i binLow = Math::FloorToInt(bin);

					for (auto c = 0; c < 3; c++)
					{
						float weightH = 0.0f, weightL = 0.0f;
						if (binLow[c] < Histogram::NUM_BINS - 2)
						{
							weightH = bin[c] - binLow[c];
							weightL = 1.0f - weightH;
							mSampleHistogram.histogramWeights[binLow[c]][Vector2i(colAdd, rowAdd)][c] += weightL;
							mSampleHistogram.histogramWeights[binLow[c] + 1][Vector2i(colAdd, rowAdd)][c] += weightH;
						}
						else
						{
							weightH = (normalizedSample[c] - 1) / (S - 1.0f);
							weightL = 1.0f - weightH;
							mSampleHistogram.histogramWeights[Histogram::NUM_BINS - 2][Vector2i(colAdd, rowAdd)][c] += weightL;
							mSampleHistogram.histogramWeights[Histogram::NUM_BINS - 1][Vector2i(colAdd, rowAdd)][c] += weightH;
						}
					}

					mSampleHistogram.totalWeight[Vector2i(colAdd, rowAdd)]++;
				}
			}
		}

		void FilmRHF::Denoise()
		{
			HistogramFusion();
		}

		void FilmRHF::HistogramFusion()
		{
			mDenoisedPixelBuffer.Clear();
			mRHFSampleCount.Clear();

			parallel_for(0, mHeight, [this](int y)
			{
				static const int MAX_PATCH_SIZE = 3;
				Color tempPatchBuffer[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
				for (int x = 0; x < mWidth; x++)
				{
					const auto halfPatchSize = Math::Min(mHalfPatchSize, Math::Min(Math::Min(x, y), Math::Min(mWidth - 1 - x, mHeight - 1 - y)));
					const auto minX = Math::Max(x - mHalfWindowSize, halfPatchSize);
					const auto minY = Math::Max(y - mHalfWindowSize, halfPatchSize);
					const auto maxX = Math::Min(x + mHalfWindowSize, mWidth - 1 - halfPatchSize);
					const auto maxY = Math::Min(y + mHalfWindowSize, mHeight - 1 - halfPatchSize);

					auto num = 0;
					memset(tempPatchBuffer, 0, sizeof(Color) * MAX_PATCH_SIZE * MAX_PATCH_SIZE);
					for (auto i = minY; i <= maxY; i++)
					{
						for (auto j = minX; j <= maxX; j++)
						{
							float dist = (x != j || y != i) ? ChiSquareDistance(Vector2i(x, y), Vector2i(j, i), halfPatchSize) : Math::EDX_NEG_INFINITY;
							if (dist < mMaxDist)
							{
								for (auto h = -halfPatchSize; h <= halfPatchSize; h++)
									for (auto w = -halfPatchSize; w <= halfPatchSize; w++)
										tempPatchBuffer[(h + halfPatchSize) * MAX_PATCH_SIZE + w + halfPatchSize] += mPixelBuffer[Vector2i(j + w, i + h)];

								num++;
							}
						}
					}

					if (num > 0)
					{
						EDXLockApply lock(mRHFLock);
						for (auto h = -halfPatchSize; h <= halfPatchSize; h++)
						{
							for (auto w = -halfPatchSize; w <= halfPatchSize; w++)
							{
								mDenoisedPixelBuffer[Vector2i(x + w, y + h)] += tempPatchBuffer[(h + halfPatchSize) * MAX_PATCH_SIZE + w + halfPatchSize] / float(num);
								mRHFSampleCount[Vector2i(x + w, y + h)]++;
							}
						}
					}
				}
			});

			parallel_for(0, mHeight, [this](int y)
			{
				for (int x = 0; x < mWidth; x++)
				{
					float weight = mRHFSampleCount[Vector2i(x, y)];
					if (weight > 0.0f)
						mPixelBuffer[Vector2i(x, y)] = mDenoisedPixelBuffer[Vector2i(x, y)] / weight;
				}
			});
		}

		void GaussianDownSample(const Array<2, Color>& input, Array<2, Color>& output, float scale)
		{

		}

		float FilmRHF::ChiSquareDistance(const Vector2i& coord0, const Vector2i& coord1, const int halfPatchSize)
		{
			int normFactor = 0;
			auto PixelWiseDist = [this, &normFactor](const Vector2i& c0, const Vector2i& c1, const int binIdx) -> float
			{
				float ret = 0.0f;

				const float weight0 = mSampleHistogram.totalWeight[c0];
				const float weight1 = mSampleHistogram.totalWeight[c1];
				for (auto c = 0; c < 3; c++)
				{
					const float histo0 = mSampleHistogram.histogramWeights[binIdx][c0][c];
					const float histo1 = mSampleHistogram.histogramWeights[binIdx][c1][c];
					const float sum = histo0 + histo1;
					if (sum > 1.0f)
					{
						const float diff = weight1 * histo0 - weight0 * histo1;

						ret += diff * diff / ((weight0 * weight1) * sum);
						normFactor++;
					}
				}

				return ret;
			};

			float patchWiseDist = 0.0f;
			for (auto binIdx = 0; binIdx < Histogram::NUM_BINS; binIdx++)
			{
				for (auto i = -halfPatchSize; i <= halfPatchSize; i++)
				{
					for (auto j = -halfPatchSize; j <= halfPatchSize; j++)
					{
						Vector2i c0 = coord0 + Vector2i(i, j);
						Vector2i c1 = coord1 + Vector2i(i, j);

						patchWiseDist += PixelWiseDist(c0, c1, binIdx);
					}
				}
			}

			return patchWiseDist / (normFactor + 1e-4f);
		}
	}
}