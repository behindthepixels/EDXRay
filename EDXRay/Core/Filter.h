#pragma once

#include "Math/EDXMath.h"

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
		public:
			BoxFilter()
				: Filter(0.25f)
			{
			}

		public:
			const float Eval(const float dx, const float dy) const
			{
				return 1.0f;
			}
		};

		class GaussianFilter : public Filter
		{
		private:
			float mStdDev;
			float mAlpha;
			float mExpR;

		public:
			GaussianFilter(const float stdDiv = 0.5f)
				: Filter(4 * stdDiv)
				, mStdDev(stdDiv)
			{
				mAlpha = -1.0f / (2.0f * mStdDev * mStdDev);
				mExpR = Math::Exp(mAlpha * mRadius * mRadius);
			}

		public:
			const float Eval(const float dx, const float dy) const
			{
				auto Gaussian = [this](const float d)
				{
					return Math::Max(0.0f, Math::Exp(mAlpha * d * d) - mExpR);
				};

				return Gaussian(dx) * Gaussian(dy);
			}
		};

		class MitchellNetravaliFilter : public Filter
		{
		private:
			float mB, mC;

		public:
			MitchellNetravaliFilter()
				: Filter(2.0f)
				, mB(1.0f / 3.0f)
				, mC(1.0f / 3.0f)
			{
			}

		public:
			const float Eval(const float dx, const float dy) const
			{
				auto MitchellNetravali = [this](const float d)
				{
					float absD = Math::Abs(d);
					float dSqr = d * d, dCube = dSqr * absD;

					if (absD < 1)
					{
						return 1.0f / 6.0f * ((12 - 9 * mB - 6 * mC)*dCube
							+ (-18 + 12 * mB + 6 * mC) * dSqr + (6 - 2 * mB));
					}
					else if (absD < 2)
					{
						return 1.0f / 6.0f * ((-mB - 6 * mC) * dCube + (6 * mB + 30 * mC) * dSqr
							+ (-12 * mB - 48 * mC) * absD + (8 * mB + 24 * mC));
					}
					else
					{
						return 0.0f;
					}
				};

				return MitchellNetravali(dx) * MitchellNetravali(dy);
			}
		};
	}
}