#include "Film.h"
#include "Math/EDXMath.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
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

					float weight = mpFilter->Eval(j - x, i - y);
					mpAccumulateBuffer[rowAdd * mWidth + colAdd].weight += weight;
					mpAccumulateBuffer[rowAdd * mWidth + colAdd].color += weight * sample;
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
					mpPixelBuffer[y * mWidth + x] = Math::Pow(mpAccumulateBuffer[y * mWidth + x].color / mpAccumulateBuffer[y * mWidth + x].weight, 0.45f);
				}
			}
		}
	}
}