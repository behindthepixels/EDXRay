#include "DirectLighting.h"
#include "../Core/Scene.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/BSDF.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
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
				const Vector3& position = diffGeom.mPosition;
				const Vector3& normal = diffGeom.mNormal;
				const BSDF* pBSDF = diffGeom.mpBSDF;

				for (const auto& pLight : pScene->GetLight())
				{
					Sample sample;

					// Sample light sources
					Vector3 lightDir;
					VisibilityTester visibility;
					float lightPdf;
					const Color Li = pLight->Illuminate(position, sample, &lightDir, &visibility, &lightPdf);
					if (lightPdf > 0.0f && !Li.IsBlack())
					{
						const Color f = pBSDF->Eval(eyeDir, lightDir, diffGeom, ScatterType(BSDF_ALL & ~BSDF_SPECULAR));
						if (!f.IsBlack() && visibility.Unoccluded(pScene))
						{
							if (pLight->IsDelta())
							{
								L += f * Li * Math::AbsDot(lightDir, normal) / lightPdf;
								continue;
							}

							const float bsdfPdf = pBSDF->PDF(eyeDir, lightDir, diffGeom);
							const float misWeight = Sampling::PowerHeuristic(1, lightPdf, 1, bsdfPdf);
							L += f * Li * Math::AbsDot(lightDir, normal) * misWeight / lightPdf;
						}
					}

					// Sample BSDF for MIS
					ScatterType types;
					float bsdfPdf;
					const Color f = pBSDF->SampleScattered(eyeDir, sample, diffGeom, &lightDir, &bsdfPdf, ScatterType(BSDF_ALL & ~BSDF_SPECULAR), &types);
					if (bsdfPdf > 0.0f && !f.IsBlack())
					{
						lightPdf = pLight->Pdf(position, lightDir);
						if (lightPdf > 0.0f)
						{
							float misWeight = Sampling::PowerHeuristic(1, lightPdf, 1, bsdfPdf);

							DifferentialGeom isect;
							Ray rayLight = Ray(position, lightDir);
							Color Li;
							if (pScene->Intersect(rayLight, &isect))
							{
								if (pLight.Ptr() == (Light*)isect.mpAreaLight)
									Li = pLight->Emit(-lightDir);
							}

							if (!Li.IsBlack())
							{
								L += f * Li * Math::AbsDot(lightDir, normal) * misWeight / bsdfPdf;
							}
						}
					}
				}

				L += Integrator::SpecularReflect(this, pScene, ray, diffGeom, pSamples, memory, random);
				L += Integrator::SpecularTransmit(this, pScene, ray, diffGeom, pSamples, memory, random);
			}

			return L;
		}
	}
}