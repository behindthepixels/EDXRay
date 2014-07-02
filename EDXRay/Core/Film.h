#pragma once

#include "../ForwardDecl.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		class Film
		{
		protected:
			int mWidth, mHeight;
			Color* mpDisplayBuffer;
			Color* mpAccumulateBuffer;

		public:
			Color* GetDisplayBuffer() const { return mpDisplayBuffer; }

			void Accumulate(int x, int y, const Color& color)
			{
				mpAccumulateBuffer[y * mWidth + x] += color;
			}
		};
	}
}