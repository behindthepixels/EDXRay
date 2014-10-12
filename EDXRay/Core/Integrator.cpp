#include "Integrator.h"
#include "Scene.h"
#include "Light.h"
#include "Sampling.h"
#include "DifferentialGeom.h"
#include "BSDF.h"
#include "Sampler.h"
#include "Graphics/Color.h"
#include "Math/Ray.h"

namespace EDX
{
	namespace RayTracer
	{
		Color Integrator::EstimateDirectLighting(const DifferentialGeom& diffGeom,
			const Vector3& outDir,
			const Light* pLight,
			const Scene* pScene,
			const Sample& lightSample,
			const Sample& bsdfSample)
		{
			const Vector3& position = diffGeom.mPosition;
			const Vector3& normal = diffGeom.mNormal;
			const BSDF* pBSDF = diffGeom.mpBSDF;

			Color L;

			// Sample light sources
			Vector3 lightDir;
			VisibilityTester visibility;
			float lightPdf;
			const Color Li = pLight->Illuminate(position, lightSample, &lightDir, &visibility, &lightPdf);
			if (lightPdf > 0.0f && !Li.IsBlack())
			{
				const Color f = pBSDF->Eval(outDir, lightDir, diffGeom, ScatterType(BSDF_ALL & ~BSDF_SPECULAR));
				if (!f.IsBlack() && visibility.Unoccluded(pScene))
				{
					if (pLight->IsDelta())
					{
						L += f * Li * Math::AbsDot(lightDir, normal) / lightPdf;
						return L;
					}

					const float bsdfPdf = pBSDF->Pdf(outDir, lightDir, diffGeom);
					const float misWeight = Sampling::PowerHeuristic(1, lightPdf, 1, bsdfPdf);
					L += f * Li * Math::AbsDot(lightDir, normal) * misWeight / lightPdf;
				}
			}

			if (pLight->IsDelta())
				return L;

			// Sample BSDF for MIS
			ScatterType types;
			float bsdfPdf;
			const Color f = pBSDF->SampleScattered(outDir, bsdfSample, diffGeom, &lightDir, &bsdfPdf, ScatterType(BSDF_ALL & ~BSDF_SPECULAR), &types);
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
						if (pLight == (Light*)isect.mpAreaLight)
							Li = pLight->Emit(-lightDir);
					}

