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
			void Init(Renderer* pRenderer, uint id)
			{
				mpRenderer = pRenderer;
				mId = id;
			}

		private:
			uint		mId;
			Renderer*	mpRenderer;
			MemoryArena mMemory;
			RandomGen	mRendom;
		};
	}
}
