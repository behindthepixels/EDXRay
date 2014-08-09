#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"


namespace EDX
{
	namespace RayTracer
	{
		class Integrator
		{
		public:
			virtual Color Li(const RayDifferential& ray, const Scene* pScene, const Sample* pSamples, RandomGen& random, MemoryArena& memory) const = 0;
		};
	}
}