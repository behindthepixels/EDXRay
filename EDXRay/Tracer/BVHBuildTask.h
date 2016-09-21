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
				Array<TriangleInfo>& info,
				int start,
				int end,
				int depth,
				MemoryPool& memory)
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
			Array<TriangleInfo>& mBuildInfo;
			int mStart;
			int mEnd;
			int mDepth;
			MemoryPool& mMemory;
		};

		class QueuedBuildTask : public QueuedWork
		{
		public:
			QueuedBuildTask(BVH2* pBvh,
				BVH2::BuildNode* pNode,
				Array<TriangleInfo>& info,
				int start,
				int end,
				int depth,
				MemoryPool& memory)
				: mpBvh(pBvh)
				, mpNode(pNode)
				, mBuildInfo(info)
				, mStart(start)
				, mEnd(end)
				, mDepth(depth)
				, mMemory(memory)
			{
			}

			void DoThreadedWork()
			{
				mpBvh->RecursiveBuildNode(mpNode, mBuildInfo, mStart, mEnd, mDepth, mMemory);
			}

			void Abandon()
			{

			}

		private:
			BVH2* mpBvh;
			BVH2::BuildNode* mpNode;
			Array<TriangleInfo>& mBuildInfo;
			int mStart;
			int mEnd;
			int mDepth;
			MemoryPool& mMemory;
		};
	}
}
