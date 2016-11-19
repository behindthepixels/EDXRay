#pragma once

#include "EDXPrerequisites.h"
#include "BidirectionalPathTracing.h"
#include "../Core/Sampler.h"


namespace EDX
{
	namespace RayTracer
	{
		class MetropolisSampler : public Sampler
		{
		private:
			struct PrimarySample
			{
			public:
				float value = 0.0f;
				uint64 modifiedIteration = 0u;

				float backupValue = 0.0f;
				uint64 backupModifiedIteration = 0u;

				void Backup()
				{
					backupValue = value;
					backupModifiedIteration = modifiedIteration;
				}

				void Restore()
				{
					value = backupValue;
					modifiedIteration = backupModifiedIteration;
				}
			};

		private:
			Array<PrimarySample> mSamples;

			const float mSigma;
			const float mLargeStepProb;
			int mSampleIndex;
			int mStreamIndex;
			const int StreamCount = 3;
			uint64 mPrevLargeStepIteration = 0u;
			uint64 mCurrentIteration = 0u;
			bool mLargeStep = true;

			RandomGen mRandom;

		public:
			MetropolisSampler(const float sigma,
				const float largeStepProb,
				const int randomSeed)
				: mSigma(sigma)
				, mLargeStepProb(largeStepProb)
				, mRandom(randomSeed)
			{
			}

		public:
			float Get1D() override;
			Vector2 Get2D() override;
			Sample GetSample() override;
			void GenerateSamples(
				const int pixelX,
				const int pixelY,
				CameraSample* pSamples,
				RandomGen& random) override;
			UniquePtr<Sampler> Clone(const int seed) const override;

			void StartIteration();
			void Accept();
			void Reject();
			void Mutate(const int index);

			void StartStream(const int index)
			{
				Assert(index < StreamCount);
				mStreamIndex = index;
				mSampleIndex = 0;
			}

		private:
			int GetNextIndex()
			{
				return mStreamIndex + StreamCount * mSampleIndex++;
			}
		};

		class MultiplexedMLTIntegrator : public Integrator
		{
		private:
			const Camera* mpCamera;
			Film* mpFilm;
			uint mMaxDepth;

			int mNumBootstrap;
			int mNumChains;
			float mSigma;
			float mLargeStepProb;

			mutable CriticalSection mCS;

		public:
			MultiplexedMLTIntegrator(int depth, const Camera* pCam, Film* pFilm, const RenderJobDesc& jobDesc, const TaskSynchronizer& taskSync)
				: Integrator(jobDesc, taskSync)
				, mMaxDepth(depth)
				, mpCamera(pCam)
				, mpFilm(pFilm)
				, mNumBootstrap(1 << 17)
				, mNumChains(2048)
				, mSigma(0.01f)
				, mLargeStepProb(0.3f)
			{
			}
			~MultiplexedMLTIntegrator()
			{
			}

		public:
			virtual void Render(const Scene* pScene,
				const Camera* pCamera,
				Sampler* pSampler,
				Film* pFilm) const override;

		private:
			Color EvalSample(const Scene* pScene,
				MetropolisSampler* pSampler,
				const int connectDepth,
				Vector2* pRasterPos,
				RandomGen& random,
				MemoryPool& memory) const;
		};
	}
}