#include "TriangleMesh.h"

#include "Graphics/ObjMesh.h"
#include "Memory/Memory.h"

namespace EDX
{
	namespace RayTracer
	{
		void TriangleMesh::LoadMesh(const char* path,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot)
		{

		}

		void TriangleMesh::LoadSphere(const float radius,
			const int slices,
			const int stacks,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot)
		{
			ObjMesh mesh;
			mesh.LoadSphere(pos, scl, rot, radius, slices, stacks);

			// Init vertex buffer data
			mVertexCount = mesh.GetVertexCount();
			mpPositionBuffer = new Vector3[mVertexCount];
			mpNormalBuffer = new Vector3[mVertexCount];
			mpTexcoordBuffer = new Vector2[mVertexCount];
			for (auto i = 0; i < mVertexCount; i++)
			{
				const MeshVertex& vertex = mesh.GetVertexAt(i);
				mpPositionBuffer[i] = vertex.position;
				mpNormalBuffer[i] = vertex.normal;
				mpTexcoordBuffer[i] = Vector2(vertex.fU, vertex.fV);
			}

			// Init index buffer data
			mTriangleCount = mesh.GetTriangleCount();
			mpIndexBuffer = new uint[3 * mTriangleCount];
			memcpy(mpIndexBuffer, mesh.GetIndexAt(0), 3 * mTriangleCount * sizeof(uint));
		}

		Vector3 TriangleMesh::GetPositionAt(uint idx) const
		{
			assert(idx < 3 * mTriangleCount);
			assert(mpIndexBuffer);
			assert(mpPositionBuffer);
			return mpPositionBuffer[mpIndexBuffer[idx]];
		}

		Vector3 TriangleMesh::GetNormalAt(uint idx) const
		{
			assert(idx < 3 * mTriangleCount);
			assert(mpIndexBuffer);
			assert(mpPositionBuffer);
			return mpNormalBuffer[mpIndexBuffer[idx]];
		}

		Vector2 TriangleMesh::GetTexcoordAt(uint idx) const
		{
			assert(idx < 3 * mTriangleCount);
			assert(mpIndexBuffer);
			assert(mpPositionBuffer);
			return mpTexcoordBuffer[mpIndexBuffer[idx]];
		}

		void TriangleMesh::Release()
		{
			SafeDeleteArray(mpPositionBuffer);
			SafeDeleteArray(mpNormalBuffer);
			SafeDeleteArray(mpTexcoordBuffer);
			SafeDeleteArray(mpIndexBuffer);
			mVertexCount = 0;
			mTriangleCount = 0;
		}
	}
}