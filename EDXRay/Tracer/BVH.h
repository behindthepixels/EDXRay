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
			float x, y, z;
			int pad;

			BuildVertex(float _x = 0, float _y = 0, float _z = 0)
				: x(_x)
				, y(_y)
				, z(_z)
			{
			}
		};

		class BVH2
		{
		public:
			struct Node
			{

			};

		private:
			BuildVertex*	mpVertices;
			BuildTriangle*	mpTriangleIndices;
			int mBuildVertexCount;
			int mBuildTriangleCount;

			BoundingBox mBounds;

		public:
			BVH2()
				: mpVertices(nullptr)
				, mpTriangleIndices(nullptr)
				, mBuildVertexCount(0)
				, mBuildTriangleCount(0)
			{
			}
			~BVH2()
			{
				Destroy();
			}

			void Build(const vector<RefPtr<Primitive>>& prims);

		private:
			void ExtractGeometry(const vector<RefPtr<Primitive>>& prims);

			void Destroy()
			{
				SafeDeleteArray(mpVertices);
				SafeDeleteArray(mpTriangleIndices);
			}
		};
	}
}