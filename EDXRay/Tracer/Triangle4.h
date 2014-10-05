#pragma once

#include "SIMD/SSE.h"
#include "BVH.h"
#include "../Core/DifferentialGeom.h"

namespace EDX
{
	namespace RayTracer
	{
		class Triangle4
		{
		private:
			Vec3f_SSE	mVertices0;
			Vec3f_SSE	mEdges1;
			Vec3f_SSE	mEdges2;
			Vec3f_SSE	mGeomNormals;

			IntSSE		mMeshIds;
			IntSSE		mTriIds;

		public:
			Triangle4()
			{
			}

			void Pack(const BuildVertex tris[4][3], const BuildTriangle indices[4], const size_t count)
			{
				assert(count <= 4);

				for (auto i = 0; i < count; i++)
				{
					// Pack 4 triangles into SOA form
					mVertices0.x[i] = tris[i][0].pos.x;
					mVertices0.y[i] = tris[i][0].pos.y;
					mVertices0.z[i] = tris[i][0].pos.z;

					Vector3 vEdge1 = tris[i][0].pos - tris[i][1].pos;
					Vector3 vEdge2 = tris[i][2].pos - tris[i][0].pos;

					mEdges1.x[i] = vEdge1.x;
					mEdges1.y[i] = vEdge1.y;
					mEdges1.z[i] = vEdge1.z;

					mEdges2.x[i] = vEdge2.x;
					mEdges2.y[i] = vEdge2.y;
					mEdges2.z[i] = vEdge2.z;

					// Pack normals
					Vector3 vGeomN = Math::Cross(vEdge1, vEdge2);
					mGeomNormals.x[i] = vGeomN.x;
					mGeomNormals.y[i] = vGeomN.y;
					mGeomNormals.z[i] = vGeomN.z;

					// Pack Ids
					mMeshIds[i] = indices[i].meshIdx;
					mTriIds[i] = indices[i].triIdx;
				}
			}

			__forceinline bool Intersect(const Ray& ray, Intersection* pIsect) const
			{
				// Calculate determinant
				const Vec3f_SSE vOrgs = Vec3f_SSE(ray.mOrg);
				const Vec3f_SSE vDirs = Vec3f_SSE(ray.mDir);
				const Vec3f_SSE vC = mVertices0 - vOrgs;
				const Vec3f_SSE vR = Math::Cross(vDirs, vC);
				const FloatSSE det = Math::Dot(mGeomNormals, vDirs);
				const FloatSSE absDet = SSE::Abs(det);
				const FloatSSE signedDet = SSE::SignMask(det);

				// Edge tests
				const FloatSSE U = Math::Dot(vR, mEdges2) ^ signedDet;
				const FloatSSE V = Math::Dot(vR, mEdges1) ^ signedDet;
				BoolSSE valid = (det != FloatSSE(Math::EDX_ZERO)) & (U >= 0.0f) & (V >= 0.0f) & (U + V <= absDet);
				if (SSE::None(valid))
					return false;

				const FloatSSE T = Math::Dot(mGeomNormals, vC) ^ signedDet;
				valid &= (T > absDet * FloatSSE(ray.mMin)) & (T < absDet * FloatSSE(pIsect->mDist));
				if (SSE::None(valid))
					return false;

				const FloatSSE invAbsDet = SSE::Rcp(absDet);
				const FloatSSE u = U * invAbsDet;
				const FloatSSE v = V * invAbsDet;
				const FloatSSE t = T * invAbsDet;

				const size_t idx = SSE::SelectMin(valid, t);
				float hitT = t[idx];
				float baryCentricU = u[idx], baryCentricV = v[idx];

				ray.mMax = hitT;
				pIsect->mDist = hitT;
				pIsect->mU = baryCentricU;
				pIsect->mV = baryCentricV;
				pIsect->mPrimId = mMeshIds[idx];
				pIsect->mTriId = mTriIds[idx];

				return true;
			}

			__forceinline bool Occluded(const Ray& ray) const
			{
				// Calculate determinant
				const Vec3f_SSE vOrgs = Vec3f_SSE(ray.mOrg);
				const Vec3f_SSE vDirs = Vec3f_SSE(ray.mDir);
				const Vec3f_SSE vC = mVertices0 - vOrgs;
				const Vec3f_SSE vR = Math::Cross(vDirs, vC);
				const FloatSSE det = Math::Dot(mGeomNormals, vDirs);
				const FloatSSE absDet = SSE::Abs(det);
				const FloatSSE signedDet = SSE::SignMask(det);

				// Edge tests
				const FloatSSE U = Math::Dot(vR, mEdges2) ^ signedDet;
				const FloatSSE V = Math::Dot(vR, mEdges1) ^ signedDet;
				BoolSSE valid = (det != FloatSSE(Math::EDX_ZERO)) & (U >= 0.0f) & (V >= 0.0f) & (U + V <= absDet);
				if (SSE::None(valid))
					return false;

				const FloatSSE T = Math::Dot(mGeomNormals, vC) ^ signedDet;
				valid &= (T > absDet * FloatSSE(ray.mMin)) & (T < absDet * FloatSSE(ray.mMax));
				if (SSE::None(valid))
					return false;

				return true;
			}
		};

		struct Triangle4Node
		{
			uint triangleCount;
			Triangle4 tri4;
		};
	}
}