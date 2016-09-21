#pragma once

#include "EDXPrerequisites.h"
#include "Renderer.h"

#include "Windows/Threading.h"

#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		class RenderTask
		{
		public:
			RenderTask(Renderer* pRenderer)
				: mpRenderer(pRenderer)
			{
				
			}

			__forceinline void Render(int idx)
			{
				mpRenderer->RenderImage(idx, mRandom, mMemory);
			}

			static void _Render(RenderTask* pThis, int idx)
			{
				pThis->Render(idx);
			}

		public:
			Renderer*	mpRenderer;

		private:
			MemoryPool mMemory;
			RandomGen	mRandom;
		};

		class QueuedRenderTask : public QueuedWork
		{
		public:
			QueuedRenderTask(Renderer* pRenderer, const int idx)
				: mpRenderer(pRenderer)
				, mIndex(idx)
			{
			}

			void DoThreadedWork()
			{
				mpRenderer->RenderImage(mIndex, mRandom, mMemory);
			}

			void Abandon()
			{

			}

		private:
			Renderer*	mpRenderer;
			int			mIndex;

			MemoryPool	mMemory;
			RandomGen	mRandom;
		};
	}
}
