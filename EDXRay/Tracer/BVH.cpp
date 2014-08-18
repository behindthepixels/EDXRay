#include "BVH.h"
#include "../Core/Primitive.h"
#include "../Core/TriangleMesh.h"
#include "../Core/DifferentialGeom.h"
#include "Triangle4.h"

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

				buildInfo.push_back(TriangleInfo(i, box));
				mBounds = Math::Union(mBounds, box);
			}

			MemoryArena memory;

			// Alloc space for the root BuildNode
			BuildNode* pBuildRoot = memory.Alloc<BuildNode>();
			RecursiveBuildBuildNode(pBuildRoot, buildInfo, 0, mBuildTriangleCount, 0, memory);

			mpRoot = AllocAligned<Node>(mTreeBufSize, 64);
			uint offset = 0;
			LinearizeNodes(pBuildRoot, &offset);
			assert(offset == mTreeBufSize);
		}

		void BVH2::RecursiveBuildBuildNode(BuildNode* pNode,
			vector<TriangleInfo>& buildInfo,
			const int startIdx,
			const int endIdx,
			const int depth,
			MemoryArena& memory)
		{
			*pNode = BuildNode();
			auto numTriangles = endIdx - startIdx;

			auto CreateLeaf = [&]
			{
				uint packedCount = (numTriangles + 3) / 4;
				uint infoIdx = startIdx;

				Triangle4* pTri4 = memory.Alloc<Triangle4>(packedCount);
				for (auto i = 0; i < packedCount; i++)
				{
					BuildVertex triangles[4][3] = { 0 };
					BuildTriangle indices[4] = { 0 };
					uint count = 0;
					for (; count < 4; count++)
					{
						if (infoIdx >= endIdx)
							break;

						indices[count] = mpBuildIndices[buildInfo[infoIdx++].idx];
						const Vector3& vt1 = mpBuildVertices[indices[count].idx1].pos;
						const Vector3& vt2 = mpBuildVertices[indices[count].idx2].pos;
						const Vector3& vt3 = mpBuildVertices[indices[count].idx3].pos;

						triangles[count][0].pos = vt1;
						triangles[count][1].pos = vt2;
						triangles[count][2].pos = vt3;
					}

					pTri4[i].Pack(triangles, indices, count);
				}

				pNode->InitLeaf(pTri4, packedCount);
				mTreeBufSize += 4 * packedCount;
			};

			if (numTriangles <= 4 || depth > MaxDepth) // Create a leaf if triangle count is 1 or depth exceeds maximum
			{
				CreateLeaf();
			}
			else // Split the BuildNode and create an interior
			{
				// Compute centroid bounds for current BuildNode
				BoundingBox centroidBounds;
				for (auto i = startIdx; i < endIdx; i++)
					centroidBounds = Math::Union(centroidBounds, buildInfo[i].centroid);

				// Get split dimension
				int dim = centroidBounds.MaximumExtent();
				// Create leaf BuildNode if maximum extent is zero
				if (centroidBounds.mMin[dim] == centroidBounds.mMax[dim])
				{
					CreateLeaf();
					return;
				}

				// Partition primitives
				// Binning
				//const int MaxBins = 32;
				//int numBins = Math::Min(MaxBins, int(4.0f + 0.05f * numTriangles));

				int mid = 0;
				mid = (startIdx + endIdx) / 2;
				std::nth_element(&buildInfo[startIdx], &buildInfo[mid], &buildInfo[endIdx - 1] + 1,
					[dim](const TriangleInfo& lhs, const TriangleInfo& rhs)
				{
					return lhs.centroid[dim] < rhs.centroid[dim];
				});

				// Compute left and right bounds
				BoundingBox leftBounds, rightBounds;
				for (auto i = startIdx; i < mid; i++)
					leftBounds = Math::Union(leftBounds, buildInfo[i].bbox);
				for (auto i = mid; i < endIdx; i++)
					rightBounds = Math::Union(rightBounds, buildInfo[i].bbox);


				BuildNode* pLeft = memory.Alloc<BuildNode>();
				BuildNode* pRight = memory.Alloc<BuildNode>();
				pNode->InitInterior(leftBounds, rightBounds, pLeft, pRight);

				RecursiveBuildBuildNode(pLeft, buildInfo, startIdx, mid, depth + 1, memory);
				RecursiveBuildBuildNode(pRight, buildInfo, mid, endIdx, depth + 1, memory);
				mTreeBufSize += 1;
			}

			return;
		}

		uint BVH2::LinearizeNodes(BuildNode* pBuildNode, uint* pOffset)
		{
			Node* pNode = &mpRoot[*pOffset];

			pNode->minMaxBoundsX[0] = pBuildNode->leftBounds.mMin.x;
			pNode->minMaxBoundsX[1] = pBuildNode->rightBounds.mMin.x;
			pNode->minMaxBoundsX[2] = pBuildNode->leftBounds.mMax.x;
			pNode->minMaxBoundsX[3] = pBuildNode->rightBounds.mMax.x;

			pNode->minMaxBoundsY[0] = pBuildNode->leftBounds.mMin.y;
			pNode->minMaxBoundsY[1] = pBuildNode->rightBounds.mMin.y;
			pNode->minMaxBoundsY[2] = pBuildNode->leftBounds.mMax.y;
			pNode->minMaxBoundsY[3] = pBuildNode->rightBounds.mMax.y;

			pNode->minMaxBoundsZ[0] = pBuildNode->leftBounds.mMin.z;
			pNode->minMaxBoundsZ[1] = pBuildNode->rightBounds.mMin.z;
			pNode->minMaxBoundsZ[2] = pBuildNode->leftBounds.mMax.z;
			pNode->minMaxBoundsZ[3] = pBuildNode->rightBounds.mMax.z;

			uint currOffset = (*pOffset);

			if (pBuildNode->primCount > 0) // Create leaf node
			{
				Triangle4Node* pLeafNode = (Triangle4Node*)pNode;
				for (auto i = 0; i < pBuildNode->primCount; i++)
				{
					pLeafNode[i].triangleCount = pBuildNode->primCount - i;
					memcpy(&pLeafNode[i].tri4, &pBuildNode->pTriangles[i], sizeof(Triangle4));
				}

				(*pOffset) += 4 * pBuildNode->primCount;
			}
			else // Interior
			{
				pNode->triangleCount = 0;
				(*pOffset)++;
				LinearizeNodes(pBuildNode->pChildren[0], pOffset);
				pNode->secondChildOffset = LinearizeNodes(pBuildNode->pChildren[1], pOffset);
			}

			return currOffset;
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

		bool BVH2::Intersect(const Ray& ray, Intersection* pIsect) const
		{
			const IntSSE identity = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
			const IntSSE swap = _mm_set_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
			const IntSSE shuffleX = ray.mDir.x >= 0 ? identity : swap;
			const IntSSE shuffleY = ray.mDir.y >= 0 ? identity : swap;
			const IntSSE shuffleZ = ray.mDir.z >= 0 ? identity : swap;

			const IntSSE pn = IntSSE(0x00000000, 0x00000000, 0x80000000, 0x80000000);
			const Vec3f_SSE norg(-ray.mOrg.x, -ray.mOrg.y, -ray.mOrg.z);
			const Vec3f_SSE rdir = Vec3f_SSE(FloatSSE(1.0f / ray.mDir.x) ^ pn, FloatSSE(1.0f / ray.mDir.y) ^ pn, FloatSSE(1.0f / ray.mDir.z) ^ pn);
			FloatSSE nearFar(ray.mMin, ray.mMin, -ray.mMax, -ray.mMax);

			struct TravStackItem
			{
				float dist;
				int index;
			};
			TravStackItem TodoStack[64];
			uint stackTop = 0, nodeIdx = 0;

			bool hit = false;
			while (true)
			{
				const Node* pNode = &mpRoot[nodeIdx];

				// Interior node
				if (pNode->triangleCount == 0)
				{
					const FloatSSE tNearFarX = (SSE::Shuffle8(pNode->minMaxBoundsX, shuffleX) + norg.x) * rdir.x;
					const FloatSSE tNearFarY = (SSE::Shuffle8(pNode->minMaxBoundsY, shuffleY) + norg.y) * rdir.y;
					const FloatSSE tNearFarZ = (SSE::Shuffle8(pNode->minMaxBoundsZ, shuffleZ) + norg.z) * rdir.z;
					const FloatSSE tNearFar = SSE::Max(SSE::Max(tNearFarX, tNearFarY), SSE::Max(tNearFarZ, nearFar)) ^ pn;
					const BoolSSE lrhit = tNearFar <= SSE::Shuffle8(tNearFar, swap);

					if (lrhit[0] != 0 && lrhit[1] != 0)
					{
						if (tNearFar[0] < tNearFar[1]) // First child first
						{
							TodoStack[stackTop].index = pNode->secondChildOffset;
							TodoStack[stackTop].dist = tNearFar[1];
							stackTop++;
							nodeIdx = nodeIdx + 1;
						}
						else
						{
							TodoStack[stackTop].index = nodeIdx + 1;
							TodoStack[stackTop].dist = tNearFar[0];
							stackTop++;
							nodeIdx = pNode->secondChildOffset;
						}
					}
					else if (lrhit[0] != 0)
					{
						nodeIdx = nodeIdx + 1;
					}
					else if (lrhit[1] != 0)
					{
						nodeIdx = pNode->secondChildOffset;
					}
					else // If miss the node's bounds
					{
						do
						{
							if (stackTop == 0)
								return hit;
							stackTop--;
							nodeIdx = TodoStack[stackTop].index;
						} while (TodoStack[stackTop].dist > pIsect->mDist);
					}
				}
				else // Leaf node
				{
					Triangle4Node* pLeafNode = (Triangle4Node*)pNode;
					for (auto i = 0; i < pLeafNode->triangleCount; i++)
					{
						if (pLeafNode[i].tri4.Intersect(ray, pIsect))
							hit = true;
					}
					nearFar = SSE::Shuffle<0, 1, 2, 3>(nearFar, -pIsect->mDist);

					do
					{
						if (stackTop == 0)
							return hit;
						stackTop--;
						nodeIdx = TodoStack[stackTop].index;
					} while (TodoStack[stackTop].dist > pIsect->mDist);
				}
			}

			return hit;
		}

		bool BVH2::Occluded(const Ray& ray) const
		{
			return false;
		}
	}
}