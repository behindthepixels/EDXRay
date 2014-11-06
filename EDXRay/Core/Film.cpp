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

			mpPixelBuffer.Free();
			mpPixelBuffer.Init(Vector2i(width, height));
			mpAccumulateBuffer.Free();
			mpAccumulateBuffer.Init(Vector2i(width, height));
			mSampleCount = 0;
		}

		void Film::Release()
		{
			mpPixelBuffer.Free();
			mpAccumulateBuffer.Free();
		}

		void Film::Clear()
		{
			mSampleCount = 0;
			mpPixelBuffer.Clear();
			mpAccumulateBuffer.Clear();
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
					Pixel& pixel = mpAccumulateBuffer[Vector2i(colAdd, rowAdd)];

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
					const Pixel& pixel = mpAccumulateBuffer[Vector2i(x, y)];
					mpPixelBuffer[y * mWidth + x] = Math::Pow(pixel.color / pixel.weight, INV_GAMMA);
				}
			}
		}

		// Ray Histogram Fusion film implementation
		void FilmRHF::Init(int width, int height, Filter* pFilter)
		{
			Film::Init(width, height, pFilter);
			mSampleHistogram.Init(width, height);
		}

		void FilmRHF::Resize(int width, int height)
		{
			Film::Resize(width, height);
			mSampleHistogram.Init(width, height);
		}

		void FilmRHF::Clear()
		{
			Film::Clear();
			mSampleHistogram.Clear();
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
					Pixel& pixel = mpAccumulateBuffer[Vector2i(colAdd, rowAdd)];

					float weight = mpFilter->Eval(j - x, i - y);
					Color weightedSample = weight * sample;

					pixel.weight += weight;
					pixel.color += weightedSample;

					Color normalizedSample = Math::Pow(sample, INV_GAMMA) / Histogram::MAX_VAL;
					normalizedSample.r = Math::Min(2.0f, normalizedSample.r);
					normalizedSample.g = Math::Min(2.0f, normalizedSample.g);
					normalizedSample.b = Math::Min(2.0f, normalizedSample.b);

					Vector3 bin = float(Histogram::NUM_BINS - 2) * Vector3(normalizedSample.r, normalizedSample.g, normalizedSample.b);
					Vector3i binLow = Math::FloorToInt(bin);

					for (auto d = 0; d < 3; d++)
					{
						if (binLow[d] < Histogram::NUM_BINS - 2)
						{
							float weightH = bin[d] - binLow[d];
							float weightL = 1.0f - weightH;
							mSampleHistogram.histogramWeights[binLow[d]][Vector2i(colAdd, rowAdd)][d] += weightL;
							mSampleHistogram.histogramWeights[binLow[d] + 1][Vector2i(colAdd, rowAdd)][d] += weightH;
						}
						else
						{
							float weightH = (normalizedSample[d] - 1);
							float weightL = 1.0f - weightH;
							mSampleHistogram.histogramWeights[Histogram::NUM_BINS - 2][Vector2i(colAdd, rowAdd)][d] += weightL;
							mSampleHistogram.histogramWeights[Histogram::NUM_BINS - 1][Vector2i(colAdd, rowAdd)][d] += weightH;
						}
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
					const Pixel& pixel = mpAccumulateBuffer[Vector2i(x, y)];
					mpPixelBuffer[y * mWidth + x] = Math::Pow(pixel.color / pixel.weight, INV_GAMMA);
				}
			}
		}

		float FilmRHF::ChiSquareDistance(const Vector2i& x, const Vector2i& y)
		{
			Vector3 totalWeight0, totalWeight1;
			Vector3i numEmptyBins;
			for (auto i = 0; i < Histogram::NUM_BINS; i++)
			{
				totalWeight0 += mSampleHistogram.histogramWeights[i][x];
				totalWeight1 += mSampleHistogram.histogramWeights[i][y];

				for (auto d = 0; d < 3; d++)
				{
					numEmptyBins[d] += mSampleHistogram.histogramWeights[i][x][d] > 0.0f ? 1 : 0;
					numEmptyBins[d] += mSampleHistogram.histogramWeights[i][y][d] > 0.0f ? 1 : 0;
				}
			}

			Vector3 scaleX, scaleY;


			for (auto i = 0; i < Histogram::NUM_BINS; i++)
			{

		}
	}
}