#include "Integrator.h"
#include "Scene.h"
#include "Light.h"
#include "Medium.h"
#include "Sampling.h"
#include "DifferentialGeom.h"
#include "BSDF.h"
#include "Sampler.h"
#include "Film.h"
#include "Config.h"
#include "Graphics/Color.h"
#include "../Core/Ray.h"

#include <ppl.h>
using namespace concurrency;

namespace EDX
{
	namespace RayTracer
	{
		void TiledIntegrator::Render(const Scene* pScene, const Camera* pCamera, Sampler* pSampler, Film* pFilm) const
		{
			for (int spp = 0; spp < mJobDesc.SamplesPerPixel; spp++)
			{
				int numTiles = mTaskSync.GetNumTiles();

				parallel_for(0, numTiles, [&](int i)
				{
					const RenderTile& tile = mTaskSync.GetTile(i);

					// Clone a sampler for this tile
					UniquePtr<Sampler> pTileSampler(pSampler->Clone(spp * numTiles + i));

					RandomGen random;
					MemoryPool memory;

					for (auto y = tile.minY; y < tile.maxY; y++)
					{
						for (auto x = tile.minX; x < tile.maxX; x++)
						{
							if (mTaskSync.Aborted())
								return;

							pTileSampler->StartPixel(x, y);
							CameraSample camSample;
							pTileSampler->GenerateSamples(x, y, &camSample, random);
							camSample.imageX += x;
							camSample.imageY += y;

							RayDifferential ray;
							Color L = Color::BLACK;
							if (pCamera->GenRayDifferential(camSample, &ray))
							{
								L = Li(ray, pScene, pTileSampler.Get(), random, memory);
							}

							pFilm->AddSample(camSample.imageX, camSample.imageY, L);
							memory.FreeAll();
						}
					}
				});

				pSampler->AdvanceSampleIndex();

				pFilm->IncreSampleCount();
				pFilm->ScaleToPixel();

				if (mTaskSync.Aborted())
					break;

				//mFrameTime = mTimer.GetElapsedTime();
			}
		}

