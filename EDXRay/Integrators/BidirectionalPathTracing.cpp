#include "BidirectionalPathTracing.h"
#include "../Core/Camera.h"
#include "../Core/Film.h"
#include "../Core/Scene.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/BSDF.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "../Core/Ray.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		Color BidirPathTracingIntegrator::Li(const RayDifferential& ray,
			const Scene* pScene,
			Sampler* pSampler,
			RandomGen& random,
			MemoryPool& memory) const
		{
			// Generate the light path
			PathVertex* pLightPath = memory.Alloc<PathVertex>(mMaxDepth);
			int numLightVertex;
			int lightPathLength = GenerateLightPath(pScene, pSampler, mMaxDepth + 1, pLightPath, mpCamera, mpFilm, &numLightVertex, true, random);

			// Initialize the camera PathState
			PathState cameraPathState;
			SampleCamera(pScene, ray, mpCamera, mpFilm, cameraPathState);

			// Trace camera path, and connect it with the light path
			Color Ret = Color::BLACK;
			while (true)
			{
				RayDifferential pathRay = RayDifferential(cameraPathState.Origin, cameraPathState.Direction);
				DifferentialGeom diffGeomLocal;

				if (!pScene->Intersect(pathRay, &diffGeomLocal))
				{
					if (pScene->GetEnvironmentMap())
					{
						cameraPathState.PathLength++;

						Ret += cameraPathState.Throughput *
							HittingLightSource(pScene,
								pathRay,
								diffGeomLocal,
								pScene->GetEnvironmentMap(),
								cameraPathState,
								random);
					}
					break;
				}

				pScene->PostIntersect(pathRay, &diffGeomLocal);

				// Update MIS quantities before storing the vertex
				float cosIn = Math::AbsDot(diffGeomLocal.mNormal, -pathRay.mDir);
				cameraPathState.DVCM *= MIS(diffGeomLocal.mDist * diffGeomLocal.mDist);
				cameraPathState.DVCM /= MIS(cosIn);
				cameraPathState.DVC /= MIS(cosIn);

				// Add contribution when directly hitting a light source
				if (diffGeomLocal.GetAreaLight() != nullptr)
				{
					cameraPathState.PathLength++;

					Ret += cameraPathState.Throughput *
						HittingLightSource(pScene,
							pathRay,
							diffGeomLocal,
							diffGeomLocal.GetAreaLight(),
							cameraPathState,
							random);

					break;
				}

				if (++cameraPathState.PathLength >= mMaxDepth + 2)
				{
					break;
				}

				const BSDF* pBSDF = diffGeomLocal.mpBSDF;
				if (!pBSDF->IsSpecular())
				{
					// Connect to light source
					Ret += cameraPathState.Throughput *
						ConnectToLight(pScene,
							pathRay,
							diffGeomLocal,
							pSampler,
							cameraPathState,
							random);

					// Connect to light vertices
					for (int i = 0; i < numLightVertex; i++)
					{
						const PathVertex& lightVertex = pLightPath[i];

						if (lightVertex.PathLength + cameraPathState.PathLength - 2 > mMaxDepth)
						{
							break;
						}

						Ret += lightVertex.Throughput * cameraPathState.Throughput *
							ConnectVertex(pScene,
								diffGeomLocal,
								lightVertex,
								cameraPathState,
								random);
					}
				}

				if (!SampleScattering(pScene, pathRay, diffGeomLocal, pSampler->GetSample(), cameraPathState, random))
				{
					break;
				}
			}

			return Ret;
		}

		BidirPathTracingIntegrator::~BidirPathTracingIntegrator()
		{
		}

		BidirPathTracingIntegrator::PathState BidirPathTracingIntegrator::SampleLightSource(
			const Scene* pScene,
			Sampler* pSampler,
			RandomGen& random)
		{
			PathState ret;

			float lightIdSample = pSampler->Get1D();
			float lightPickPdf;
			auto pSampledLight = pScene->ChooseLightSource(lightIdSample, &lightPickPdf);

			Ray lightRay;
			Vector3 emitDir;
			float emitPdf, directPdf;
			Sample lightSample1 = pSampler->GetSample();
			Sample lightSample2 = pSampler->GetSample();

			ret.Throughput = pSampledLight->Sample(lightSample1, lightSample2, &lightRay, &emitDir, &emitPdf, &directPdf);
			if (emitPdf == 0.0f)
				return ret;

			directPdf *= lightPickPdf;
			emitPdf *= lightPickPdf;
			ret.Throughput /= emitPdf;
			ret.IsFiniteLight = pSampledLight->IsFinite();
			ret.SpecularPath = true;
			ret.PathLength = 1;
			ret.Direction = lightRay.mDir;
			ret.Origin = lightRay.mOrg;

			float emitCos = Math::Dot(emitDir, lightRay.mDir);
			ret.DVCM = MIS(directPdf / emitPdf);
			ret.DVC = pSampledLight->IsDelta() ? 0.0f : (pSampledLight->IsFinite() ? MIS(emitCos / emitPdf) : MIS(1.0f / emitPdf));

			return ret;
		}

		int BidirPathTracingIntegrator::GenerateLightPath(const Scene* pScene,
			Sampler* pSampler,
			const int maxDepth,
			PathVertex* pPath,
			const Camera* pCamera,
			Film* pFilm,
			int* pVertexCount,
			const bool bConnectToCamera,
			RandomGen& random,
			const int RRDepth)
		{
			if (maxDepth == 0)
			{
				*pVertexCount = 0;
				return 0;
			}

			// Choose a light source, and generate the light path
			PathState lightPathState = SampleLightSource(pScene, pSampler, random);
			if (lightPathState.Throughput.IsBlack())
				return 0;

			if (lightPathState.PathLength >= maxDepth)
			{
				*pVertexCount = 0;
				return lightPathState.PathLength;
			}

			// Trace light path
			*pVertexCount = 0;
			while (true)
			{
				Ray pathRay = Ray(lightPathState.Origin, lightPathState.Direction);
				DifferentialGeom diffGeom;
				if (!pScene->Intersect(pathRay, &diffGeom))
				{
					return lightPathState.PathLength;
				}
				pScene->PostIntersect(pathRay, &diffGeom);

				// Update MIS quantities before storing the vertex
				if (lightPathState.PathLength > 1 || lightPathState.IsFiniteLight)
				{
					lightPathState.DVCM *= MIS(diffGeom.mDist * diffGeom.mDist);
				}
				float cosIn = Math::AbsDot(diffGeom.mNormal, -pathRay.mDir);
				lightPathState.DVCM /= MIS(cosIn);
				lightPathState.DVC /= MIS(cosIn);

				// Store the current vertex and connect the light vertex with camera
				// unless hitting purely specular surfaces
				const BSDF* pBSDF = diffGeom.mpBSDF;
				if (!pBSDF->IsSpecular())
				{
					// Store the vertex
					PathVertex& lightVertex = pPath[(*pVertexCount)++];
					lightVertex.Throughput = lightPathState.Throughput;
					lightVertex.PathLength = lightPathState.PathLength + 1;
					lightVertex.DiffGeom = diffGeom;
					lightVertex.InDir = -lightPathState.Direction;
					lightVertex.DVCM = lightPathState.DVCM;
					lightVertex.DVC = lightPathState.DVC;

					// Connect to camera
					if (bConnectToCamera)
					{
						Vector3 ptRaster;
						auto connectRadiance = ConnectToCamera(pScene, diffGeom, lightVertex, pCamera, pSampler, pFilm, &ptRaster, random);

						pFilm->Splat(ptRaster.x, ptRaster.y, connectRadiance);
					}
				}

				// Terminate if path is too long
				if (++lightPathState.PathLength >= maxDepth)
				{
					break;
				}

				if (!SampleScattering(pScene, pathRay, diffGeom, pSampler->GetSample(), lightPathState, random, RRDepth))
				{
					break;
				}
			}

			return lightPathState.PathLength;
		}

		Color BidirPathTracingIntegrator::ConnectToCamera(const Scene* pScene,
			const DifferentialGeom& diffGeom,
			const PathVertex& pathVertex,
			const Camera* pCamera,
			Sampler* pSampler,
			Film* pFilm,
			Vector3* pRasterPos,
			RandomGen& random)
		{
			Vector3 camPos = pCamera->mPos;
			Vector3 camDir = pCamera->mDir;

			// Check if the point lies within the raster range
			Vector3 dirToCamera;

			if (pCamera->GetCircleOfConfusionRadius() == 0.0f)
			{
				*pRasterPos = pCamera->WorldToRaster(diffGeom.mPosition);
				dirToCamera = camPos - diffGeom.mPosition;
			}
			else
			{
				Vector2 screenCoord = 2.0f * Vector2(pRasterPos->x, pRasterPos->y) / Vector2(pCamera->GetFilmSizeX(), pCamera->GetFilmSizeY()) - Vector2::UNIT_SCALE;
				screenCoord.x *= pCamera->mRatio;
				screenCoord.y *= -1.0f;

				float U, V;
				Sampling::ConcentricSampleDisk(pSampler->Get1D(), pSampler->Get1D(), &U, &V);

				if (Math::Length(Vector2(screenCoord.x + U, screenCoord.y + V)) > pCamera->mVignetteFactor)
					return Color::BLACK;

				U *= pCamera->GetCircleOfConfusionRadius();
				V *= pCamera->GetCircleOfConfusionRadius();

				Ray ray;
				ray.mOrg = Vector3(U, V, 0.0f);
				ray.mDir = Matrix::TransformPoint(diffGeom.mPosition, pCamera->GetViewMatrix()) - ray.mOrg;
				Vector3 focalHit = ray.CalcPoint(pCamera->GetFocusDistance() / ray.mDir.z);

				*pRasterPos = pCamera->CameraToRaster(focalHit);
				dirToCamera = Matrix::TransformPoint(ray.mOrg, pCamera->GetViewInvMatrix()) - diffGeom.mPosition;
			}

			// Check if the point is in front of camera
			if (Math::Dot(camDir, -dirToCamera) <= 0.0f)
			{
				return Color::BLACK;
			}

			if (!pCamera->CheckRaster(*pRasterPos))
			{
				return Color::BLACK;
			}

			float distToCamera = Math::Length(dirToCamera);
			dirToCamera = Math::Normalize(dirToCamera);

			const BSDF* pBSDF = diffGeom.mpBSDF;
			Color bsdfFac = pBSDF->Eval(dirToCamera, pathVertex.InDir, diffGeom, ScatterType(BSDF_ALL & ~BSDF_SPECULAR))
				* Math::AbsDot(pathVertex.InDir, diffGeom.mNormal)
				/ Math::AbsDot(pathVertex.InDir, diffGeom.mGeomNormal);
			if (bsdfFac.IsBlack())
				return Color::BLACK;

			float pdf = pBSDF->Pdf(pathVertex.InDir, dirToCamera, diffGeom, ScatterType(BSDF_ALL & ~BSDF_SPECULAR));
			float reversePdf = pBSDF->Pdf(dirToCamera, pathVertex.InDir, diffGeom, ScatterType(BSDF_ALL & ~BSDF_SPECULAR));
			if (pdf == 0.0f || reversePdf == 0.0f)
				return Color::BLACK;

			float cosToCam = Math::Dot(diffGeom.mGeomNormal, dirToCamera);

			float cosAtCam = Math::Dot(camDir, -dirToCamera);
			float rasterToCamDist = pCamera->GetImagePlaneDistance() / cosAtCam;
			float rasterToSolidAngleFac = rasterToCamDist * rasterToCamDist / cosAtCam;

			float cameraPdfA = rasterToSolidAngleFac * Math::Abs(cosToCam) / (distToCamera * distToCamera);
			float WLight = MIS(cameraPdfA / (float)pFilm->GetPixelCount()) * (pathVertex.DVCM + pathVertex.DVC * MIS(reversePdf));
			float MISWeight = 1.0f / (WLight + 1.0f);

			Color contrib = MISWeight * pathVertex.Throughput * bsdfFac * cameraPdfA / (float)pFilm->GetPixelCount();

			Ray rayToCam = Ray(diffGeom.mPosition, dirToCamera, diffGeom.mMediumInterface.GetMedium(dirToCamera, diffGeom.mNormal), distToCamera);

			if (!contrib.IsBlack() && !pScene->Occluded(rayToCam))
			{
				return contrib;
			}

			return Color::BLACK;
		}

		void BidirPathTracingIntegrator::SampleCamera(const Scene* pScene,
			const RayDifferential& primRay,
			const Camera* pCamera,
			Film* pFilm,
			PathState& initPathState)
		{
			float cosAtCam = Math::Dot(pCamera->mDir, primRay.mDir);
			float rasterToCamDist = pCamera->GetImagePlaneDistance() / cosAtCam;
			float cameraPdfW = rasterToCamDist * rasterToCamDist / cosAtCam;

			initPathState.Origin = primRay.mOrg;
			initPathState.Direction = primRay.mDir;
			initPathState.Throughput = Color::WHITE;
			initPathState.PathLength = 1;
			initPathState.SpecularPath = true;

			initPathState.DVC = 0.0f;
			initPathState.DVCM = MIS(pFilm->GetPixelCount() / cameraPdfW);
		}

		Color BidirPathTracingIntegrator::ConnectToLight(const Scene* pScene,
			const RayDifferential& pathRay,
			const DifferentialGeom& diffGeom,
			Sampler* pSampler,
			const PathState& cameraPathState,
			RandomGen& random)
		{
			// Sample light source and get radiance
			float lightIdSample = pSampler->Get1D();
			float lightPickPdf;
			const Light* pLight = pScene->ChooseLightSource(lightIdSample, &lightPickPdf);

			const Vector3& pos = diffGeom.mPosition;
			Vector3 vIn;
			VisibilityTester visibility;
			float lightPdfW;
			float cosAtLight;
			float emitPdfW;
			Sample lightSample = pSampler->GetSample();
			Color radiance = pLight->Illuminate(diffGeom, lightSample, &vIn, &visibility, &lightPdfW, &cosAtLight, &emitPdfW);
			if (radiance.IsBlack() || lightPdfW == 0.0f)
			{
				return Color::BLACK;
			}

			const BSDF* pBSDF = diffGeom.mpBSDF;
			Vector3 vOut = -pathRay.mDir;
			Color bsdfFac = pBSDF->Eval(vOut, vIn, diffGeom);
			if (bsdfFac.IsBlack())
			{
				return Color::BLACK;
			}

			float bsdfPdfW = pBSDF->Pdf(vOut, vIn, diffGeom);
			if (bsdfPdfW == 0.0f)
			{
				return Color::BLACK;
			}
			if (pLight->IsDelta())
			{
				bsdfPdfW = 0.0f;
			}
			float bsdfRevPdfW = pBSDF->Pdf(vIn, vOut, diffGeom);

			float WLight = MIS(bsdfPdfW / (lightPdfW * lightPickPdf));

			float cosToLight = Math::AbsDot(diffGeom.mNormal, vIn);
			float WCamera = MIS(emitPdfW * cosToLight / (lightPdfW * cosAtLight))
				* (cameraPathState.DVCM + cameraPathState.DVC * MIS(bsdfRevPdfW));

			float fMISWeight = 1.0f / (WLight + 1.0f + WCamera);
			Color contribution = (fMISWeight * cosToLight / (lightPdfW * lightPickPdf)) * bsdfFac * radiance;

			if (contribution.IsBlack() || !visibility.Unoccluded(pScene))
			{
				return Color::BLACK;
			}

			return contribution;
		}

		Color BidirPathTracingIntegrator::HittingLightSource(const Scene* pScene,
			const RayDifferential& pathRay,
			const DifferentialGeom& diffGeom,
			const Light* pLight,
			const PathState& cameraPathState,
			RandomGen& random)
		{
			float lightPickPdf = pScene->LightPdf(pLight);
			Vector3 vOut = -pathRay.mDir;
			float directPdfA;
			float emitPdfW;
			Color emittedRadiance = pLight->Emit(vOut, diffGeom.mGeomNormal, &emitPdfW, &directPdfA);

			if (emittedRadiance.IsBlack())
			{
				return Color::BLACK;
			}
			if (cameraPathState.PathLength == 2)
			{
				return emittedRadiance;
			}

			directPdfA *= lightPickPdf;
			emitPdfW *= lightPickPdf;
			float WCamera = MIS(directPdfA) * cameraPathState.DVCM + MIS(emitPdfW) * cameraPathState.DVC;
			float MISWeight = 1.0f / (1.0f + WCamera);

			return MISWeight * emittedRadiance;
		}

		Color BidirPathTracingIntegrator::ConnectVertex(const Scene* pScene,
			const DifferentialGeom& cameraDiffGeom,
			const PathVertex& lightVertex,
			const PathState& cameraState,
			RandomGen& random)
		{
			const Vector3& cameraPos = cameraDiffGeom.mPosition;

			Vector3 dirToLight = lightVertex.DiffGeom.mPosition - cameraPos;
			float distToLightSqr = Math::LengthSquared(dirToLight);
			float distToLight = Math::Sqrt(distToLightSqr);
			dirToLight = Math::Normalize(dirToLight);

			const BSDF* pCameraBSDF = cameraDiffGeom.mpBSDF;
			const Vector3& cameraNormal = cameraDiffGeom.mNormal;
			Vector3 vOutCam = -cameraState.Direction;
			Color cameraBsdfFac = pCameraBSDF->Eval(vOutCam, dirToLight, cameraDiffGeom);
			float cosAtCam = Math::Dot(cameraNormal, dirToLight);
			float cameraDirPdfW = pCameraBSDF->Pdf(vOutCam, dirToLight, cameraDiffGeom);
			float cameraReversePdfW = pCameraBSDF->Pdf(dirToLight, vOutCam, cameraDiffGeom);

			if (cameraBsdfFac.IsBlack() || cameraDirPdfW == 0.0f || cameraReversePdfW == 0.0f)
				return Color::BLACK;

			const BSDF* pLightBSDF = lightVertex.DiffGeom.mpBSDF;
			Vector3 dirToCamera = -dirToLight;
			Color lightBsdfFac = pLightBSDF->Eval(lightVertex.InDir, dirToCamera, lightVertex.DiffGeom);
			float cosAtLight = Math::Dot(lightVertex.DiffGeom.mNormal, dirToCamera);
			float lightDirPdfW = pLightBSDF->Pdf(lightVertex.InDir, dirToCamera, lightVertex.DiffGeom);
			float lightRevPdfW = pLightBSDF->Pdf(dirToCamera, lightVertex.InDir, lightVertex.DiffGeom);

			if (lightBsdfFac.IsBlack() || lightDirPdfW == 0.0f || lightRevPdfW == 0.0f)
				return Color::BLACK;

			float geometryTerm = cosAtLight * cosAtCam / distToLightSqr;
			if (geometryTerm < 0.0f)
			{
				return Color::BLACK;
			}

			float cameraDirPdfA = Sampling::PdfWToA(cameraDirPdfW, distToLight, cosAtLight);
			float lightDirPdfA = Sampling::PdfWToA(lightDirPdfW, distToLight, cosAtCam);

			float WLight = MIS(cameraDirPdfA) * (lightVertex.DVCM + lightVertex.DVC * MIS(lightRevPdfW));
			float WCamera = MIS(lightDirPdfA) * (cameraState.DVCM + cameraState.DVC * MIS(cameraReversePdfW));

			float fMISWeight = 1.0f / (WLight + 1.0f + WCamera);

			Color contribution = (fMISWeight * geometryTerm) * lightBsdfFac * cameraBsdfFac;

			Ray rayToLight = Ray(cameraPos, dirToLight, cameraDiffGeom.mMediumInterface.GetMedium(dirToLight, cameraNormal), distToLight);
			if (contribution.IsBlack() || pScene->Occluded(rayToLight))
			{
				return Color::BLACK;
			}

			return contribution;
		}

		bool BidirPathTracingIntegrator::SampleScattering(const Scene* pScene,
			const RayDifferential& rayIn,
			const DifferentialGeom& diffGeom,
			const Sample& bsdfSample,
			PathState& pathState,
			RandomGen& random,
			const int RRDepth)
		{
			// Sample the scattered direction
			const BSDF* pBSDF = diffGeom.mpBSDF;
			Vector3 scatteredDir;
			float scatteredPdf;
			ScatterType sampledBSDF;
			Color bsdfFac = pBSDF->SampleScattered(-rayIn.mDir, bsdfSample, diffGeom,
				&scatteredDir, &scatteredPdf, BSDF_ALL, &sampledBSDF);

			bool nonSpecularBounce = (sampledBSDF & BSDF_SPECULAR) == 0;

			// For specular bounce, reverse pdf equals to forward pdf
			float reversePdf = nonSpecularBounce ?
				pBSDF->Pdf(scatteredDir, -rayIn.mDir, diffGeom) :
				scatteredPdf;

			// Update PathState before walking to the next vertex
			if (bsdfFac.IsBlack() || scatteredPdf == 0.0f)
			{
				return false;
			}
			// Apply Russian Roulette if non-specular surface was hit
			if (nonSpecularBounce && RRDepth != -1 && pathState.PathLength > RRDepth)
			{
				float fRRProb = Math::Min(1.0f, pathState.Throughput.Luminance());
				if (random.Float() < fRRProb)
				{
					scatteredPdf *= fRRProb;
					reversePdf *= fRRProb;
				}
				else
				{
					return false;
				}
			}

			pathState.Origin = diffGeom.mPosition;
			pathState.Direction = scatteredDir;

			float cosOut = Math::AbsDot(diffGeom.mNormal, scatteredDir);
			if (nonSpecularBounce)
			{
				pathState.SpecularPath &= 0;

				pathState.DVC = MIS(cosOut / scatteredPdf) * (pathState.DVC * MIS(reversePdf) + pathState.DVCM);
				pathState.DVCM = MIS(1.0f / scatteredPdf);
			}
			else
			{
				pathState.SpecularPath &= 1;

				pathState.DVCM = 0.0f;
				//pathState.DVC *= MIS(cosOut / scatteredPdf) * MIS(reversePdf);
				pathState.DVC *= MIS(cosOut);
			}

			pathState.Throughput *= bsdfFac * cosOut / scatteredPdf;

			return true;
		}
	}
}