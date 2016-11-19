#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"
#include "Windows/Threading.h"

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
			Array<RenderTile> mTiles;
			uint mCurrentTileIdx;
			CriticalSection mLock;

			uint mThreadCount;
			AtomicCounter mPreRenderSyncedCount, mPostRenderSyncedCount;
			Array<HANDLE>	mPreRenderEvent;
			Array<HANDLE>	mPostRenderEvent;
			bool mAllTaskFinished;
			bool mAbort;

		public:
			void Init(const int x, const int y)
			{
				mTiles.Clear();
				mCurrentTileIdx = 0;

				for (int i = 0; i < y; i += RenderTile::TILE_SIZE)
				{
					for (int j = 0; j < x; j += RenderTile::TILE_SIZE)
					{
						int minX = j, minY = i;
						int maxX = j + RenderTile::TILE_SIZE, maxY = i + RenderTile::TILE_SIZE;
						maxX = maxX <= x ? maxX : x;
						maxY = maxY <= y ? maxY : y;

						mTiles.Emplace(minX, minY, maxX, maxY);
					}
				}

				mThreadCount = GetNumberOfCores();
				mPreRenderEvent.Resize(mThreadCount);
				mPostRenderEvent.Resize(mThreadCount);
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
				ScopeLock cs(&mLock);

				if (mCurrentTileIdx < mTiles.Size())
					pTask = &mTiles[mCurrentTileIdx];
				else
					return false;

				mCurrentTileIdx++;

				return true;
			}

			void SyncThreadsPreRender(int threadId)
			{
				SetEvent(mPreRenderEvent[threadId]);
				while (WaitForMultipleObjects(mThreadCount, mPreRenderEvent.Data(), true, 20) == WAIT_TIMEOUT)
				{
					if (mAbort)
						return;
				}

				mPreRenderSyncedCount.Increment();

				if (mPreRenderSyncedCount.GetValue() == mThreadCount)
				{
					mPreRenderSyncedCount.Set(0);
					for (auto& it : mPreRenderEvent)
					{
						ResetEvent(it);
					}
				}
			}

			void SyncThreadsPostRender(int threadId)
			{
				SetEvent(mPostRenderEvent[threadId]);
				while (WaitForMultipleObjects(mThreadCount, mPostRenderEvent.Data(), true, 20) == WAIT_TIMEOUT)
				{
					if (mAbort)
						return;
				}

				mPostRenderSyncedCount.Increment();

				if (mPostRenderSyncedCount.GetValue() == mThreadCount)
				{
					mPostRenderSyncedCount.Set(0);
					for (auto& it : mPostRenderEvent)
					{
						ResetEvent(it);
					}
				}
			}

			const RenderTile& GetTile(const int index) const
			{
				Assert(index < mTiles.Size());
				return mTiles[index];
			}

			int GetNumTiles() const
			{
				return mTiles.Size();
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