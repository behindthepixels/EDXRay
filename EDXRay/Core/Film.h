#pragma once

#include "../ForwardDecl.h"
#include "Memory/Array.h"

namespace EDX
{
	namespace RayTracer
	{
		class Film
		{
		protected:
			int mWidth, mHeight;
			int mSampleCount;
			Array<2, Color> mpPixelBuffer;
			Array<2, Color> mpAccumulateBuffer;

		public:
			~Film()
			{
				Release();
			}

			void Init(int width, int height);
			void Resize(int width, int height);
			void Release();
			void Clear();

			void AddSample(int x, int y, const Color& sample);
			void ScaleToPixel();
			inline void IncreSampleCount() { mSampleCount++; }

			const Color* GetPixelBuffer() const { return mpPixelBuffer.Data(); }
		};
	}
}