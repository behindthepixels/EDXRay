#include "MultiplexedMLT.h"
#include "../Core/Camera.h"
#include "../Core/Film.h"
#include "../Core/Scene.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/BSDF.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "../Core/Ray.h"
#include "../Core/Config.h"
#include "Graphics/Color.h"

#include <ppl.h>
using namespace concurrency;

namespace EDX
{
	namespace RayTracer
	{
		float MetropolisSampler::Get1D()
		{
			const int index = GetNextIndex();
			Mutate(index);

			return mSamples[index].value;
		}

		Vector2 MetropolisSampler::Get2D()
		{
			return Vector2(Get1D(), Get1D());
		}

		Sample MetropolisSampler::GetSample()
		{
			Sample ret;
			ret.u = Get1D();
			ret.v = Get1D();
			ret.w = Get1D();

			return ret;
		}

		void MetropolisSampler::GenerateSamples(
			const int pixelX,
			const int pixelY,
			CameraSample* pSamples,
			RandomGen& random)
		{
			pSamples->lensU = Get1D();
			pSamples->lensV = Get1D();
			pSamples->time = Get1D();
		}

		void MetropolisSampler::StartIteration()
		{
			mLargeStep = mRandom.Float() < mLargeStepProb;
			mCurrentIteration++;
		}

		void MetropolisSampler::Mutate(const int index)
		{
			if (index >= mSamples.Size())
				mSamples.Resize(index + 1);

			PrimarySample& sample = mSamples[index];
			if (sample.modifiedIteration < mPrevLargeStepIteration)
			{
				sample.value = mRandom.Float();
				sample.modifiedIteration = mPrevLargeStepIteration;
			}

			sample.Backup();
			if (mLargeStep) // Perform large step mutation
			{
				sample.value = mRandom.Float();
			}
			else // Small step mutation
			{
				int numSmallSteps = mCurrentIteration - sample.modifiedIteration;

				// Sample the standard normal distribution
				const float Sqrt2 = 1.41421356237309504880f;
				float normalSample = Sqrt2 * Math::ErfInv(2 * mRandom.Float() - 1);

				// Compute the effective standard deviation and apply perturbation to
				float effSigma = mSigma * Math::Sqrt(float(numSmallSteps));
				sample.value += normalSample * effSigma;
				sample.value -= Math::FloorToInt(sample.value);
			}

			sample.modifiedIteration = mCurrentIteration;
		}

		void MetropolisSampler::Accept()
		{
			if (mLargeStep)
				mPrevLargeStepIteration = mCurrentIteration;
		}

		void MetropolisSampler::Reject()
		{
			for (auto& it : mSamples)
			{
				if (it.modifiedIteration == mCurrentIteration)
					it.Restore();
			}
			mCurrentIteration--;
		}

		UniquePtr<Sampler> MetropolisSampler::Clone(const int seed) const
		{
			return MakeUnique<MetropolisSampler>(mSigma, mLargeStepProb, seed);
		}

