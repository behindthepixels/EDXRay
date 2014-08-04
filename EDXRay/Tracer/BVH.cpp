#include "BVH.h"
#include "../Core/Primitive.h"
#include "../Core/TriangleMesh.h"

namespace EDX
{
	namespace RayTracer
	{
		void BVH2::Build(const vector<RefPtr<Primitive>>& prims)
		{
			ExtractGeometry(prims);
		}

		void BVH2::ExtractGeometry(const vector<RefPtr<Primitive>>& prims)
		{
			// Get sizes of build buffer
			uint vertexBufSize = 0;
			uint triangleBufSize = 0;
			for (auto i = 0; i < prims.size(); i++)
			{
				auto pMesh = prims[i]->GetMesh();

				vertexBufSize += pMesh->GetVertexCount();
				triangleBufSize += pMesh->GetTriangleCount();
			}

			// Allocate build buffers
			mpVertices = new BuildVertex[vertexBufSize];
			mpTriangleIndices = new BuildTriangle[triangleBufSize];

			// Fill build buffers
			mBuildVertexCount = mBuildTriangleCount = 0;
			for (auto i = 0; i < prims.size(); i++)
			{
				auto pMesh = prims[i]->GetMesh();

				// Extract indices
				const uint* pIndices = pMesh->GetIndexBuffer();
				for (auto j = 0; j < pMesh->GetTriangleCount(); j++)
				{
					mpTriangleIndices[mBuildTriangleCount++] = BuildTriangle(mBuildVertexCount + pIndices[3 * j],
						mBuildVertexCount + pIndices[3 * j + 1],
						mBuildVertexCount + pIndices[3 * j + 2],
						i,
						j);
				}

				// Extract vertices
				const Vector3* pVertices = pMesh->GetPositionBuffer();
				for (auto j = 0; j < pMesh->GetVertexCount(); j++)
				{
					mpVertices[mBuildVertexCount++] = BuildVertex(pVertices[j].x, pVertices[j].y, pVertices[j].z);
					mBounds = Math::Union(mBounds, pVertices[j]);
				}
			}

			assert(mBuildVertexCount == vertexBufSize);
			assert(mBuildTriangleCount == triangleBufSize);
		}
	}
}