#pragma once

#include "EDXPrerequisites.h"

#include "BVH.h"
#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		class BuildTask
		{
		public:
			BuildTask(BVH2* pBvh,
				BVH2::BuildNode* pNode,
				vector<TriangleInfo>& info,
				int start,
				int end,
				int depth,
				MemoryArena& memory)
				: mpBvh(pBvh)
				, mpNode(pNode)
				, mBuildInfo(info)
				, mStart(start)
				, mEnd(end)
				, mDepth(depth)
				, mMemory(memory)
			{
			}

			__forceinline void Run()
			{
				mpBvh->RecursiveBuildNode(mpNode, mBuildInfo, mStart, mEnd, mDepth, mMemory);
			}

			static void _Run(BuildTask* pThis, int idx)
			{
				pThis->Run();
			}

		private:
			BVH2* mpBvh;
			BVH2::BuildNode* mpNode;
			vector<TriangleInfo>& mBuildInfo;
			int mStart;
			int mEnd;
			int mDepth;
			MemoryArena& mMemory;
		};
	}
}
