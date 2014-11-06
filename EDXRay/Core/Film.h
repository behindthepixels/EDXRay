#pragma once

#include "Filter.h"
#include "../ForwardDecl.h"
#include "Graphics/Color.h"
#include "Memory/Array.h"
#include "Memory/BlockedArray.h"
#include "Memory/RefPtr.h"

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
			Array<2, Color> mpPixelBuffer;
			BlockedArray<2, Pixel> mpAccumulateBuffer;
			RefPtr<Filter> mpFilter;

			static const float INV_GAMMA;

		public:
			~Film()
			{
				Release();
			}

			virtual void Init(int width, int height, Filter* pFilter);
			virtual void Resize(int width, int height);
			virtual void Clear();
			void Release();

			virtual void AddSample(float x, float y, const Color& sample);
			virtual void ScaleToPixel();
			inline void IncreSampleCount() { mSampleCount++; }

			const Color* GetPixelBuffer() const { return mpPixelBuffer.Data(); }
			const int GetSampleCount() const { return mSampleCount; }
		};

		class FilmRHF : public Film
		{
		protected:
			struct Histogram
			{
				static const int NUM_BINS = 30;
				static const float MAX_VAL;

				int width, height;
				BlockedArray<2, Vector3> histogramWeights[NUM_BINS];

				void Init(int w, int h)
				{
					width = w; height = h;

					for (auto i = 0; i < NUM_BINS; i++)
						histogramWeights[i].Init(width, height);
				}

				void Clear()
				{
					for (auto i = 0; i < NUM_BINS; i++)
						histogramWeights[i].Clear();
				}
			};

			Histogram mSampleHistogram;

		public:
			void Init(int width, int height, Filter* pFilter);
			void Resize(int width, int height);
			void Clear();

			void AddSample(float x, float y, const Color& sample);
			void ScaleToPixel();

		private:
			float ChiSquareDistance(const Vector2i& x, const Vector2i& y);
		};
	}
}