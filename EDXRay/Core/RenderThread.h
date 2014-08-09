#pragma once

#include "EDXPrerequisites.h"
#include "Windows/Thread.h"
#include "Memory/RefPtr.h"
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
			void Init(Renderer* pRenderer, uint id);

		private:
			Renderer*	mpRenderer;
			uint		mId;
			MemoryArena mMemory;
			RandomGen	mRandom;
		};
	}
}
