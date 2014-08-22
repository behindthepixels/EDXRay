#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Integrator.h"


namespace EDX
{
	namespace RayTracer
	{
		class DirectLightingIntegrator : public Integrator
		{
		private:
			uint mMaxDepth;

		public:
			Color Li(const RayDifferential& ray, const Scene* pScene, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory) const;
		};
	}
}