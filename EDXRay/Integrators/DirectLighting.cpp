#include "DirectLighting.h"
#include "../Core/Scene.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/Sampler.h"
#include "../Core/Ray.h"
#include "Graphics/Color.h"
#include "Core/Memory.h"

namespace EDX
{
	namespace RayTracer
	{
		Color DirectLightingIntegrator::Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryPool& memory) const
		{
			DifferentialGeom diffGeom;
			Color L;
			if (pScene->Intersect(ray, &diffGeom))
			{
				pScene->PostIntersect(ray, &diffGeom);

				auto numLights = pScene->GetLights().Size();

				for (auto i = 0; i < pScene->GetLights().Size(); i++)
				{
					auto pLight = pScene->GetLights()[i].Get();
					L += Integrator::EstimateDirectLighting(diffGeom, -ray.mDir, pLight, pScene, pSampler);
				}

				if (ray.mDepth < mMaxDepth)
				{
					L += Integrator::SpecularReflect(this, pScene, pSampler, ray, diffGeom, random, memory);
					L += Integrator::SpecularTransmit(this, pScene, pSampler, ray, diffGeom, random, memory);
				}
			}
			else
			{
				if (auto envMap = pScene->GetEnvironmentMap())
					L += envMap->Emit(-ray.mDir);
			}

			return L;
		}

		DirectLightingIntegrator::~DirectLightingIntegrator()
		{
		}
	}
}