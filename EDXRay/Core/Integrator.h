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
			virtual Color Li(const RayDifferential& ray, const Scene* pScene, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory) const = 0;

		public:
			static Color SpecularReflect(const Integrator* pIntegrator, const Scene* pScene, const RayDifferential& ray,
				const DifferentialGeom& diffGeom, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory);
			static Color SpecularTransmit(const Integrator* pIntegrator, const Scene* pScene, const RayDifferential& ray,
				const DifferentialGeom& diffGeom, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory);
		};
	}
}