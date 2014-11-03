#pragma once

#include "../ForwardDecl.h"
#include "Memory/Array.h"

namespace EDX
{
	namespace RayTracer
	{
		class Filter
		{
		protected:
			float mRadius;

		public:
			Filter(const float rad)
				: mRadius(rad)
			{
			}
			virtual ~Filter() {}

			const float GetRadius() const
			{
				return mRadius;
			}
			virtual const float Eval(const float dx, const float dy) const = 0;
		};

		class BoxFilter : public Filter
		{
			const float Eval(const float dx, const float dy) const
			{
				return 1.0f;
			}
		};

		class Film
		{
		protected:
			//struct Pixel
			//{
			//	Color color;
			//	float weight;
			//};

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
			const int GetSampleCount() const { return mSampleCount; }
		};
	}
}