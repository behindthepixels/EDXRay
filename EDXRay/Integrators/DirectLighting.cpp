#include "DirectLighting.h"
#include "../Core/Scene.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/Sampler.h"
#include "Math/Ray.h"
#include "Graphics/Color.h"
#include "Memory/Memory.h"

namespace EDX
{
	namespace RayTracer
	{
		Color DirectLightingIntegrator::Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryArena& memory) const
		{
			DifferentialGeom diffGeom;
			Color L;
			if (pScene->Intersect(ray, &diffGeom))
			{
				pScene->PostIntersect(ray, &diffGeom);

				auto numLights = pScene->GetLights().size();

				for (auto i = 0; i < pScene->GetLights().size(); i++)
				{
					auto pLight = pScene->GetLights()[i].Ptr();
					Sample lightSample = pSampler->GetSample();
					Sample bsdfSample = pSampler->GetSample();
					L += Integrator::EstimateDirectLighting(diffGeom, -ray.mDir, pLight, pScene, lightSample, bsdfSample);
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

		void DirectLightingIntegrator::RequestSamples(const Scene* pScene, SampleBuffer* pSampleBuf)
		{
			assert(pSampleBuf);

			auto numLights = pScene->GetLights().size();
			mpLightSampleOffsets = new SampleOffsets[numLights];
			mpBSDFSampleOffsets = new SampleOffsets[numLights];

			for (auto i = 0; i < numLights; i++)
			{
				mpLightSampleOffsets[i] = SampleOffsets(pScene->GetLights()[i]->GetSampleCount(), pSampleBuf);
				mpBSDFSampleOffsets[i] = SampleOffsets(pScene->GetLights()[i]->GetSampleCount(), pSampleBuf);
			}
		}

		DirectLightingIntegrator::~DirectLightingIntegrator()
		{
			SafeDeleteArray(mpLightSampleOffsets);
			SafeDeleteArray(mpBSDFSampleOffsets);
		}
	}
}