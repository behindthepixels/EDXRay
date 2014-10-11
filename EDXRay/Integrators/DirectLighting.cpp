#include "DirectLighting.h"
#include "../Core/Scene.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/Sampler.h"
#include "Math/Ray.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		Color DirectLightingIntegrator::Li(const RayDifferential& ray, const Scene* pScene, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory) const
		{
			DifferentialGeom diffGeom;
			Color L;
			if (pScene->Intersect(ray, &diffGeom))
			{
				pScene->PostIntersect(ray, &diffGeom);
				diffGeom.ComputeDifferentials(ray);

				for (const auto& pLight : pScene->GetLight())
				{
					L += Integrator::EstimateDirectLighting(diffGeom, -ray.mDir, pLight.Ptr(), pScene, Sample(random), Sample(random));
				}

				if (ray.mDepth < mMaxDepth)
				{
					L += Integrator::SpecularReflect(this, pScene, ray, diffGeom, pSamples, random, memory);
					L += Integrator::SpecularTransmit(this, pScene, ray, diffGeom, pSamples, random, memory);
				}
			}

			return L;
		}
	}
}