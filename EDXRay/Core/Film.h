#pragma once

#include "Filter.h"
#include "../ForwardDecl.h"
#include "Graphics/Color.h"
#include "Memory/Array.h"
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
			Array<2, Pixel> mpAccumulateBuffer;
			RefPtr<Filter> mpFilter;

		public:
			~Film()
			{
				Release();
			}

			void Init(int width, int height, Filter* pFilter);
			void Resize(int width, int height);
			void Release();
			void Clear();

			void AddSample(float x, float y, const Color& sample);
			void ScaleToPixel();
			inline void IncreSampleCount() { mSampleCount++; }

			const Color* GetPixelBuffer() const { return mpPixelBuffer.Data(); }
			const int GetSampleCount() const { return mSampleCount; }
		};
	}
}