#pragma once

#include "EDXPrerequisites.h"
#include "Memory/RefPtr.h"
#include "Memory/Memory.h"
#include "Math/BoundingBox.h"
#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		struct BuildTriangle
		{
			int idx1, idx2, idx3;
			int meshIdx, triIdx;

			BuildTriangle(int i1 = 0, int i2 = 0, int i3 = 0, int m = 0, int t = 0)
				: idx1(i1)
				, idx2(i2)
				, idx3(i3)
				, meshIdx(m)
				, triIdx(t)
			{
			}
		};

		struct BuildVertex
		{
			Vector3 pos;
			int pad;

			BuildVertex(float x = 0, float y = 0, float z = 0)
				: pos(x, y, z)
			{
			}
		};

		struct TriangleInfo
		{
			int idx;
			Vector3 centroid;
			BoundingBox bbox;

			TriangleInfo(int _idx = 0, const BoundingBox& box = BoundingBox())
				: idx(idx)
				, bbox(box)
			{
				centroid = bbox.Centroid();
			}
		};

		class BVH2
		{
		public:
			struct Node
			{
				int primOffset;
				int primCount;

				BoundingBox leftBounds;
				BoundingBox rightBounds;
				Node* pChilds[2];

				void InitLeaf(int offset, int count, const BoundingBox& bounds)
				{
					primOffset = offset;
					primCount = count;
					leftBounds = bounds;
					pChilds[0] = pChilds[1] = nullptr;
				}
				void InitInterior(const BoundingBox& lBounds, const BoundingBox& rBounds)
				{
					leftBounds = lBounds;
					rightBounds = rBounds;
				}
			};

		private:
			BuildVertex*	mpBuildVertices;
			BuildTriangle*	mpBuildIndices;
			int mBuildVertexCount;
			int mBuildTriangleCount;

			BoundingBox mBounds;

			const int MaxDepth;

		public:
			BVH2()
				: mpBuildVertices(nullptr)
				, mpBuildIndices(nullptr)
				, mBuildVertexCount(0)
				, mBuildTriangleCount(0)
				, MaxDepth(128)
			{
			}
			~BVH2()
			{
				Destroy();
			}

			void	Construct(const vector<RefPtr<Primitive>>& prims);
			Node*	RecursiveBuildNode(vector<TriangleInfo>& buildInfo,
				const int startIdx,
				const int endIdx,
				const int depth,
				MemoryArena& memory);

		private:
			void ExtractGeometry(const vector<RefPtr<Primitive>>& prims);

			void Destroy()
			{
				SafeDeleteArray(mpBuildVertices);
				SafeDeleteArray(mpBuildIndices);
			}
		};
	}
}