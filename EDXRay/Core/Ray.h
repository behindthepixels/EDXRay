#pragma once

#include "../ForwardDecl.h"
#include "Math/Vec3.h"
#include "Math/Ray.h"
#include "Math/Matrix.h"

#define RAY_EPSI 1e-4f

namespace EDX
{
	namespace RayTracer
	{
		class Ray : public EDX::Ray
		{
		public:
			const Medium* mpMedium;

		public:
			Ray()
				: EDX::Ray()
				, mpMedium(nullptr)
			{
			}
			Ray(const Vector3& pt, const Vector3& dir, const Medium* pMed = nullptr, float max = float(Math::EDX_INFINITY), float min = 0.0f, int depth = 0)
				: EDX::Ray(pt, dir, max, min, depth)
				, mpMedium(pMed)
			{
			}
			~Ray()
			{
			}
		};

		class RayDifferential : public Ray
		{
		public:
			Vector3 mDxOrg, mDyOrg;
			Vector3 mDxDir, mDyDir;

			bool mHasDifferential;

		public:
			RayDifferential()
				: Ray(), mHasDifferential(false)
			{
			}
			RayDifferential(const Vector3& vOrig, const Vector3& vDir, const Medium* pMed = nullptr, float max = float(Math::EDX_INFINITY), float min = 0.0f, int depth = 0)
				: Ray(vOrig, vDir, pMed, max, min, depth)
				, mHasDifferential(false)
			{
			}
			RayDifferential(const Ray& ray)
				: Ray(ray), mHasDifferential(false)
			{
			}
		};

		inline Ray TransformRay(const Ray& rRay, const Matrix& mat)
		{
			Ray ray = rRay;
			ray.mOrg = Matrix::TransformPoint(ray.mOrg, mat);
			ray.mDir = Matrix::TransformVector(ray.mDir, mat);

			return ray;
		}

		inline RayDifferential TransformRayDiff(const RayDifferential& rRay, const Matrix& mat)
		{
			RayDifferential ray = RayDifferential(TransformRay(Ray(rRay), mat));
			ray.mDxOrg = Matrix::TransformPoint(rRay.mDxOrg, mat);
			ray.mDyOrg = Matrix::TransformPoint(rRay.mDyOrg, mat);
			ray.mDxDir = Matrix::TransformVector(rRay.mDxDir, mat);
			ray.mDyDir = Matrix::TransformVector(rRay.mDyDir, mat);
			ray.mHasDifferential = rRay.mHasDifferential;

			return ray;
		}
	}
}