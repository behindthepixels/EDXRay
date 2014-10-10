#include "Integrator.h"
#include "Scene.h"
#include "DifferentialGeom.h"
#include "BSDF.h"
#include "Sampler.h"
#include "Graphics/Color.h"
#include "Math/Ray.h"

namespace EDX
{
	namespace RayTracer
	{
		Color Integrator::SpecularReflect(const Integrator* pIntegrator,
			const Scene* pScene,
			const RayDifferential& ray,
			const DifferentialGeom& diffGeom,
			const SampleBuffer* pSamples,
			MemoryArena& memory,
			RandomGen& random)
		{
			const Vector3& position = diffGeom.mPosition;
			const Vector3& normal = diffGeom.mNormal;
			const BSDF* pBSDF = diffGeom.mpBSDF;
			Vector3 vOut = -ray.mDir, vIn;
			float fPDF;

			Color f = pBSDF->SampleScattered(vOut, Sample(), diffGeom, &vIn, &fPDF, ScatterType(BSDF_REFLECTION | BSDF_SPECULAR));

			Color color;
			if (fPDF > 0.0f && !f.IsBlack() && Math::AbsDot(vIn, normal) != 0.0f)
			{
				RayDifferential rayRef = RayDifferential(position, vIn, float(Math::EDX_INFINITY), 0.0f, ray.mDepth + 1);
				if (ray.mbHasDifferential)
				{
					rayRef.mbHasDifferential = true;
					rayRef.mDxOrg = position + diffGeom.mvDpdx;
					rayRef.mDyOrg = position + diffGeom.mvDpdy;

					Vector3 vDndx = diffGeom.mnDndu * diffGeom.mfDudx +
						diffGeom.mnDndv * diffGeom.mfDvdx;
					Vector3 vDndy = diffGeom.mnDndu * diffGeom.mfDudy +
						diffGeom.mnDndv * diffGeom.mfDvdy;

					Vector3 vDOutdx = -ray.mDxDir - vOut;
					Vector3 vDOutdy = -ray.mDyDir - vOut;

					float fDcosdx = Math::Dot(vDOutdx, normal) + Math::Dot(vOut, vDndx);
					float fDcosdy = Math::Dot(vDOutdy, normal) + Math::Dot(vOut, vDndy);

					rayRef.mDxDir = vIn - vDOutdx + 2.0f * (Math::Dot(vOut, normal) * vDndx + fDcosdx * normal);
					rayRef.mDyDir = vIn - vDOutdy + 2.0f * (Math::Dot(vOut, normal) * vDndy + fDcosdy * normal);
				}

				DifferentialGeom diffGeomRef;
				Color L = pIntegrator->Li(rayRef, pScene, pSamples, random, memory);

				color = f * L * Math::AbsDot(vIn, normal) / fPDF;
			}

			return color;
		}

		Color Integrator::SpecularTransmit(const Integrator* pIntegrator,
			const Scene* pScene,
			const RayDifferential& ray,
			const DifferentialGeom& diffGeom,
			const SampleBuffer* pSamples,
			MemoryArena& memory,
			RandomGen& random)
		{
			//const Vector3& position = diffGeom.mpositionition;
			//const Vector3& normal = diffGeom.mnormal;
			//const BSDF* pBSDF = diffGeom.GetBSDF();
			//Vector3 vOut = -ray.mDir, vIn;
			//float fPDF;

			//Color f = pBSDF->SampleScattered(vOut, Sample3D(), diffGeom, &vIn, &fPDF, ScatterType(BSDF_TRANSMISSION | BSDF_SPECULAR));

			//Color color;
			//if (fPDF > 0.0f && !f.IsBlack() && Math::AbsDot(vIn, normal) != 0.0f)
			//{
			//	RayDifferential rayRfr = RayDifferential(position, vIn, float(Math::EDX_INFINITY), 0.0f, ray.miDepth + 1);
			//	if (ray.mbHasDifferential)
			//	{
			//		rayRfr.mbHasDifferential = true;
			//		rayRfr.mDxOrg = position + diffGeom.mvDpdx;
			//		rayRfr.mDyOrg = position + diffGeom.mvDpdy;

			//		float fEta = 1.5f;
			//		Vector3 vD = -vOut;
			//		if (Math::Dot(vOut, normal) < 0.0f)
			//			fEta = 1.0f / fEta;

			//		Vector3 vDndx = diffGeom.mnDndu * diffGeom.mfDudx +
			//			diffGeom.mnDndv * diffGeom.mfDvdx;
			//		Vector3 vDndy = diffGeom.mnDndu * diffGeom.mfDudy +
			//			diffGeom.mnDndv * diffGeom.mfDvdy;

			//		Vector3 vDOutdx = -ray.mDxDir - vOut;
			//		Vector3 vDOutdy = -ray.mDyDir - vOut;

			//		float fDcosdx = Math::Dot(vDOutdx, normal) + Math::Dot(vOut, vDndx);
			//		float fDcosdy = Math::Dot(vDOutdy, normal) + Math::Dot(vOut, vDndy);

			//		float fMu = fEta * Math::Dot(vD, normal) - Math::Dot(vIn, normal);
			//		float dfMudx = (fEta - (fEta * fEta*Math::Dot(vD, normal)) / Math::Dot(vIn, normal)) * fDcosdx;
			//		float dfMudy = (fEta - (fEta * fEta*Math::Dot(vD, normal)) / Math::Dot(vIn, normal)) * fDcosdy;

			//		rayRfr.mDxDir = vIn + fEta * vDOutdx - (fMu * vDndx + dfMudx * normal);
			//		rayRfr.mDyDir = vIn + fEta * vDOutdy - (fMu * vDndy + dfMudy * normal);
			//	}
			//	DifferentialGeom diffGeomRfr;
			//	Color L;
			//	if (pScene->Intersect(rayRfr, &diffGeomRfr))
			//	{
			//		diffGeomRfr.ComputeDifferentials(rayRfr);
			//		L = pIntegrator->Li(rayRfr, pScene, pSamples, diffGeomRfr, pIntegrator, memory, random);
			//	}

			//	color = f * L * Math::AbsDot(vIn, normal) / fPDF;
			//}

			return 0;
		}
	}
}