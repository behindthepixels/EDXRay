#pragma once

#include "Filter.h"
#include "../ForwardDecl.h"
#include "Graphics/Color.h"
#include "Memory/Array.h"
#include "Memory/BlockedArray.h"
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
			Array<2, Color>			mPixelBuffer;
			BlockedArray<2, Pixel>	mAccumulateBuffer;
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

				int width, height;
				Array<2, Vector3> histogramWeights[NUM_BINS];
				Array<2, int> totalWeight;

				void Init(int w, int h)
				{
					width = w; height = h;

					totalWeight.Init(Vector2i(width, height));
					for (auto i = 0; i < NUM_BINS; i++)
						histogramWeights[i].Init(Vector2i(width, height));
				}

				void Clear()
				{
					totalWeight.Clear();
					for (auto i = 0; i < NUM_BINS; i++)
						histogramWeights[i].Clear();
				}
			};

			Histogram	mSampleHistogram;
			float		mMaxDist;
			int			mHalfPatchSize;
			int			mHalfWindowSize;
			Array<2, Color>	mDenoisedPixelBuffer;
			Array<2, Color>	mInputBuffer;
			Array<2, int>	mRHFSampleCount;

			EDXLock mRHFLock;

		public:
			void Init(int width, int height, Filter* pFilter);
			void Resize(int width, int height);
			void Clear();

			void AddSample(float x, float y, const Color& sample);
			void Denoise();

		private:
			void HistogramFusion(const int numForcedNeighbors = 0);
			float ChiSquareDistance(const Vector2i& x, const Vector2i& y, const int halfPatchSize);
		};
	}
}