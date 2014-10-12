#include "BVH.h"
#include "BVHBuildTask.h"
#include "../Core/Primitive.h"
#include "../Core/TriangleMesh.h"
#include "../Core/DifferentialGeom.h"
#include "Triangle4.h"

#include "Memory/Memory.h"

namespace EDX
{
	namespace RayTracer
	{
		void BVH2::Construct(const vector<RefPtr<Primitive>>& prims)
		{
			mpRefPrims = &const_cast<vector<RefPtr<Primitive>>&>(prims);
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

			RecursiveBuildNode(pBuildRoot, buildInfo, 0, mBuildTriangleCount, 0, memory);
			ThreadScheduler::Instance()->JoinAllTasks();

			mpRoot = AllocAligned<Node>(mTreeBufSize, 64);
			memset(mpRoot, 0, mTreeBufSize * sizeof(Node));

			uint offset = 0;
			LinearizeNodes(pBuildRoot, &offset);
			assert(offset == mTreeBufSize);
		}

		void BVH2::RecursiveBuildNode(BuildNode* pNode,
			vector<TriangleInfo>& buildInfo,
			const int startIdx,
			const int endIdx,
			const int depth,
			MemoryArena& memory)
		{
			*pNode = BuildNode();
			auto numTriangles = endIdx - startIdx;
			uint packedCount = (numTriangles + 3) / 4;

			auto CreateLeaf = [&]
			{
				uint infoIdx = startIdx;

				mMemLock.Lock();
				Triangle4* pTri4 = memory.Alloc<Triangle4>(packedCount);
				mMemLock.Unlock();
				for (auto i = 0; i < packedCount; i++)
				{
					BuildVertex triangles[4][3];
					BuildTriangle indices[4];
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

					pTri4[i] = Triangle4();
					pTri4[i].Pack(triangles, indices, count);
				}

				pNode->InitLeaf(pTri4, packedCount);
				mTreeBufSize += 4 * packedCount;
			};

			if (numTriangles <= 4 || depth > MaxDepth) // Create a leaf if triangle count is 1 or depth exceeds maximum
			{
				CreateLeaf();
				return;
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

				//int mid = 0;
				//auto EqualCountSplit = [&]
				//{
				//	mid = (startIdx + endIdx) / 2;
				//	std::nth_element(&buildInfo[startIdx], &buildInfo[mid], &buildInfo[endIdx - 1] + 1,
				//		[dim](const TriangleInfo& lhs, const TriangleInfo& rhs)
				//	{
				//		return lhs.centroid[dim] < rhs.centroid[dim];
				//	});
				//};

				//EqualCountSplit();

				// Partition primitives
				// Binning
				const int MaxBins = 32;
				const float TraversalCost = 1.0f;
				const float IntersectCost = 2.0f;
				BoundingBox binBounds[MaxBins][3];
				IntSSE counts[MaxBins];

				const int numBins = Math::Min(MaxBins, int(4.0f + 0.05f * numTriangles));
				for (auto i = startIdx; i < endIdx; i++)
				{
					Vector3i binIdx = Vector3i(centroidBounds.Offset(buildInfo[i].centroid) * numBins);
					binIdx.x = Math::Clamp(binIdx.x, 0, numBins - 1);
					binIdx.y = Math::Clamp(binIdx.y, 0, numBins - 1);
					binIdx.z = Math::Clamp(binIdx.z, 0, numBins - 1);

					counts[binIdx.x][0]++;
					counts[binIdx.y][1]++;
					counts[binIdx.z][2]++;
					binBounds[binIdx.x][0] = Math::Union(binBounds[binIdx.x][0], buildInfo[i].bbox);
					binBounds[binIdx.y][1] = Math::Union(binBounds[binIdx.y][1], buildInfo[i].bbox);
					binBounds[binIdx.z][2] = Math::Union(binBounds[binIdx.z][2], buildInfo[i].bbox);
				}

				// Sweep from right to left to compute prefix sum of bounds and geometry count
				IntSSE rightCountSum[MaxBins];
				FloatSSE rightAreaSum[MaxBins];
				BoundingBox rightBoundsSum[MaxBins][3];
				IntSSE currCount;
				BoundingBox currBounds[3];
				for (auto i = numBins - 1; i >= 0; i--)
				{
					currCount += counts[i];
					rightCountSum[i] = (currCount + IntSSE(3)) >> 2;

					currBounds[0] = Math::Union(currBounds[0], binBounds[i][0]);
					currBounds[1] = Math::Union(currBounds[1], binBounds[i][1]);
					currBounds[2] = Math::Union(currBounds[2], binBounds[i][2]);
					rightBoundsSum[i][0] = currBounds[0];
					rightBoundsSum[i][1] = currBounds[1];
					rightBoundsSum[i][2] = currBounds[2];
					rightAreaSum[i][0] = rightBoundsSum[i][0].Area();
					rightAreaSum[i][1] = rightBoundsSum[i][1].Area();
					rightAreaSum[i][2] = rightBoundsSum[i][2].Area();
				}

				// Sweep from left to right and calculate SAH
				currCount = IntSSE(Math::EDX_ZERO);
				currBounds[0] = BoundingBox(); currBounds[1] = BoundingBox(); currBounds[2] = BoundingBox();
				FloatSSE bestCost = FloatSSE(Math::EDX_INFINITY);
				IntSSE bestPos;
				const float totalArea = rightBoundsSum[0][0].Area();
				for (auto i = 1; i < numBins; i++)
				{
					auto j = i - 1;

					currCount += counts[j];
					const IntSSE leftCount = (currCount + IntSSE(3)) >> 2;;

					currBounds[0] = Math::Union(currBounds[0], binBounds[j][0]);
					currBounds[1] = Math::Union(currBounds[1], binBounds[j][1]);
					currBounds[2] = Math::Union(currBounds[2], binBounds[j][2]);
					const FloatSSE leftArea = FloatSSE(currBounds[0].Area(), currBounds[1].Area(), currBounds[2].Area(), currBounds[2].Area());

					const FloatSSE cost = FloatSSE(TraversalCost) + FloatSSE(IntersectCost) * ((FloatSSE(leftCount) * leftArea) + (FloatSSE(rightCountSum[i]) * rightAreaSum[i])) / totalArea;

					bestPos = SSE::Select(cost < bestCost, i, bestPos);
					bestCost = SSE::Select(cost < bestCost, cost, bestCost);
				}

				int bestSplitDim;
				int bestSplitPos;
				float bestSplitCost = Math::EDX_INFINITY;
				for (auto i = 0; i < 3; i++)
				{
					if (bestCost[i] <= bestSplitCost)
					{
						bestSplitDim = i;
						bestSplitCost = bestCost[i];
						bestSplitPos = bestPos[i];
					}
				}

				int mid = 0;
				if (bestSplitCost < IntersectCost * packedCount || packedCount > 6)
				{
					TriangleInfo* pMid = std::partition(&buildInfo[startIdx], &buildInfo[endIdx - 1] + 1,
						[&](const TriangleInfo& par) -> bool
					{
						int binIdx = numBins * (par.centroid[bestSplitDim] - centroidBounds.mMin[bestSplitDim]) /
							(centroidBounds.mMax[bestSplitDim] - centroidBounds.mMin[bestSplitDim]);

						binIdx = Math::Clamp(binIdx, 0, numBins - 1);
						return binIdx < bestSplitPos;
					});

					mid = pMid - &buildInfo[0];
				}
				else
				{
					CreateLeaf();
					return;
				}

				// Compute left and right bounds
				BoundingBox leftBounds, rightBounds;
				for (auto i = startIdx; i < mid; i++)
					leftBounds = Math::Union(leftBounds, buildInfo[i].bbox);
				for (auto i = mid; i < endIdx; i++)
					rightBounds = Math::Union(rightBounds, buildInfo[i].bbox);

				mMemLock.Lock();
				BuildNode* pLeft = memory.Alloc<BuildNode>();
				BuildNode* pRight = memory.Alloc<BuildNode>();
				mMemLock.Unlock();

				pNode->InitInterior(leftBounds, rightBounds, pLeft, pRight);

				if (numTriangles < 4096)
				{
					RecursiveBuildNode(pLeft, buildInfo, startIdx, mid, depth + 1, memory);
					RecursiveBuildNode(pRight, buildInfo, mid, endIdx, depth + 1, memory);
				}
				else
				{
					BuildTask* pTaskLeft = new BuildTask(this, pLeft, buildInfo, startIdx, mid, depth + 1, memory);
					BuildTask* pTaskRight = new BuildTask(this, pRight, buildInfo, mid, endIdx, depth + 1, memory);
					{
						EDXLockApply lock(mTaskLock);
						mBuildTasks.push_back(pTaskLeft);
						mBuildTasks.push_back(pTaskRight);
					}
					ThreadScheduler::Instance()->AddTasks(Task((Task::TaskFunc)&BuildTask::_Run, pTaskLeft));
					ThreadScheduler::Instance()->AddTasks(Task((Task::TaskFunc)&BuildTask::_Run, pTaskRight));
				}

				mTreeBufSize += 1;
			}

			return;
		}

