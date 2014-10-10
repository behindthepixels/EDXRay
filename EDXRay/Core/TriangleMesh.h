#pragma once

#include "../ForwardDecl.h"
#include "Math/Vector.h"
#include "Memory/RefPtr.h"

namespace EDX
{
	namespace RayTracer
	{
		class TriangleMesh
		{
		protected:
			Vector3*	mpPositionBuffer;
			Vector3*	mpNormalBuffer;
			Vector2*	mpTexcoordBuffer;
			uint*		mpIndexBuffer;

			uint		mVertexCount;
			uint		mTriangleCount;

			RefPtr<const ObjMesh>	mpObjMesh;

		public:
			TriangleMesh()
				: mpPositionBuffer(nullptr)
				, mpNormalBuffer(nullptr)
				, mpTexcoordBuffer(nullptr)
				, mpIndexBuffer(nullptr)
				, mVertexCount(0)
				, mTriangleCount(0)
			{
			}
			~TriangleMesh()
			{
				Release();
			}

			void LoadMesh(const ObjMesh* pObjMesh, const BSDFType bsdfType);

			void PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const;

			// Accessor for geometry
			const Vector3& GetPositionAt(uint idx) const;
			const Vector3& GetNormalAt(uint idx) const;
			const Vector2& GetTexCoordAt(uint idx) const;
			const uint* GetIndexAt(uint triIdx) const
			{
				assert(triIdx < 3 * mTriangleCount);
				assert(mpIndexBuffer);
				return &mpIndexBuffer[3 * triIdx];
			}

			// For extraction
			const Vector3* GetPositionBuffer() const
			{
				return mpPositionBuffer;
			}
			const Vector3* GetNormalBuffer() const
			{
				return mpNormalBuffer;
			}
			const Vector2* GetTexCoordBuffer() const
			{
				return mpTexcoordBuffer;
			}
			const uint* GetIndexBuffer() const
			{
				return mpIndexBuffer;
			}

			uint GetTriangleCount() const
			{
				return mTriangleCount;
			}
			uint GetVertexCount() const
			{
				return mVertexCount;
			}
			const ObjMesh* GetObjMeshHandle() const
			{
				return mpObjMesh.Ptr();
			}

			void Release();
		};
	}
}