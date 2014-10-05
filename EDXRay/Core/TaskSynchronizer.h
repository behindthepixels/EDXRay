#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"
#include "Windows/Thread.h"
#include <atomic>

namespace EDX
{
	namespace RayTracer
	{
		struct RenderTile
		{
			int minX, minY, maxX, maxY;
			static const int TILE_SIZE = 32;

			RenderTile(int _minX, int _minY, int _maxX, int _maxY)
				: minX(_minX)
				, minY(_minY)
				, maxX(_maxX)
				, maxY(_maxY)
			{
			}
		};

		class TaskSynchronizer
		{
		private:
			vector<RenderTile> mTiles;
			uint mCurrentTileIdx;
			EDXLock mLock;

			uint mThreadCount;
			std::atomic_int mPreRenderSyncedCount, mPostRenderSyncedCount;
			vector<HANDLE>	mPreRenderEvent;
			vector<HANDLE>	mPostRenderEvent;
			bool mAllTaskFinished;
			bool mAbort;

		public:
			void Init(const int x, const int y)
			{
				mTiles.clear();
				mCurrentTileIdx = 0;

				for (int i = 0; i < y; i += RenderTile::TILE_SIZE)
				{
					for (int j = 0; j < x; j += RenderTile::TILE_SIZE)
					{
						int minX = j, minY = i;
						int maxX = j + RenderTile::TILE_SIZE, maxY = i + RenderTile::TILE_SIZE;
						maxX = maxX <= x ? maxX : x;
						maxY = maxY <= y ? maxY : y;

						mTiles.push_back(RenderTile(minX, minY, maxX, maxY));
					}
				}

				mThreadCount = DetectCPUCount();
				mPreRenderEvent.resize(mThreadCount);
				mPostRenderEvent.resize(mThreadCount);
				for (auto& it : mPreRenderEvent)
				{
					it = CreateEvent(NULL, TRUE, FALSE, NULL);
				}
				for (auto& it : mPostRenderEvent)
				{
					it = CreateEvent(NULL, TRUE, FALSE, NULL);
				}

				mAllTaskFinished = false;
				mAbort = false;
			}

			bool GetNextTask(RenderTile*& pTask)
			{
				EDXLockApply cs(mLock);

				if (mCurrentTileIdx < mTiles.size())
					pTask = &mTiles[mCurrentTileIdx];
				else
					return false;

				mCurrentTileIdx++;

				return true;
			}

			void SyncThreadsPreRender(int threadId)
			{
				SetEvent(mPreRenderEvent[threadId]);
				while (WaitForMultipleObjects(mThreadCount, mPreRenderEvent.data(), true, 20) == WAIT_TIMEOUT)
				{
					if (mAbort)
						return;
				}

				mPreRenderSyncedCount++;

				if (mPreRenderSyncedCount == mThreadCount)
				{
					mPreRenderSyncedCount = 0;
					for (auto& it : mPreRenderEvent)
					{
						ResetEvent(it);
					}
				}
			}

			void SyncThreadsPostRender(int threadId)
			{
				SetEvent(mPostRenderEvent[threadId]);
				while (WaitForMultipleObjects(mThreadCount, mPostRenderEvent.data(), true, 20) == WAIT_TIMEOUT)
				{
					if (mAbort)
						return;
				}

				mPostRenderSyncedCount++;

				if (mPostRenderSyncedCount == mThreadCount)
				{
					mPostRenderSyncedCount = 0;
					for (auto& it : mPostRenderEvent)
					{
						ResetEvent(it);
					}
				}
			}

			void ResetTasks()
			{
				mCurrentTileIdx = 0;
				mAllTaskFinished = false;
			}

			void SetAbort(const bool ab)
			{
				mAbort = ab;
			}

			const bool Aborted() const
			{
				return mAbort;
			}
		};
	}
}