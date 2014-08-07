#include "BVH.h"
#include "../Core/Primitive.h"
#include "../Core/TriangleMesh.h"

#include "Memory/Memory.h"
#include <algorithm>

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
			
			Node* pRoot = RecursiveBuildNode(buildInfo, 0, mBuildTriangleCount, 0, memory);
		}

		BVH2::Node* BVH2::RecursiveBuildNode(vector<TriangleInfo>& buildInfo,
			const int startIdx,
			const int endIdx,
			const int depth,
			MemoryArena& memory)
		{
			// Alloc space for the current node
			Node* pNode = memory.Alloc<Node>();
			*pNode = Node();
			
			auto numTriangles = endIdx - startIdx;
			if (numTriangles <= 1 || depth > MaxDepth) // Create a leaf if triangle count is 1 or depth exceeds maximum
			{
				int* pLeafTris = memory.Alloc<int>(numTriangles);
				for (auto i = startIdx; i < endIdx; i++)
					pLeafTris[i] = buildInfo[i].idx;

				pNode->InitLeaf(pLeafTris, numTriangles);
			}
			else // Split the node and create an interior
			{
				// Compute bounds and centroid bounds for current node
				BoundingBox nodeBounds, centroidBounds;
				for (auto i = startIdx; i < endIdx; i++)
				{
					nodeBounds = Math::Union(nodeBounds, buildInfo[i].bbox);
					centroidBounds = Math::Union(centroidBounds, buildInfo[i].centroid);
				}

				// Get split dimension
				int dim = centroidBounds.MaximumExtent();
				// Create leaf node if maximum extent is zero
				if (centroidBounds.mMin[dim] == centroidBounds.mMax[dim])
				{
					int* pLeafTris = memory.Alloc<int>(numTriangles);
					for (auto i = startIdx; i < endIdx; i++)
						pLeafTris[i] = buildInfo[i].idx;

					pNode->InitLeaf(pLeafTris, numTriangles);
					return pNode;
				}

				// Partition primitives
				int mid = 0;
				mid = (startIdx + endIdx) / 2;
				std::nth_element(&buildInfo[startIdx], &buildInfo[mid], &buildInfo[endIdx - 1] + 1,
					[dim](const TriangleInfo& lhs, const TriangleInfo& rhs)
				{
					return lhs.centroid[dim] < rhs.centroid[dim];
				});


				pNode->InitInterior();

				// Binning
				const int MaxBins = 32;
				int numBins = Math::Min(MaxBins, int(4.0f + 0.05f * numTriangles));

			}





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