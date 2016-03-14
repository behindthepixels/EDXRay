#include "Medium.h"


namespace EDX
{
	namespace RayTracer
	{
		float PhaseFunctionHG::Eval(const Vector3& wo, const Vector3& wi) const
		{
			return HenyeyGreenstein(Math::Dot(wo, wi), mG);
		}

		float PhaseFunctionHG::Sample(const Vector3& wo, Vector3* pvIn, const Vector2& sample) const
		{
			float cosTheta;
			if (Math::Abs(mG) < 1e-3)
			{
				cosTheta = 1 - 2 * sample.u;
			}
			else
			{
				float sqrTerm = (1 - mG * mG) / (1 - mG + 2 * mG * sample.u);
				cosTheta = (1 + mG * mG - sqrTerm * sqrTerm) / (2 * mG);
			}

			// Compute direction _wi_ for Henyey--Greenstein sample
			float sinTheta = Math::Sqrt(Math::Max(0.0f, 1 - cosTheta * cosTheta));
			float phi = float(Math::EDX_TWO_PI) * sample.v;

			Vector3 v1, v2;
			Math::CoordinateSystem(wo, &v1, &v2);
			*pvIn = Math::SphericalDirection(sinTheta, cosTheta, phi, v1, v2, -wo);

			return HenyeyGreenstein(-cosTheta, mG);
		}
	}
}