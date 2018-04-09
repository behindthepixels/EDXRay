#include "RLPathTracing.h"
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
		using RLRecordPtr = RLRecord*;

		Color RLPathTracingIntegrator::Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryPool& memory) const
		{
			Color L = Color::BLACK;
			Color pathThroughput = Color::WHITE;

			bool specBounce = true;
			RayDifferential pathRay = ray;

			uint prevCellIndex = INDEX_NONE;
			RLRecord* pPrevRecord = nullptr;
			Color prevSurfAtten = Color::WHITE;

			for (auto bounce = 0; ; bounce++)
			{
				DifferentialGeom diffGeom;
				bool intersected = pScene->Intersect(pathRay, &diffGeom);

				if (pathThroughput.IsBlack())
					break;
				
				// Sampled surface
				pScene->PostIntersect(pathRay, &diffGeom);

				ShadingKey hashKey = SpatialHashing(diffGeom, pScene);
				RLRecord** pQuery;
				pQuery = mQTable.Find(hashKey);
				RLRecord* pRecord = pQuery != nullptr ? *pQuery : nullptr;
				if (!pRecord)
				{
					pRecord = new RLRecord();
					mQTable.Insert(hashKey, pRecord);
				}

				if (specBounce)
				{
					Color shadingContrib;
					if (intersected)
					{
						shadingContrib = diffGeom.Emit(-pathRay.mDir);
						L += pathThroughput * shadingContrib;
					}
					else
					{
						if (pScene->GetEnvironmentLight())
						{
							shadingContrib = pScene->GetEnvironmentLight()->Emit(-pathRay.mDir);
							L += pathThroughput * shadingContrib;
						}
					}

					//if (pPrevRecord)
					//{
					//	if (shadingContrib.Luminance() > 0.0f)
					//	{
					//		pPrevRecord->UpdateQ(
					//			(prevSurfAtten * shadingContrib).Luminance(),
					//			prevCellIndex);
					//	}
					//}
				}

				if (!intersected || bounce >= mMaxDepth)
					break;

				// Explicitly sample light sources
				const BSDF* pBSDF = diffGeom.mpBSDF;
				if (!pBSDF->IsSpecular())
				{
					float lightIdxSample = pSampler->Get1D();
					auto lightIdx = Math::Min(lightIdxSample * pScene->GetLights().Size(), pScene->GetLights().Size() - 1);
					Color shadingContrib = Integrator::EstimateDirectLighting(diffGeom, -pathRay.mDir, pScene->GetLights()[lightIdx].Get(), pScene, pSampler) * pScene->GetLights().Size();
					L += pathThroughput * shadingContrib;

					if (pPrevRecord)
					{
						float maxVal = Math::Max(shadingContrib.Luminance(), pRecord->GetValueFunc());
						if (maxVal > 0.0f)
						{
							pPrevRecord->UpdateQ(
								(prevSurfAtten * maxVal).Luminance(),
								prevCellIndex);
						}
					}
				}

				const Vector3& pos = diffGeom.mPosition;
				const Vector3& normal = diffGeom.mNormal;
				Vector3 vOut = -pathRay.mDir;
				Vector3 vIn;
				float pdf;
				ScatterType bsdfFlags;
				Sample scatterSample = pSampler->GetSample();
				vIn = pRecord->SampleScattered(scatterSample, diffGeom, &pdf, &prevCellIndex);

				Color f = pBSDF->Eval(vOut, vIn, diffGeom);
				if (f.IsBlack() || pdf == 0.0f)
					break;
				pathThroughput *= f * Math::AbsDot(vIn, normal) / pdf;

				specBounce = false;
				pathRay = Ray(pos, vIn, diffGeom.mMediumInterface.GetMedium(vIn, normal));

				// Russian Roulette
				if (bounce > 3)
				{
					float RR = Math::Min(1.0f, pathThroughput.Luminance());
					if (random.Float() > RR)
						break;

					pathThroughput /= RR;
				}

				// Store record info
				pPrevRecord = pRecord;
				prevSurfAtten = f * Math::AbsDot(vIn, normal);
			}

			return L;
		}

		// Maps the sphere to a unit square with a uniform distribution
		inline Vector2 UniformSphereToSquares(const Vector3& vec)
		{
			const float cosTheta = vec.y;
			float phi;
			if (Math::Abs(vec.y) >= 1.0f - 1.0e-5f)
			{
				phi = 0.0f;
			}
			else
			{
				phi = Math::Atan2(vec.z, vec.x);
				phi = phi < 0.0f ? phi + float(Math::EDX_TWO_PI) : phi;
			}
			return Vector2(phi / float(Math::EDX_TWO_PI), (cosTheta + 1.0f) * 0.5f);
		}

		uint ToFixedPoint(const float val, const uint bits)
		{
			const uint mask = (1 << bits) - 1;

			return Math::Min(Math::RoundToInt(val * mask), mask - 1);
		}

		ShadingKey RLPathTracingIntegrator::SpatialHashing(const DifferentialGeom& diffGeom, const Scene* pScene) const
		{
			ShadingKey ret;

			const BoundingBox sceneBounds = pScene->WorldBounds();
			//const float totalSceneArea = sceneBounds.Area() * 3.0f; // Simple heuristic

			const int axis = sceneBounds.MaximumExtent();
			const float maxBound = sceneBounds.mMax[axis] - sceneBounds.mMin[axis];
			const int density = 32;
			const float gridSize = maxBound / float(density);

			const Vector3 gridPos = Vector3i(
				Math::RoundToInt(diffGeom.mPosition.x / gridSize) * gridSize,
				Math::RoundToInt(diffGeom.mPosition.y / gridSize) * gridSize,
				Math::RoundToInt(diffGeom.mPosition.z / gridSize) * gridSize
			);

			const Vector3 scaledPos = (gridPos - sceneBounds.mMin) / maxBound * density;
			const Vector3i roundedPos = Vector3i(
				Math::RoundToInt(scaledPos.x),
				Math::RoundToInt(scaledPos.y),
				Math::RoundToInt(scaledPos.z)
			);

			static const int NORMAL_BITS = 2;

			const Vector2 normalUV = UniformSphereToSquares(diffGeom.mNormal);
			const Vector2i normalFixedPoint = Vector2i(ToFixedPoint(normalUV.u, NORMAL_BITS), ToFixedPoint(normalUV.v, NORMAL_BITS));

			const uint posMask = (1 << 16) - 1;
			const uint normalMask = (1 << NORMAL_BITS) - 1;

			ret = uint64(roundedPos.x & posMask) |
				(uint64(roundedPos.y & posMask) << 16) |
				(uint64(roundedPos.z & posMask) << 32) |
				(uint64(normalFixedPoint.u & normalMask) << 48) |
				(uint64(normalFixedPoint.v & normalMask) << 56);

			return ret;
		}
	}
}