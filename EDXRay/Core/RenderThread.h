#pragma once

#include "EDXPrerequisites.h"
#include "Windows/Thread.h"
#include "Memory/Memory.h"
#include "RNG/Random.h"

#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		class RenderThread : public EDXThread
		{
		public:
			void WorkLoop();
			void SetRenderer(Renderer* pRenderer)
			{
				mpRenderer = pRenderer;
			}

		private:
			Renderer*	mpRenderer;
			MemoryArena mMemory;
			RandomGen	mRendom;
		};
	}
}
