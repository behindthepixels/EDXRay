#include "PathTracing.h"
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
		Color PathTracingIntegrator::Li(const RayDifferential& ray, const Scene* pScene, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory) const
		{
			Color L = Color::BLACK;
			Color pathThroughput = Color::WHITE;

			bool specBounce = false;
			RayDifferential pathRay = ray;
			for (auto bounce = 0; bounce < mMaxDepth; bounce++)
			{
				DifferentialGeom diffGeom;
				if (pScene->Intersect(pathRay, &diffGeom)) // Hit the scene
				{
					pScene->PostIntersect(pathRay, &diffGeom);

					// Compute ray differential for the 1st bounce
					if (bounce == 0)
						diffGeom.ComputeDifferentials(pathRay);

					// Explicitly sample light sources
					L += pathThroughput * Integrator::EstimateDirectLighting(diffGeom, -pathRay.mDir, pScene->GetLight()[0].Ptr(), pScene, Sample(random), Sample(random));

					const Vector3& pos = diffGeom.mPosition;
					const Vector3& normal = diffGeom.mNormal;
					const BSDF* pBSDF = diffGeom.mpBSDF;
					Vector3 vOut = -pathRay.mDir;
					Vector3 vIn;
					float pdf;
					ScatterType bsdfFlags;
					Color f = pBSDF->SampleScattered(vOut, Sample(random), diffGeom, &vIn, &pdf, BSDF_ALL, &bsdfFlags);
					if (f.IsBlack() || pdf == 0.0f)
						break;

					specBounce = (bsdfFlags & BSDF_SPECULAR) != 0;
					pathThroughput *= f * Math::AbsDot(vIn, normal) / pdf;
					pathRay = Ray(pos, vIn);

					// Russian Roulette
					if (bounce > 3)
					{
						float RR = Math::Min(1.0f, pathThroughput.Luminance());
						if (random.Float() > RR)
							break;

						pathThroughput /= RR;
					}
				}
				else // Hit the environmental light source (if there is one)
				{
					break;
				}
			}

			return L;
		}
	}
}