					if (!Li.IsBlack())
					{
						L += f * Li * Math::AbsDot(lightDir, normal) * misWeight / bsdfPdf;
					}
				}
			}

			return L;
		}

		Color Integrator::SpecularReflect(const Integrator* pIntegrator,
			const Scene* pScene,
			const RayDifferential& ray,
			const DifferentialGeom& diffGeom,
			const SampleBuffer* pSamples,
			RandomGen& random,
			MemoryArena& memory)
		{
			const Vector3& position = diffGeom.mPosition;
			const Vector3& normal = diffGeom.mNormal;
			const BSDF* pBSDF = diffGeom.mpBSDF;
			Vector3 vOut = -ray.mDir, vIn;
			float pdf;

			Color f = pBSDF->SampleScattered(vOut, Sample(), diffGeom, &vIn, &pdf, ScatterType(BSDF_REFLECTION | BSDF_SPECULAR));

			Color color;
			if (pdf > 0.0f && !f.IsBlack() && Math::AbsDot(vIn, normal) != 0.0f)
			{
				RayDifferential rayRef = RayDifferential(position, vIn, float(Math::EDX_INFINITY), 0.0f, ray.mDepth + 1);
				if (ray.mHasDifferential)
				{
					rayRef.mHasDifferential = true;
					rayRef.mDxOrg = position + diffGeom.mDpdx;
					rayRef.mDyOrg = position + diffGeom.mDpdy;

					Vector3 vDndx = diffGeom.mDndu * diffGeom.mDudx +
						diffGeom.mDndv * diffGeom.mDvdx;
					Vector3 vDndy = diffGeom.mDndu * diffGeom.mDudy +
						diffGeom.mDndv * diffGeom.mDvdy;

					Vector3 vDOutdx = -ray.mDxDir - vOut;
					Vector3 vDOutdy = -ray.mDyDir - vOut;

					float fDcosdx = Math::Dot(vDOutdx, normal) + Math::Dot(vOut, vDndx);
					float fDcosdy = Math::Dot(vDOutdy, normal) + Math::Dot(vOut, vDndy);

					rayRef.mDxDir = vIn - vDOutdx + 2.0f * (Math::Dot(vOut, normal) * vDndx + fDcosdx * normal);
					rayRef.mDyDir = vIn - vDOutdy + 2.0f * (Math::Dot(vOut, normal) * vDndy + fDcosdy * normal);
				}

				Color L = pIntegrator->Li(rayRef, pScene, pSamples, random, memory);
				color = f * L * Math::AbsDot(vIn, normal) / pdf;
			}

			return color;
		}

		Color Integrator::SpecularTransmit(const Integrator* pIntegrator,
			const Scene* pScene,
			const RayDifferential& ray,
			const DifferentialGeom& diffGeom,
			const SampleBuffer* pSamples,
			RandomGen& random,
			MemoryArena& memory)
		{
			const Vector3& position = diffGeom.mPosition;
			const Vector3& normal = diffGeom.mNormal;
			const BSDF* pBSDF = diffGeom.mpBSDF;
			Vector3 vOut = -ray.mDir, vIn;
			float pdf;

			Color f = pBSDF->SampleScattered(vOut, Sample(), diffGeom, &vIn, &pdf, ScatterType(BSDF_TRANSMISSION | BSDF_SPECULAR));

			Color color;
			if (pdf > 0.0f && !f.IsBlack() && Math::AbsDot(vIn, normal) != 0.0f)
			{
				RayDifferential rayRfr = RayDifferential(position, vIn, float(Math::EDX_INFINITY), 0.0f, ray.mDepth + 1);
				if (ray.mHasDifferential)
				{
					rayRfr.mHasDifferential = true;
					rayRfr.mDxOrg = position + diffGeom.mDpdx;
					rayRfr.mDyOrg = position + diffGeom.mDpdy;

					float fEta = 1.5f;
					Vector3 vD = -vOut;
					if (Math::Dot(vOut, normal) < 0.0f)
						fEta = 1.0f / fEta;

					Vector3 vDndx = diffGeom.mDndu * diffGeom.mDudx +
						diffGeom.mDndv * diffGeom.mDvdx;
					Vector3 vDndy = diffGeom.mDndu * diffGeom.mDudy +
						diffGeom.mDndv * diffGeom.mDvdy;

					Vector3 vDOutdx = -ray.mDxDir - vOut;
					Vector3 vDOutdy = -ray.mDyDir - vOut;

					float fDcosdx = Math::Dot(vDOutdx, normal) + Math::Dot(vOut, vDndx);
					float fDcosdy = Math::Dot(vDOutdy, normal) + Math::Dot(vOut, vDndy);

					float fMu = fEta * Math::Dot(vD, normal) - Math::Dot(vIn, normal);
					float dfMudx = (fEta - (fEta * fEta*Math::Dot(vD, normal)) / Math::Dot(vIn, normal)) * fDcosdx;
					float dfMudy = (fEta - (fEta * fEta*Math::Dot(vD, normal)) / Math::Dot(vIn, normal)) * fDcosdy;

					rayRfr.mDxDir = vIn + fEta * vDOutdx - (fMu * vDndx + dfMudx * normal);
					rayRfr.mDyDir = vIn + fEta * vDOutdy - (fMu * vDndy + dfMudy * normal);
				}

				Color L = pIntegrator->Li(rayRfr, pScene, pSamples, random, memory);
				color = f * L * Math::AbsDot(vIn, normal) / pdf;
			}

			return color;
		}
	}
}