		void MultiplexedMLTIntegrator::Render(const Scene* pScene,
			const Camera* pCamera,
			Sampler* pSampler,
			Film* pFilm) const
		{
			// Generate bootstrap samples and compute normalization constant b
			int numBootstrapSamples = mNumBootstrap * (mMaxDepth + 1);
			Array<float> bootstrapWeights;
			bootstrapWeights.Init(0.0f, numBootstrapSamples);

			//for (int i = 0; i < mNumBootstrap; i++)
			parallel_for(0, mNumBootstrap, [&](int i)
			{
				RandomGen random(i);
				MemoryPool memory;

				for (int depth = 0; depth <= mMaxDepth; depth++)
				{
					if (mTaskSync.Aborted())
						return;

					int rndSeed = depth + i * (mMaxDepth + 1);
					MetropolisSampler sampler(mSigma,
						mLargeStepProb,
						rndSeed);

					Vector2 rasterPos;
					bootstrapWeights[rndSeed] =
						EvalSample(pScene, &sampler, depth, &rasterPos, random, memory).Luminance();

					memory.FreeAll();
				}
			});

			if (mTaskSync.Aborted())
				return;

			Sampling::Distribution1D bootstrapDist(bootstrapWeights.Data(), bootstrapWeights.Size());
			float b = bootstrapDist.GetIntegral() * (mMaxDepth + 1);

			// Mutations per chain roughly equals to samples per pixel
			float mutationsPerPixel = mJobDesc.SamplesPerPixel;
			uint64 numTotalMutations = mutationsPerPixel * mpFilm->GetPixelCount();
			uint64 totalSamples = 0;

			//for (int i = 0; i < mNumChains; i++)
			parallel_for(0, mNumChains, [&](int i)
			{
				if (mTaskSync.Aborted())
					return;

				uint64 numChainMutations =
					Math::Min((i + 1) * numTotalMutations / mNumChains, numTotalMutations) -
					i * numTotalMutations / mNumChains;

				RandomGen random(i);
				MemoryPool memory;

				int bootstrapIndex = bootstrapDist.SampleDiscrete(random.Float(), nullptr);
				int depth = bootstrapIndex % (mMaxDepth + 1);

				// Initialize local variables for selected state
				MetropolisSampler sampler(mSigma,
					mLargeStepProb,
					bootstrapIndex);

				Vector2 currentRaster;
				Color currentLum = EvalSample(pScene, &sampler, depth, &currentRaster, random, memory);

				// Run the Markov chain for numChainMutations steps
				for (uint64 j = 0; j < numChainMutations; j++)
				{
					if (mTaskSync.Aborted())
						return;

					sampler.StartIteration();

					Vector2 proposedRaster;
					Color proposedLum = EvalSample(pScene, &sampler, depth, &proposedRaster, random, memory);

					// Compute acceptance probability for proposed sample
					float acceptProb = std::min(1.0f, proposedLum.Luminance() / (currentLum.Luminance() + 1e-4f));

					if (acceptProb > 0.0f)
					{
						mpFilm->Splat(proposedRaster.x, proposedRaster.y, proposedLum * acceptProb / (proposedLum.Luminance() + 1e-4f));
					}

					mpFilm->Splat(currentRaster.x, currentRaster.y, currentLum * (1 - acceptProb) / (currentLum.Luminance() + 1e-4f));

					// Accept or reject the proposal
					if (random.Float() < acceptProb)
					{
						currentRaster = proposedRaster;
						currentLum = proposedLum;
						sampler.Accept();
					}
					else
						sampler.Reject();

					memory.FreeAll();

					// Progressive display
					{
						ScopeLock lock(&mCS);

						totalSamples += 1;

						if (totalSamples % mpFilm->GetPixelCount() == 0)
						{
							mpFilm->IncreSampleCount();

							float currentMutationsPerPixel = (totalSamples / float(mpFilm->GetPixelCount()));
							mpFilm->ScaleToPixel(currentMutationsPerPixel / b);
						}
					}
				}
			//}
			});

			mpFilm->ScaleToPixel(mutationsPerPixel / b);
		}

