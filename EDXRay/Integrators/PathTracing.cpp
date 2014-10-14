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
		Color PathTracingIntegrator::Li(const RayDifferential& ray, const Scene* pScene, const SampleBuffer* pSampleBuf, RandomGen& random, MemoryArena& memory) const
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

					// Compute ray differentials
					diffGeom.ComputeDifferentials(pathRay);

					// Explicitly sample light sources
					const BSDF* pBSDF = diffGeom.mpBSDF;
					if (!pBSDF->IsSpecular())
					{
						Sample lightSample = Sample(mpLightSampleOffsets[bounce], pSampleBuf);
						Sample bsdfSample = Sample(mpBSDFSampleOffsets[bounce], pSampleBuf);
						auto lightIdx = Math::Min(lightSample.w * pScene->GetLights().size(), pScene->GetLights().size() - 1);
						L += pathThroughput * Integrator::EstimateDirectLighting(diffGeom, -pathRay.mDir, pScene->GetLights()[lightIdx].Ptr(), pScene, lightSample, bsdfSample);
					}

					const Vector3& pos = diffGeom.mPosition;
					const Vector3& normal = diffGeom.mNormal;
					Vector3 vOut = -pathRay.mDir;
					Vector3 vIn;
					float pdf;
					ScatterType bsdfFlags;
					Sample scatterSample = Sample(mpScatterOffsets[bounce], pSampleBuf);
					Color f = pBSDF->SampleScattered(vOut, scatterSample, diffGeom, &vIn, &pdf, BSDF_ALL, &bsdfFlags);
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

		void PathTracingIntegrator::RequestSamples(const Scene* pScene, SampleBuffer* pSampleBuf)
		{
			assert(pSampleBuf);

			mpLightSampleOffsets = new SampleOffsets[mMaxDepth];
			mpBSDFSampleOffsets = new SampleOffsets[mMaxDepth];
			mpScatterOffsets = new SampleOffsets[mMaxDepth];

			for (auto i = 0; i < mMaxDepth; i++)
			{
				mpLightSampleOffsets[i] = SampleOffsets(1, pSampleBuf);
				mpBSDFSampleOffsets[i] = SampleOffsets(1, pSampleBuf);
				mpScatterOffsets[i] = SampleOffsets(1, pSampleBuf);
			}

			pSampleBuf->Validate();
		}

		PathTracingIntegrator::~PathTracingIntegrator()
		{
			SafeDeleteArray(mpLightSampleOffsets);
			SafeDeleteArray(mpBSDFSampleOffsets);
			SafeDeleteArray(mpScatterOffsets);
		}
	}
}