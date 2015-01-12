#pragma once

#include "Filter.h"
#include "../ForwardDecl.h"
#include "Graphics/Color.h"
#include "Memory/Array.h"
#include "Memory/RefPtr.h"
#include "Windows/Thread.h"

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
				float weight;
			};

			int mWidth, mHeight;
			int mSampleCount;
			Array<2, Color>	mPixelBuffer;
			Array<2, Pixel>	mAccumulateBuffer;
			RefPtr<Filter> mpFilter;

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

			virtual void AddSample(float x, float y, const Color& sample);
			void ScaleToPixel();
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

				uint64 numSamples;
				Array<2, Color> histogramWeights[NUM_BINS];
				Array<2, float> totalWeights;

				void Init(int w, int h)
				{
					numSamples = 0;

					totalWeights.Init(Vector2i(w, h));
					for (auto i = 0; i < NUM_BINS; i++)
						histogramWeights[i].Init(Vector2i(w, h));
				}

				void Clear()
				{
					numSamples = 0;
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

			EDXLock mRHFLock;

		public:
			void Init(int width, int height, Filter* pFilter);
			void Resize(int width, int height);
			void Clear();

			void AddSample(float x, float y, const Color& sample);
			void Denoise();

		private:
			void HistogramFusion(Array<2, Color>& input, const Histogram& histogram);
			float ChiSquareDistance(const Vector2i& x, const Vector2i& y, const int halfPatchSize, const Histogram& histogram);

			template<typename T>
			void GaussianDownSample(const Array<2, T>& input, Array<2, T>& output, float scale);
			void BicubicInterpolation(const Array<2, Color>& input, Array<2, Color>& output);
		};
	}
}