#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Integrator.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "../Core/SpatialHashMap.h"

namespace EDX
{
	namespace RayTracer
	{
		using ShadingKey = uint64;

		struct RLRecord
		{
			static const int NUM_X = 16;
			static const int NUM_Y = 8;
			static const int NUM_XY = NUM_X * NUM_Y;

			float mDirectionalDensity[NUM_XY];
			float mMaxQ;
			Sampling::Distribution1D mPdf;

			mutable CriticalSection mLock;
			
			RLRecord()
			{
				// Initialize directinal density as consine weighted diffuse
				for (int y = 0; y < NUM_Y; y++)
				{
					for (int x = 0; x < NUM_X; x++)
					{
						const float initDensity = y / float(NUM_Y);
						mDirectionalDensity[x + y * NUM_X] = initDensity;
					}
				}

				mPdf.SetFunction(mDirectionalDensity, NUM_XY);
				mMaxQ = 0.0f;
			}

			void UpdateValueFunc(const float value)
			{
				mLock.Lock();
				{
					mMaxQ = Math::Max(mMaxQ, value);
				}
				mLock.Unlock();
			}

			const float GetValueFunc() const
			{
				return mMaxQ;
			}

			void UpdateQ(const float QVal, const uint cellIdx)
			{
				const float alpha = 0.85f;

				mLock.Lock();
				{
					mDirectionalDensity[cellIdx] = (1.0f - alpha) * mDirectionalDensity[cellIdx] + alpha * QVal;
					mMaxQ = Math::Max(mMaxQ, mDirectionalDensity[cellIdx]);
					mPdf.SetFunction(mDirectionalDensity, NUM_XY);
				}
				mLock.Unlock();
			}

			Vector3 SampleScattered(const Sample& sample, const DifferentialGeom& diffGeom, float* pPdf, uint* cellIdx) const
			{
				Vector3 localWi;

				float cellPdf;

				mLock.Lock();
				*cellIdx = mPdf.SampleDiscrete(sample.u, &cellPdf);
				mLock.Unlock();

				const int thetaIdx = *cellIdx / NUM_X;
				const int phiIdx = *cellIdx % NUM_X;
				const float u = ((float)thetaIdx + sample.v) / NUM_Y;
				const float v = ((float)phiIdx + sample.w) / NUM_X;

				localWi = Vector3(
					Math::Sqrt(1.0f - u * u) * Math::Cos(float(Math::EDX_TWO_PI) * v),
					Math::Sqrt(1.0f - u * u) * Math::Sin(float(Math::EDX_TWO_PI) * v),
					u);

				Vector3 Wo = diffGeom.LocalToWorld(localWi);
				*pPdf = (NUM_XY * cellPdf / float(Math::EDX_TWO_PI)); // Solid angle probability
				//*pPdf = BSDFCoordinate::AbsCosTheta(localWi) * float(Math::EDX_INV_PI); // Solid angle probability

				return Wo;
			}
		};

		class RLPathTracingIntegrator : public TiledIntegrator
		{
		private:
			uint mMaxDepth;
			mutable SpatialHashMap<ShadingKey, RLRecord*> mQTable;

		public:
			RLPathTracingIntegrator(int depth, const int qTableSize, const RenderJobDesc& jobDesc, const TaskSynchronizer& taskSync)
				: TiledIntegrator(jobDesc, taskSync)
				, mMaxDepth(depth)
				, mQTable(qTableSize)
			{
			}
			~RLPathTracingIntegrator()
			{
			}

		public:
			Color Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryPool& memory) const;

		private:
			ShadingKey SpatialHashing(const DifferentialGeom& diffGeom, const Scene* pScene) const;
		};
	}
}