		Color Integrator::EstimateDirectLighting(const Scatter& scatter,
			const Vector3& outDir,
			const Light* pLight,
			const Scene* pScene,
			Sampler* pSampler,
			ScatterType scatterType)
		{
			const Vector3& position = scatter.mPosition;
			const Vector3& normal = scatter.mNormal;

			Color L;

			bool requireMIS = false;
			if (scatter.IsSurfaceScatter()) // Handle surface scattering
			{
				const DifferentialGeom& diffGeom = static_cast<const DifferentialGeom&>(scatter);
				const BSDF* pBSDF = diffGeom.mpBSDF;

				requireMIS = (pBSDF->GetScatterType() & BSDF_GLOSSY);
			}

			// Sample light sources
			{
				Vector3 lightDir;
				VisibilityTester visibility;
				float lightPdf, shadingPdf;
				Color transmittance;
				const Color Li = pLight->Illuminate(scatter, pSampler->GetSample(), &lightDir, &visibility, &lightPdf);

				if (lightPdf > 0.0f && !Li.IsBlack())
				{
					Color f;
					if (scatter.IsSurfaceScatter()) // Handle surface scattering
					{
						const DifferentialGeom& diffGeom = static_cast<const DifferentialGeom&>(scatter);
						const BSDF* pBSDF = diffGeom.mpBSDF;

						f = pBSDF->Eval(outDir, lightDir, diffGeom, scatterType);
						f *= Math::AbsDot(lightDir, normal);
						shadingPdf = pBSDF->Pdf(outDir, lightDir, diffGeom);
					}
					else
					{
						const MediumScatter& mediumScatter = static_cast<const MediumScatter&>(scatter);
						const PhaseFunctionHG* pPhaseFunc = mediumScatter.mpPhaseFunc;

						const float phase = pPhaseFunc->Eval(outDir, lightDir);
						f = Color(phase);
						shadingPdf = phase;
					}

					if (!f.IsBlack() && visibility.Unoccluded(pScene))
					{
						Color transmittance = visibility.Transmittance(pScene, pSampler);
						if (pLight->IsDelta() || !requireMIS)
						{
							L += f * Li * transmittance / lightPdf;
							return L;
						}

						const float misWeight = Sampling::PowerHeuristic(1, lightPdf, 1, shadingPdf);
						L += f * Li * transmittance * misWeight / lightPdf;
					}
				}
			}

			// Sample BSDF or medium
			{
				if (!pLight->IsDelta() && requireMIS)
				{
					Vector3 lightDir;
					Color f;
					float shadingPdf;
					if (scatter.IsSurfaceScatter()) // Handle surface scattering
					{
						const DifferentialGeom& diffGeom = static_cast<const DifferentialGeom&>(scatter);
						const BSDF* pBSDF = diffGeom.mpBSDF;

						ScatterType types;
						f = pBSDF->SampleScattered(outDir, pSampler->GetSample(), diffGeom, &lightDir, &shadingPdf, scatterType, &types);
						f *= Math::AbsDot(lightDir, normal);
					}
					else
					{
						const MediumScatter& mediumScatter = static_cast<const MediumScatter&>(scatter);
						const PhaseFunctionHG* pPhaseFunc = mediumScatter.mpPhaseFunc;

						const float phase = pPhaseFunc->Sample(outDir, &lightDir, pSampler->Get2D());
						f = Color(phase);
						shadingPdf = phase;
					}

					if (shadingPdf > 0.0f && !f.IsBlack())
					{
						float lightPdf = pLight->Pdf(position, lightDir);
						if (lightPdf > 0.0f)
						{
							float misWeight = Sampling::PowerHeuristic(1, shadingPdf, 1, lightPdf);

							Vector3 center;
							float radius;
							pScene->WorldBounds().BoundingSphere(&center, &radius);

							DifferentialGeom diffGeom;
							Ray rayLight = Ray(position, lightDir, scatter.mMediumInterface.GetMedium(lightDir, normal), 2.0f * radius);
							Color Li;
							if (pScene->Intersect(rayLight, &diffGeom))
							{
								pScene->PostIntersect(rayLight, &diffGeom);
								if (pLight == (Light*)diffGeom.mpAreaLight)
									Li = diffGeom.Emit(-lightDir);
							}
							else if ((Light*)pScene->GetEnvironmentMap() == pLight)
							{
								Li = pLight->Emit(-lightDir);
							}

							if (!Li.IsBlack())
							{
								Color transmittance = Color::WHITE;
								if (rayLight.mpMedium)
									transmittance = rayLight.mpMedium->Transmittance(rayLight, pSampler);

								L += f * Li * transmittance * misWeight / shadingPdf;
							}
						}
					}
				}
			}

			return L;
		}

		Color Integrator::SpecularReflect(const TiledIntegrator* pIntegrator,
			const Scene* pScene,
			Sampler* pSampler,
			const RayDifferential& ray,
			const DifferentialGeom& diffGeom,
			RandomGen& random,
			MemoryPool& memory)
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
				RayDifferential rayRef = RayDifferential(position, vIn, diffGeom.mMediumInterface.GetMedium(vIn, normal), float(Math::EDX_INFINITY), 0.0f, ray.mDepth + 1);
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

				Color L = pIntegrator->Li(rayRef, pScene, pSampler, random, memory);
				color = f * L * Math::AbsDot(vIn, normal) / pdf;
			}

			return color;
		}

		Color Integrator::SpecularTransmit(const TiledIntegrator* pIntegrator,
			const Scene* pScene,
			Sampler* pSampler,
			const RayDifferential& ray,
			const DifferentialGeom& diffGeom,
			RandomGen& random,
			MemoryPool& memory)
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
				RayDifferential rayRfr = RayDifferential(position, vIn, diffGeom.mMediumInterface.GetMedium(vIn, normal), float(Math::EDX_INFINITY), 0.0f, ray.mDepth + 1);
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

				Color L = pIntegrator->Li(rayRfr, pScene, pSampler, random, memory);
				color = f * L * Math::AbsDot(vIn, normal) / pdf;
			}

			return color;
		}
	}
}