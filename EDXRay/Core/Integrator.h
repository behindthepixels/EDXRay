#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"
#include "Math/Vector.h"
#include "BSDF.h"


namespace EDX
{
	namespace RayTracer
	{
		class Integrator
		{
		public:
			virtual Color Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryPool& memory) const = 0;
			virtual void RequestSamples(const Scene* pScene, SampleBuffer* pSampleBuf) = 0;
			virtual ~Integrator() {}

		public:
			static Color EstimateDirectLighting(const Scatter& scatter, const Vector3& outVec, const Light* pLight,
				const Scene* pScene, Sampler* pSampler, ScatterType scatterType = ScatterType(BSDF_ALL & ~BSDF_SPECULAR));
			static Color SpecularReflect(const Integrator* pIntegrator, const Scene* pScene, Sampler* pSampler, const RayDifferential& ray,
				const DifferentialGeom& diffGeom, RandomGen& random, MemoryPool& memory);
			static Color SpecularTransmit(const Integrator* pIntegrator, const Scene* pScene, Sampler* pSampler, const RayDifferential& ray,
				const DifferentialGeom& diffGeom, RandomGen& random, MemoryPool& memory);
		};
	}
}