		Color MultiplexedMLTIntegrator::EvalSample(const Scene* pScene,
			MetropolisSampler* pSampler,
			const int connectDepth,
			Vector2* pRasterPos,
			RandomGen& random,
			MemoryPool& memory) const
		{
			pSampler->StartStream(0);

			int lightLength, eyeLength, numStrategies;
			if (connectDepth == 0)
			{
				numStrategies = 1;
				lightLength = 0;
				eyeLength = 2;
			}
			else
			{
				numStrategies = connectDepth + 2;
				lightLength = Math::Min(int(pSampler->Get1D() * float(numStrategies)), numStrategies - 1);
				eyeLength = numStrategies - lightLength;
			}

			// Generate light sub-path
			BidirPathTracingIntegrator::PathVertex* pLightPath = memory.Alloc<BidirPathTracingIntegrator::PathVertex>(lightLength);
			int numLightVertex;
			if (BidirPathTracingIntegrator::GenerateLightPath(pScene, pSampler, lightLength, pLightPath, mpCamera, mpFilm, &numLightVertex, false, random, -1) != lightLength)
				return Color::BLACK;

			// Generate primary ray
			pSampler->StartStream(1);
			CameraSample camSample;

			*pRasterPos = pSampler->Get2D() * Vector2(mJobDesc.ImageWidth, mJobDesc.ImageHeight);
			pSampler->GenerateSamples(pRasterPos->x, pRasterPos->y, &camSample, random);
			camSample.imageX = pRasterPos->x;
			camSample.imageY = pRasterPos->y;

			RayDifferential ray;
			Color L = Color::BLACK;
			if (!mpCamera->GenRayDifferential(camSample, &ray))
				return Color::BLACK;

			// Initialize the camera PathState
			BidirPathTracingIntegrator::PathState cameraPathState;
			DifferentialGeom diffGeomCam;
			const Light* pEnvLight = nullptr;
			BidirPathTracingIntegrator::SampleCamera(pScene, ray, mpCamera, mpFilm, cameraPathState);

			// Trace camera sub-path
			while (eyeLength > 1)
			{
				RayDifferential pathRay = RayDifferential(cameraPathState.Origin, cameraPathState.Direction);

				if (!pScene->Intersect(pathRay, &diffGeomCam))
				{
					pEnvLight = pScene->GetEnvironmentLight();
					if (pEnvLight)
						cameraPathState.PathLength++;

					break;
				}

				pScene->PostIntersect(pathRay, &diffGeomCam);

				// Update MIS quantities before storing the vertex
				float cosIn = Math::AbsDot(diffGeomCam.mNormal, -pathRay.mDir);
				cameraPathState.DVCM *= BidirPathTracingIntegrator::MIS(diffGeomCam.mDist * diffGeomCam.mDist);
				cameraPathState.DVCM /= BidirPathTracingIntegrator::MIS(cosIn);
				cameraPathState.DVC /= BidirPathTracingIntegrator::MIS(cosIn);

				if (++cameraPathState.PathLength >= eyeLength)
					break;

				if (!BidirPathTracingIntegrator::SampleScattering(pScene, pathRay, diffGeomCam, pSampler->GetSample(), cameraPathState, random, -1))
					break;
			}

			Color Ret = Color::BLACK;

			// Connect eye sub-path and light sub-path
			if (cameraPathState.PathLength == eyeLength)
			{
				pSampler->StartStream(2);

				if (lightLength == 0)
				{
					auto pLight = diffGeomCam.GetAreaLight() ? diffGeomCam.GetAreaLight() : pEnvLight;

					if (pLight)
					{
						Ret = cameraPathState.Throughput *
							BidirPathTracingIntegrator::HittingLightSource(pScene,
								Ray(cameraPathState.Origin, cameraPathState.Direction),
								diffGeomCam,
								pLight,
								cameraPathState,
								random);
					}
				}
				else if (eyeLength == 1)
				{
					if (numLightVertex > 0)
					{
						const BidirPathTracingIntegrator::PathVertex& lightVertex = pLightPath[numLightVertex - 1];

						if (lightVertex.PathLength == lightLength)
						{
							Vector3 rasterPos;
							Ret = BidirPathTracingIntegrator::ConnectToCamera(pScene,
								lightVertex.DiffGeom,
								lightVertex,
								mpCamera,
								pSampler,
								mpFilm,
								&rasterPos,
								random);

							*pRasterPos = Vector2(rasterPos.x, rasterPos.y);
						}
					}
				}
				else if (lightLength == 1)
				{
					if (diffGeomCam.mpBSDF && !diffGeomCam.mpBSDF->IsSpecular())
					{
						// Connect to light source
						Ret = cameraPathState.Throughput *
							BidirPathTracingIntegrator::ConnectToLight(pScene,
								Ray(cameraPathState.Origin, cameraPathState.Direction),
								diffGeomCam,
								pSampler,
								cameraPathState,
								random);
					}
				}
				else
				{
					if (numLightVertex > 0 && diffGeomCam.mpBSDF && !diffGeomCam.mpBSDF->IsSpecular())
					{
						const BidirPathTracingIntegrator::PathVertex& lightVertex = pLightPath[numLightVertex - 1];

						Ret = lightVertex.Throughput * cameraPathState.Throughput *
							BidirPathTracingIntegrator::ConnectVertex(pScene,
								diffGeomCam,
								lightVertex,
								cameraPathState,
								random);
					}
				}

				Ret *= numStrategies;
			}

			return Ret;
		}
	}
}