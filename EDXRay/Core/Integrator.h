#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"
#include "Math/Vector.h"


namespace EDX
{
	namespace RayTracer
	{
		class Integrator
		{
		public:
			virtual Color Li(const RayDifferential& ray, const Scene* pScene, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory) const = 0;
			virtual void RequestSamples(const Scene* pScene, SampleBuffer* pSampleBuf) = 0;
			virtual ~Integrator() {}

		public:
			static Color EstimateDirectLighting(const DifferentialGeom& diffGeom, const Vector3& outVec, const Light* pLight,
				const Scene* pScene, const Sample& lightSample, const Sample& bsdfSample);
			static Color SpecularReflect(const Integrator* pIntegrator, const Scene* pScene, const RayDifferential& ray,
				const DifferentialGeom& diffGeom, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory);
			static Color SpecularTransmit(const Integrator* pIntegrator, const Scene* pScene, const RayDifferential& ray,
				const DifferentialGeom& diffGeom, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory);
		};
	}
}