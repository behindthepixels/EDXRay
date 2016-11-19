#pragma once

#include "Filter.h"
#include "../ForwardDecl.h"
#include "Graphics/Color.h"
#include "Containers/DimensionalArray.h"
#include "Core/SmartPointer.h"
#include "Windows/Threading.h"

namespace EDX
{
	namespace RayTracer
	{
		class Film
		{
		protected:
			struct Pixel
			{
				Color color;
				Color splat;
				float weight;
			};

			int mWidth, mHeight;
			int mSampleCount;
			DimensionalArray<2, Color>	mPixelBuffer;
			DimensionalArray<2, Pixel>	mAccumulateBuffer;
			UniquePtr<Filter> mpFilter;

			mutable CriticalSection mCS;

			static const float INV_GAMMA;

		public:
			virtual ~Film()
			{
				Release();
			}

			virtual void Init(int width, int height, Filter* pFilter);
			virtual void Resize(int width, int height);
			virtual void Clear();
			void Release();
			int GetPixelCount() const { return mPixelBuffer.LinearSize(); }

			virtual void AddSample(float x, float y, const Color& sample);
			virtual void Splat(float x, float y, const Color& sample);
			void ScaleToPixel(const float splatScale = 0.0f);
			inline void IncreSampleCount() { mSampleCount++; }

			const Color* GetPixelBuffer() const { return mPixelBuffer.Data(); }
			const int GetSampleCount() const { return mSampleCount; }
			virtual void Denoise() {}
		};

		class FilmRHF : public Film
		{
		protected:
			struct Histogram
			{
				static const int NUM_BINS = 20;
				static const float MAX_VAL;

				DimensionalArray<2, Color> histogramWeights[NUM_BINS];
				DimensionalArray<2, float> totalWeights;

				void Init(int w, int h)
				{
					totalWeights.Init(Vector2i(w, h));
					for (auto i = 0; i < NUM_BINS; i++)
						histogramWeights[i].Init(Vector2i(w, h));
				}

				void Clear()
				{
					totalWeights.Clear();
					for (auto i = 0; i < NUM_BINS; i++)
						histogramWeights[i].Clear();
				}
			};

			static const int MAX_SCALE = 3;
			Histogram	mSampleHistogram;
			float		mMaxDist;
			int			mHalfPatchSize;
			int			mHalfWindowSize;
			int			mScale;

			CriticalSection mRHFLock;

		public:
			void Init(int width, int height, Filter* pFilter);
			void Resize(int width, int height);
			void Clear();

			void AddSample(float x, float y, const Color& sample);
			void Denoise();

		private:
			void HistogramFusion(DimensionalArray<2, Color>& input, const Histogram& histogram);
			float ChiSquareDistance(const Vector2i& x, const Vector2i& y, const int halfPatchSize, const Histogram& histogram);

			template<typename T>
			void GaussianDownSample(const DimensionalArray<2, T>& input, DimensionalArray<2, T>& output, float scale);
			void BicubicInterpolation(const DimensionalArray<2, Color>& input, DimensionalArray<2, Color>& output);
		};
	}
}