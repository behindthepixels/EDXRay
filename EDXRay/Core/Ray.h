#pragma once

#include "../ForwardDecl.h"
#include "Math/Vec3.h"
#include "Math/Ray.h"

#define RAY_EPSI 1e-4f

namespace EDX
{
	namespace RayTracer
	{
		class RayDifferential : public EDX::RayDifferential
		{
		public:
			const Medium* mpMedium;

		public:
			RayDifferential()
				: EDX::RayDifferential()
			{
			}
			RayDifferential(const Vector3& vOrig, const Vector3& vDir, float max = float(Math::EDX_INFINITY), float min = 0.0f, int depth = 0, const Medium* pMed = nullptr)
				: EDX::RayDifferential(vOrig, vDir, max, min, depth)
				, mpMedium(pMed)
			{
			}
			RayDifferential(const Ray& ray)
				: EDX::RayDifferential(ray)
			{
			}
		};
	}
}