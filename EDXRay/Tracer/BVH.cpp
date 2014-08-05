#include "BVH.h"
#include "../Core/Primitive.h"
#include "../Core/TriangleMesh.h"

#include "Memory/Memory.h"

namespace EDX
{
	namespace RayTracer
	{
		void BVH2::Construct(const vector<RefPtr<Primitive>>& prims)
		{
			ExtractGeometry(prims);

			// Init build bounding box and index info
			vector<TriangleInfo> buildInfo;
			buildInfo.reserve(mBuildTriangleCount);
			for (auto i = 0; i < mBuildTriangleCount; i++)
			{
				BoundingBox box;
				box = Math::Union(box, mpBuildVertices[mpBuildIndices[i].idx1].pos);
				box = Math::Union(box, mpBuildVertices[mpBuildIndices[i].idx2].pos);
				box = Math::Union(box, mpBuildVertices[mpBuildIndices[i].idx3].pos);

				buildInfo[i] = TriangleInfo(i, box);
				mBounds = Math::Union(mBounds, box);
			}

			MemoryArena memory;
			
			Node* pRoot = RecursiveBuildNode(buildInfo, 0, mBuildTriangleCount, memory);
		}

		BVH2::Node* BVH2::RecursiveBuildNode(vector<TriangleInfo>& buildInfo,
			const int startIdx,
			const int endIdx,
			MemoryArena& memory)
		{
			Node* pNode = memory.Alloc<Node>();
			*pNode = Node();
			
			auto numTriangle = endIdx - startIdx;




			return pNode;
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
			mpBuildVertices = new BuildVertex[vertexBufSize];
			mpBuildIndices = new BuildTriangle[triangleBufSize];

			// Fill build buffers
			mBuildVertexCount = mBuildTriangleCount = 0;
			for (auto i = 0; i < prims.size(); i++)
			{
				auto pMesh = prims[i]->GetMesh();

				// Extract indices
				const uint* pIndices = pMesh->GetIndexBuffer();
				for (auto j = 0; j < pMesh->GetTriangleCount(); j++)
				{
					mpBuildIndices[mBuildTriangleCount++] = BuildTriangle(mBuildVertexCount + pIndices[3 * j],
						mBuildVertexCount + pIndices[3 * j + 1],
						mBuildVertexCount + pIndices[3 * j + 2],
						i,
						j);
				}

				// Extract vertices
				const Vector3* pVertices = pMesh->GetPositionBuffer();
				for (auto j = 0; j < pMesh->GetVertexCount(); j++)
				{
					mpBuildVertices[mBuildVertexCount++] = BuildVertex(pVertices[j].x, pVertices[j].y, pVertices[j].z);
				}
			}

			assert(mBuildVertexCount == vertexBufSize);
			assert(mBuildTriangleCount == triangleBufSize);
		}
	}
}