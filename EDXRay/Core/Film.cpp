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
			mpFilter.Reset(pFilter);
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
			//mPixelBuffer.Free();
			//mAccumulateBuffer.Free();
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

		void Film::Splat(float x, float y, const Color& sample)
		{
			int X = Math::FloorToInt(x);
			int Y = Math::FloorToInt(y);
			X = Math::Clamp(X, 0, mWidth - 1);
			Y = Math::Clamp(Y, 0, mHeight - 1);

			int rowAdd = mHeight - 1 - Y;
			int colAdd = X;
			Pixel& pixel = mAccumulateBuffer[Vector2i(colAdd, rowAdd)];
			pixel.splat += sample;
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
					mPixelBuffer[y * mWidth + x] = Math::Pow(pixel.color / pixel.weight + pixel.splat / float(mSampleCount), INV_GAMMA);
				}
			});
		}

		// Ray Histogram Fusion film implementation
		void FilmRHF::Init(int width, int height, Filter* pFilter)
		{
			Film::Init(width, height, pFilter);
			mSampleHistogram.Init(width, height);

			mMaxDist = 0.6f;
			mHalfPatchSize = 1;
			mHalfWindowSize = 6;
			mScale = 3;
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

					mSampleHistogram.totalWeights[Vector2i(colAdd, rowAdd)] += 1.0f;
				}
			}
		}

		void FilmRHF::Denoise()
		{
			DimensionalArray<2, Color> scaledImage;
			DimensionalArray<2, Color> prevImage;
			Histogram scaledHistogram;

			float totalWeight = 0.0f;
			for (auto i = 0; i < mSampleHistogram.totalWeights.LinearSize(); i++)
				totalWeight += mSampleHistogram.totalWeights[i];

			for (auto s = mScale - 1; s >= 0; s--)
			{
				float scale = 1.0f / float(1 << s);
				if (s > 0)
				{
					for (auto b = 0; b < Histogram::NUM_BINS; b++)
						GaussianDownSample(mSampleHistogram.histogramWeights[b], scaledHistogram.histogramWeights[b], scale);

					GaussianDownSample(mSampleHistogram.totalWeights, scaledHistogram.totalWeights, scale);

					float scaledTotalWeight = 0.0f;
					for (auto i = 0; i < scaledHistogram.totalWeights.LinearSize(); i++)
						scaledTotalWeight += scaledHistogram.totalWeights[i];

					float ratio = totalWeight / scaledTotalWeight;
					for (auto b = 0; b < Histogram::NUM_BINS; b++)
					{
						for (auto i = 0; i < scaledHistogram.histogramWeights[b].LinearSize(); i++)
							scaledHistogram.histogramWeights[b][i] *= ratio;
					}

					GaussianDownSample(mPixelBuffer, scaledImage, scale);
				}
				else
				{
					scaledImage = mPixelBuffer;
					scaledHistogram = mSampleHistogram;
				}

				HistogramFusion(scaledImage, scaledHistogram);

				if (s < mScale - 1)
				{
					DimensionalArray<2, Color> posTerm;
					posTerm.Init(scaledImage.Size());
					BicubicInterpolation(prevImage, posTerm);

					DimensionalArray<2, Color> negTermD;
					GaussianDownSample(scaledImage, negTermD, 0.5f);
					DimensionalArray<2, Color> negTerm;
					negTerm.Init(scaledImage.Size());
					BicubicInterpolation(negTermD, negTerm);

					for (auto i = 0; i < scaledImage.LinearSize(); i++)
						scaledImage[i] += posTerm[i] - negTerm[i];
				}

				prevImage = scaledImage;
			}

			mPixelBuffer = scaledImage;
		}

		void FilmRHF::HistogramFusion(DimensionalArray<2, Color>& input, const Histogram& histogram)
		{
			DimensionalArray<2, Color> mDenoisedPixelBuffer;
			mDenoisedPixelBuffer.Init(input.Size());
			DimensionalArray<2, int> mRHFSampleCount;
			mRHFSampleCount.Init(input.Size());

			const int width = input.Size(0);
			const int height = input.Size(1);

			parallel_for(0, height, [&](int y)
			{
				static const int MAX_PATCH_SIZE = 3;
				Color tempPatchBuffer[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
				for (int x = 0; x < width; x++)
				{
					const auto halfPatchSize = Math::Min(mHalfPatchSize, Math::Min(Math::Min(x, y), Math::Min(width - 1 - x, height - 1 - y)));
					const auto minX = Math::Max(x - mHalfWindowSize, halfPatchSize);
					const auto minY = Math::Max(y - mHalfWindowSize, halfPatchSize);
					const auto maxX = Math::Min(x + mHalfWindowSize, width - 1 - halfPatchSize);
					const auto maxY = Math::Min(y + mHalfWindowSize, height - 1 - halfPatchSize);

					auto num = 0;
					Memory::Memset(tempPatchBuffer, 0, sizeof(Color) * MAX_PATCH_SIZE * MAX_PATCH_SIZE);
					for (auto i = minY; i <= maxY; i++)
					{
						for (auto j = minX; j <= maxX; j++)
						{
							float dist = (x != j || y != i) ? ChiSquareDistance(Vector2i(x, y), Vector2i(j, i), halfPatchSize, histogram) : Math::EDX_NEG_INFINITY;
							if (dist < mMaxDist)
							{
								for (auto h = -halfPatchSize; h <= halfPatchSize; h++)
									for (auto w = -halfPatchSize; w <= halfPatchSize; w++)
										tempPatchBuffer[(h + halfPatchSize) * MAX_PATCH_SIZE + w + halfPatchSize] += input[Vector2i(j + w, i + h)];

								num++;
							}
						}
					}

					if (num > 0)
					{
						ScopeLock lock(&mRHFLock);
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

			parallel_for(0, height, [&](int y)
			{
				for (int x = 0; x < width; x++)
				{
					float weight = mRHFSampleCount[Vector2i(x, y)];
					if (weight > 0.0f)
						input[Vector2i(x, y)] = mDenoisedPixelBuffer[Vector2i(x, y)] / weight;
				}
			});
		}

		float FilmRHF::ChiSquareDistance(const Vector2i& coord0, const Vector2i& coord1, const int halfPatchSize, const Histogram& histogram)
		{
			int normFactor = 0;
			int count = 0;
			auto PixelWiseDist = [this, &normFactor, &count, &histogram](const Vector2i& c0, const Vector2i& c1, const int binIdx) -> float
			{
				float ret = 0.0f;
				for (auto c = 0; c < 3; c++)
				{
					const float histo0 = histogram.histogramWeights[binIdx][c0][c];
					const float histo1 = histogram.histogramWeights[binIdx][c1][c];
					const float sum = histo0 + histo1;
					if (sum > 1.0f)
					{
						const float weight0 = histogram.totalWeights[c0];
						const float weight1 = histogram.totalWeights[c1];

						const float diff = weight1 * histo0 - weight0 * histo1;

						ret += diff * diff / ((weight0 * weight1) * sum);
						normFactor++;
					}
					else
						count++;
				}

				return ret;
			};

			float patchWiseDist = 0.0f;
			const float maxNormFac = 3 * Histogram::NUM_BINS * (2 * halfPatchSize + 1) * (2 * halfPatchSize + 1);
			for (auto binIdx = 0; binIdx < Histogram::NUM_BINS; binIdx++)
			{
				for (auto i = -halfPatchSize; i <= halfPatchSize; i++)
				{
					for (auto j = -halfPatchSize; j <= halfPatchSize; j++)
					{
						Vector2i c0 = coord0 + Vector2i(i, j);
						Vector2i c1 = coord1 + Vector2i(i, j);

						patchWiseDist += PixelWiseDist(c0, c1, binIdx);

						if (patchWiseDist / (maxNormFac - count) >= mMaxDist)
							return Math::EDX_INFINITY;
					}
				}
			}

			return patchWiseDist / (normFactor + 1e-4f);
		}

		template<typename T>
		void FilmRHF::GaussianDownSample(const DimensionalArray<2, T>& input, DimensionalArray<2, T>& output, float scale)
		{
			static const float SIGMA_SCALE = 0.55f;
			float sigma = scale < 1.0 ? SIGMA_SCALE * Math::Sqrt(1 / (scale * scale) - 1) : SIGMA_SCALE;

			const float prec = 2.0f;
			const int halfSize = Math::CeilToInt(sigma * Math::Sqrt(2.0f * prec * logf(10.0f)));
			const int kernelSize = 1 + 2 * halfSize;
			
			float* pKernel = new float[kernelSize];

			const auto dimX = input.Size(0);
			const auto dimY = input.Size(1);
			const auto scaledDimX = Math::CeilToInt(scale * dimX);
			const auto scaledDimY = Math::CeilToInt(scale * dimY);
			output.Init(Vector2i(scaledDimX, scaledDimY));

			auto GaussianKernel = [&kernelSize, &pKernel, sigma](const float mean)
			{
				float sum = 0.0f;
				for (auto i = 0; i < kernelSize; i++)
				{
					float val = (i - mean) / sigma;
					pKernel[i] = Math::Exp(-0.5f * val * val);
					sum += pKernel[i];
				}

				// Normalization
				if (sum > 0.0f)
				{
					for (auto i = 0; i < kernelSize; i++)
						pKernel[i] /= sum;
				}
			};

			DimensionalArray<2, T> auxBuf;
			auxBuf.Init(Vector2i(scaledDimX, dimY));

			for (auto x = 0; x < scaledDimX; x++)
			{
				const float org = (x + 0.5f) / scale;
				const int crd = Math::FloorToInt(org);

				GaussianKernel(halfSize + org - crd - 0.5f);

				for (auto y = 0; y < dimY; y++)
				{
					T sum = 0.0f;
					for (auto k = 0; k < kernelSize; k++)
					{
						auto idx = crd - halfSize + k;
						idx = Math::Clamp(idx, 0, dimX - 1);

						sum += input[Vector2i(idx, y)] * pKernel[k];
					}

					auxBuf[Vector2i(x, y)] = sum;
				}
			}

			for (auto y = 0; y < scaledDimY; y++)
			{
				const float org = (y + 0.5f) / scale;
				const int crd = Math::FloorToInt(org);

				GaussianKernel(halfSize + org - crd - 0.5f);

				for (auto x = 0; x < scaledDimX; x++)
				{
					T sum = 0.0f;
					for (auto k = 0; k < kernelSize; k++)
					{
						auto idx = crd - halfSize + k;
						idx = Math::Clamp(idx, 0, dimY - 1);

						sum += auxBuf[Vector2i(x, idx)] * pKernel[k];
					}

					output[Vector2i(x, y)] = sum;
				}
			}

			Memory::SafeDeleteArray(pKernel);
		}

		void FilmRHF::BicubicInterpolation(const DimensionalArray<2, Color>& input, DimensionalArray<2, Color>& output)
		{
			const auto dimX = input.Size(0);
			const auto dimY = input.Size(1);
			const auto scaledDimX = output.Size(0);
			const auto scaledDimY = output.Size(1);

			float scaleX = scaledDimX / float(dimX);
			float scaleY = scaledDimY / float(dimY);

			DimensionalArray<2, Color> auxBuf;
			auxBuf.Init(scaledDimX, dimY);

			float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			auto CalcBicubicWeights = [&weights](float t, float a)
			{
				float t2 = t * t;
				float at = a * t;
				weights[0] = a * t2 * (1.0f - t);
				weights[1] = (2.0f * a + 3.0f - (a + 2.0f) * t) * t2 - at;
				weights[2] = ((a + 2.0f) * t - a - 3.0f) * t2 + 1.0f;
				weights[3] = a * (t - 2.0f) * t2 + at;
			};

			for (auto x = 0; x < scaledDimX; x++)
			{
				float org = (x + 0.5f) / scaleX;

				if (org < 0.0f || org > dimX)
				{
					for (auto y = 0; y < dimY; y++)
						auxBuf[Vector2i(x, y)] = Color::BLACK;

					continue;
				}

				org -= 0.5f;
				int crd = Math::FloorToInt(org);
				float lin = org - crd;
				CalcBicubicWeights(lin, -0.5f);

				for (auto y = 0; y < dimY; y++)
				{
					Color sum = Color::BLACK;
					for (auto l = -1; l <= 2; l++)
					{
						auto clampedL = Math::Clamp(crd + l, 0, dimX - 1);
						sum += weights[2 - l] * input[Vector2i(clampedL, y)];
					}

					auxBuf[Vector2i(x, y)] = sum;
				}
			}
			for (auto y = 0; y < scaledDimY; y++)
			{
				float org = (y + 0.5f) / scaleY;

				if (org < 0.0f || org > dimY)
				{
					for (auto x = 0; x < scaledDimX; x++)
						auxBuf[Vector2i(x, y)] = Color::BLACK;

					continue;
				}

				org -= 0.5f;
				int crd = Math::FloorToInt(org);
				float lin = org - crd;
				CalcBicubicWeights(lin, -0.5f);

				for (auto x = 0; x < scaledDimX; x++)
				{
					Color sum = Color::BLACK;
					for (auto l = -1; l <= 2; l++)
					{
						auto clampedL = Math::Clamp(crd + l, 0, dimY - 1);
						sum += weights[2 - l] * auxBuf[Vector2i(x, clampedL)];
					}

					output[Vector2i(x, y)] = sum;
				}
			}
		}
	}
}