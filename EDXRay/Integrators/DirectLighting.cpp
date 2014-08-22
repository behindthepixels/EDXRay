#include "DirectLighting.h"
#include "../Core/Scene.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/BSDF.h"
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

				const Vector3 eyeDir = -ray.mDir;
				const Vector3& normal = diffGeom.mNormal;
				for (auto& it : pScene->GetLight())
				{
					Sample sample;
					Vector3 lightDir;
					VisibilityTester visibility;
					float lightPdf;
					const BSDF* pBSDF = diffGeom.mpBSDF;
					Color Li = it->Illuminate(diffGeom.mPosition, sample, &lightDir, &visibility, &lightPdf);
					if (lightPdf > 0.0f && !Li.IsBlack())
					{
						Color f = pBSDF->Eval(eyeDir, lightDir, diffGeom, ScatterType(BSDF_ALL & ~BSDF_SPECULAR));
						if (!f.IsBlack() && visibility.Unoccluded(pScene))
						{
							L += f * Li * Math::AbsDot(lightDir, normal) / lightPdf;
						}
					}
				}
			}

			return L;
		}
	}
}