#pragma once

#include "EDXPrerequisites.h"
#include "TaskSynchronizer.h"

#include "../ForwardDecl.h"
#include "Math/Vector.h"
#include "BSDF.h"


namespace EDX
{
	namespace RayTracer
	{
		class Integrator
		{
		protected:
			const RenderJobDesc& mJobDesc;

			// Synchronization managing object
			const TaskSynchronizer& mTaskSync;

		public:
			Integrator(const RenderJobDesc& jobDesc, const TaskSynchronizer& taskSync)
				: mJobDesc(jobDesc)
				, mTaskSync(taskSync)
			{
			}

			virtual void Render(const Scene* pScene, const Camera* pCamera, Sampler* pSampler, Film* pFilm) const = 0;
			virtual ~Integrator() {}

		public:
			static Color EstimateDirectLighting(const Scatter& scatter, const Vector3& outVec, const Light* pLight,
				const Scene* pScene, Sampler* pSampler, ScatterType scatterType = ScatterType(BSDF_ALL & ~BSDF_SPECULAR));
			static Color SpecularReflect(const TiledIntegrator* pIntegrator, const Scene* pScene, Sampler* pSampler, const RayDifferential& ray,
				const DifferentialGeom& diffGeom, RandomGen& random, MemoryPool& memory);
			static Color SpecularTransmit(const TiledIntegrator* pIntegrator, const Scene* pScene, Sampler* pSampler, const RayDifferential& ray,
				const DifferentialGeom& diffGeom, RandomGen& random, MemoryPool& memory);
		};

		class TiledIntegrator : public Integrator
		{
		protected:

		public:
			TiledIntegrator(const RenderJobDesc& jobDesc, const TaskSynchronizer& taskSync)
				: Integrator(jobDesc, taskSync)
			{
			}

			virtual void Render(const Scene* pScene, const Camera* pCamera, Sampler* pSampler, Film* pFilm) const override;
			virtual Color Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryPool& memory) const = 0;
			virtual ~TiledIntegrator() {}
		};
	}
}