		uint BVH2::LinearizeNodes(const BuildNode* pBuildNode, uint* pOffset)
		{
			Node* pNode = &mpRoot[*pOffset];
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
				const uint* pIndices = pMesh->GetIndexAt(0);
				for (auto j = 0; j < pMesh->GetTriangleCount(); j++)
				{
					mpBuildIndices[mBuildTriangleCount++] = BuildTriangle(mBuildVertexCount + pIndices[3 * j],
						mBuildVertexCount + pIndices[3 * j + 1],
						mBuildVertexCount + pIndices[3 * j + 2],
						i,
						j,
						prims[i]->GetBSDF(j)->GetTexture()->HasAlpha());
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
						if (pLeafNode[i].tri4.Intersect(ray, pIsect, mpRefPrims))
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
			const IntSSE identity = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
			const IntSSE swap = _mm_set_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
			const IntSSE shuffleX = ray.mDir.x >= 0 ? identity : swap;
			const IntSSE shuffleY = ray.mDir.y >= 0 ? identity : swap;
			const IntSSE shuffleZ = ray.mDir.z >= 0 ? identity : swap;

			const IntSSE pn = IntSSE(0x00000000, 0x00000000, 0x80000000, 0x80000000);
			const Vec3f_SSE norg(-ray.mOrg.x, -ray.mOrg.y, -ray.mOrg.z);
			const Vec3f_SSE rdir = Vec3f_SSE(FloatSSE(1.0f / ray.mDir.x) ^ pn, FloatSSE(1.0f / ray.mDir.y) ^ pn, FloatSSE(1.0f / ray.mDir.z) ^ pn);
			FloatSSE nearFar(ray.mMin, ray.mMin, -ray.mMax, -ray.mMax);

			int TodoStack[64];
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
							TodoStack[stackTop++] = pNode->secondChildOffset;
							nodeIdx = nodeIdx + 1;
						}
						else
						{
							TodoStack[stackTop++] = nodeIdx + 1;
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
						if (stackTop == 0)
							return false;

						nodeIdx = TodoStack[--stackTop];
					}
				}
				else // Leaf node
				{
					Triangle4Node* pLeafNode = (Triangle4Node*)pNode;
					for (auto i = 0; i < pLeafNode->triangleCount; i++)
					{
						if (pLeafNode[i].tri4.Occluded(ray, mpRefPrims))
							return true;
					}

					if (stackTop == 0)
						return false;

					nodeIdx = TodoStack[--stackTop];
				}
			}

			return false;
		}
	}
}