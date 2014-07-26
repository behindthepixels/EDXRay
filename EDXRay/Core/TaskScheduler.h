#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"
#include "Windows/Thread.h"

namespace EDX
{
	namespace RayTracer
	{
		struct RenderTask
		{
			int minX, minY, maxX, maxY;
			static const int TILE_SIZE = 32;

			RenderTask(int _minX, int _minY, int _maxX, int _maxY)
				: minX(_minX)
				, minY(_minY)
				, maxX(_maxX)
				, maxY(_maxY)
			{
			}
		};

		class TaskScheduler
		{
		private:
			vector<RenderTask> mTasks;
			uint mCurrentTaskIdx;
			EDXLock mLock;

			vector<HANDLE>	mFinishEvent;

		public:
			void Init(const int x, const int y)
			{
				mTasks.clear();
				mCurrentTaskIdx = 0;

				for (int i = 0; i < y; i += RenderTask::TILE_SIZE)
				{
					for (int j = 0; j < x; j += RenderTask::TILE_SIZE)
					{
						int minX = j, minY = i;
						int maxX = j + RenderTask::TILE_SIZE, maxY = i + RenderTask::TILE_SIZE;
						maxX = maxX <= x ? maxX : x;
						maxY = maxY <= y ? maxY : y;

						mTasks.push_back(RenderTask(minX, minY, maxX, maxY));
					}
				}

				mFinishEvent.resize(DetectCPUCount());
				for (auto& it : mFinishEvent)
				{
					it = CreateEvent(NULL, TRUE, FALSE, NULL);
				}
			}

			bool GetNextTask(RenderTask*& pTask)
			{
				EDXLockApply cs(mLock);

				if (mCurrentTaskIdx < mTasks.size())
					pTask = &mTasks[mCurrentTaskIdx];
				else
					pTask = nullptr;

				mCurrentTaskIdx++;

				return pTask != nullptr;
			}

			void ResetTaskIdx()
			{
				mCurrentTaskIdx = 0;
			}
		};
	}
}