#include "DirectLighting.h"
#include "../Core/Scene.h"
#include "../Core/DifferentialGeom.h"
#include "Math/Ray.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		Color DirectLightingIntegrator::Li(const RayDifferential& ray, const Scene* pScene, const Sample* pSamples, RandomGen& random, MemoryArena& memory) const
		{
			DifferentialGeom diffGeom;
			Color L;
			if (pScene->Intersect(ray, &diffGeom))
			{
				pScene->PostIntersect(ray, &diffGeom);
				L = Color(diffGeom.mNormal);
			}

			return L;
		}
	}
}