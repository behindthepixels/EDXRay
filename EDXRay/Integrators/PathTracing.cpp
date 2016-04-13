#include "PathTracing.h"
#include "../Core/Scene.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/BSDF.h"
#include "../Core/BSSRDF.h"
#include "../Core/Medium.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "../Core/Ray.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		Color PathTracingIntegrator::Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryArena& memory) const
		{
			Color L = Color::BLACK;
			Color pathThroughput = Color::WHITE;

			bool specBounce = true;
			RayDifferential pathRay = ray;
			for (auto bounce = 0; ; bounce++)
			{
				DifferentialGeom diffGeom;
				bool intersected = pScene->Intersect(pathRay, &diffGeom);

				MediumScatter mediumScatter;
				if (pathRay.mpMedium)
					pathThroughput *= pathRay.mpMedium->Sample(pathRay, pSampler, &mediumScatter);

				if (pathThroughput.IsBlack())
					break;
				
				// Sampled surface
				if (!mediumScatter.IsValid())
				{
					pScene->PostIntersect(pathRay, &diffGeom);

					if (specBounce)
					{
						if (intersected)
						{
							L += pathThroughput * diffGeom.Emit(-pathRay.mDir);
						}
						else
						{
							if (pScene->GetEnvironmentMap())
								L += pathThroughput * pScene->GetEnvironmentMap()->Emit(-pathRay.mDir);
						}
					}

					if (!intersected || bounce >= mMaxDepth)
						break;

					// Explicitly sample light sources
					const BSDF* pBSDF = diffGeom.mpBSDF;
					if (!pBSDF->IsSpecular())
					{
						float lightIdxSample = pSampler->Get1D();
						auto lightIdx = Math::Min(lightIdxSample * pScene->GetLights().size(), pScene->GetLights().size() - 1);
						L += pathThroughput *
							Integrator::EstimateDirectLighting(diffGeom, -pathRay.mDir, pScene->GetLights()[lightIdx].Ptr(), pScene, pSampler);
					}

					const Vector3& pos = diffGeom.mPosition;
					const Vector3& normal = diffGeom.mNormal;
					Vector3 vOut = -pathRay.mDir;
					Vector3 vIn;
					float pdf;
					ScatterType bsdfFlags;
					Sample scatterSample = pSampler->GetSample();
					Color f = pBSDF->SampleScattered(vOut, scatterSample, diffGeom, &vIn, &pdf, BSDF_ALL, &bsdfFlags);
					if (f.IsBlack() || pdf == 0.0f)
						break;
					pathThroughput *= f * Math::AbsDot(vIn, normal) / pdf;

					bool sampleSubsurface = diffGeom.mpBSSRDF && Math::Dot(vOut, normal) > 0.0f && (bsdfFlags & BSDF_TRANSMISSION);
					if (!sampleSubsurface)
					{
						specBounce = (bsdfFlags & BSDF_SPECULAR) != 0;
						pathRay = Ray(pos, vIn, diffGeom.mMediumInterface.GetMedium(vIn, normal));
					}
					else // Account for attenuated subsurface scattering, if applicable
					{
						// Importance sample the BSSRDF
						Sample bssrdfSample = pSampler->GetSample();
						DifferentialGeom subsurfDiffGeom;
						float subsurfPdf;
						Color S = diffGeom.mpBSSRDF->SampleSubsurfaceScattered(
							vOut, bssrdfSample, diffGeom, pScene, &subsurfDiffGeom, &subsurfPdf, memory);

						if (S.IsBlack() || subsurfPdf == 0)
							break;

						pathThroughput *= S / subsurfPdf;

						// Account for the attenuated direct subsurface scattering
						// component
						float lightIdxSample = pSampler->Get1D();
						auto lightIdx = Math::Min(lightIdxSample * pScene->GetLights().size(), pScene->GetLights().size() - 1);
						L += pathThroughput *
							Integrator::EstimateDirectLighting(subsurfDiffGeom, subsurfDiffGeom.mNormal, pScene->GetLights()[lightIdx].Ptr(), pScene, pSampler);

						// Account for the indirect subsurface scattering component
						pBSDF = subsurfDiffGeom.mpBSDF;
						f = pBSDF->SampleScattered(subsurfDiffGeom.mNormal, pSampler->GetSample(), subsurfDiffGeom, &vIn, &pdf, BSDF_ALL, &bsdfFlags);
						if (f.IsBlack() || pdf == 0.0f)
							break;

						specBounce = (bsdfFlags & BSDF_SPECULAR) != 0;
						pathThroughput *= f * Math::AbsDot(vIn, subsurfDiffGeom.mNormal) / pdf;
						pathRay = Ray(subsurfDiffGeom.mPosition, vIn, subsurfDiffGeom.mMediumInterface.GetMedium(vIn, subsurfDiffGeom.mNormal));
					}
				}
				else // Sampled medium
				{
					float lightIdxSample = pSampler->Get1D();
					auto lightIdx = Math::Min(lightIdxSample * pScene->GetLights().size(), pScene->GetLights().size() - 1);
					L += pathThroughput * Integrator::EstimateDirectLighting(mediumScatter, -pathRay.mDir, pScene->GetLights()[lightIdx].Ptr(), pScene, pSampler);

					if (bounce >= mMaxDepth)
						break;

					const PhaseFunctionHG* pPhaseFunc = mediumScatter.mpPhaseFunc;

					Vector3 vOut = -pathRay.mDir;
					Vector3 vIn;
					pPhaseFunc->Sample(vOut, &vIn, pSampler->Get2D());

					specBounce = false;
					pathRay = Ray(mediumScatter.mPosition, vIn, pathRay.mpMedium);
				}

				// Russian Roulette
				if (bounce > 5)
				{
					float RR = Math::Min(1.0f, pathThroughput.Luminance());
					if (random.Float() > RR)
						break;

					pathThroughput /= RR;
				}
			}

			return L;
		}

		void PathTracingIntegrator::RequestSamples(const Scene* pScene, SampleBuffer* pSampleBuf)
		{
			assert(pSampleBuf);

			mpLightSampleOffsets = SampleOffsets(mMaxDepth, pSampleBuf);
			mpBSDFSampleOffsets = SampleOffsets(mMaxDepth, pSampleBuf);
			mpScatterOffsets = SampleOffsets(mMaxDepth, pSampleBuf);
		}
	}
}