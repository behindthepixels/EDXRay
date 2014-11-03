#include "Film.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		void Film::Init(int width, int height)
		{
			mWidth = width;
			mHeight = height;
			mSampleCount = 0;

			mpPixelBuffer.Free();
			mpPixelBuffer.Init(Vector2i(width, height));
			mpAccumulateBuffer.Free();
			mpAccumulateBuffer.Init(Vector2i(width, height));
		}

		void Film::Resize(int width, int height)
		{
			Init(width, height);
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

		void Film::AddSample(int x, int y, const Color& sample)
		{
			int iRowAdd = mHeight - 1 - y;
			int iColAdd = x;

			mpAccumulateBuffer[iRowAdd * mWidth + iColAdd] += sample;
		}

		void Film::ScaleToPixel()
		{
			float fScale = 1.0f / float(mSampleCount);

			for (int y = 0; y < mHeight; y++)
			{
				for (int x = 0; x < mWidth; x++)
				{
					mpPixelBuffer[y * mWidth + x] = Math::Pow(mpAccumulateBuffer[y * mWidth + x] * fScale, 0.45f);
				}
			}
		}
	}
}