#pragma once

#include "EDXPrerequisites.h"
#include "Renderer.h"

#include "Windows/Thread.h"
#include "Memory/RefPtr.h"
#include "Memory/Memory.h"
#include "RNG/Random.h"

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

			void Render(int idx)
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
			MemoryArena mMemory;
			RandomGen	mRandom;
		};
	}
}
