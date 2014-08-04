#include "Primitive.h"
#include "TriangleMesh.h"
#include "Math/Ray.h"
#include "DifferentialGeom.h"

namespace EDX
{
	namespace RayTracer
	{
		Primitive::Primitive(TriangleMesh* pMesh)
			: mpMesh(pMesh)
		{
		}

		bool Primitive::Intersect(const Ray& ray, Intersection* pIsect) const
		{
			bool hit = false;
			for (auto i = 0; i < mpMesh->GetTriangleCount(); i++)
			{
				const Vector3 vt1 = mpMesh->GetPositionAt(3 * i);
				const Vector3 vt2 = mpMesh->GetPositionAt(3 * i + 1);
				const Vector3 vt3 = mpMesh->GetPositionAt(3 * i + 2);

				const Vector3 vEdge1 = vt1 - vt2;
				const Vector3 vEdge2 = vt3 - vt1;
				const Vector3 vGNorm = Math::Cross(vEdge1, vEdge2);

				// Calculate determinant
				const Vector3 vC = vt1 - ray.mOrg;
				const Vector3 vR = Math::Cross(ray.mDir, vC);
				const float fDet = Math::Dot(vGNorm, ray.mDir);
				const float fAbsDet = Math::Abs(fDet);
				const float fSgnDet = Math::SignMask(fDet);
				if (fDet == 0.0f)
					continue;

				// Test against edge p2 p0
				const float fU = Math::Xor(Math::Dot(vR, vEdge2), fSgnDet);
				if (fU < 0.0f)
					continue;

				// Test against edge p0 p1
				const float fV = Math::Xor(Math::Dot(vR, vEdge1), fSgnDet);
				if (fV < 0.0f)
					continue;

				// Test against edge p1 p2
				const float fW = fAbsDet - fU - fV;
				if (fW < 0.0f)
					continue;

				// Perform depth test
				const float fT = Math::Xor(Math::Dot(vGNorm, vC), fSgnDet);
				if (fT < fAbsDet * ray.mMin || fAbsDet * pIsect->mDist < fT)
					continue;

				// Missing texture coords

				// Update hit information
				const float rcpAbsDet = 1.0f / fAbsDet;

				float fHitT = fT * rcpAbsDet;
				float fB1 = fU * rcpAbsDet, fB2 = fV * rcpAbsDet, fB0 = 1.0f - fB1 - fB2;

				Vector3 pos = ray.CalcPoint(fHitT);
				Vector3 norm = -vGNorm;

				ray.mMax = fHitT;
				pIsect->mDist = fHitT;

				hit = true;
			}

			return hit;
		}
	}
}