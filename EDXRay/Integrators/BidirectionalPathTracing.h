#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Integrator.h"
#include "../Core/Sampler.h"


namespace EDX
{
	namespace RayTracer
	{
		class BidirPathTracingIntegrator : public Integrator
		{
		private:
			struct PathState;
			struct PathVertex;

		private:
			const Camera* mpCamera;
			Film* mpFilm;
			uint mMaxDepth;
			float mLightPathCount;
			bool mNoDirectLight;
			bool mConnectToCamera;

			uint mLightIdSampleOffset;
			uint mLightConnectIdSampleOffset;
			SampleOffsets mLightEmitSampleOffsets;
			SampleOffsets mLightPathSampleOffsets;
			SampleOffsets mLightConnectSampleOffsets;
			SampleOffsets mCameraPathSampleOffsets;

		public:
			BidirPathTracingIntegrator(int depth, const Camera* pCam, Film* pFilm)
				: mMaxDepth(depth)
				, mpCamera(pCam)
				, mpFilm(pFilm)
				, mConnectToCamera(true)
				, mNoDirectLight(false)
			{
			}
			~BidirPathTracingIntegrator();

		public:
			Color Li(const RayDifferential& ray,
				const Scene* pScene,
				const SampleBuffer* pSamples,
				RandomGen& random,
				MemoryArena& memory) const override;
			void RequestSamples(const Scene* pScene, SampleBuffer* pSampleBuf) override;

		private:
			PathState SampleLightSource(const Scene* pScene,
				const SampleBuffer* pSamples,
				RandomGen& random) const;
			int GenerateLightPath(const Scene* pScene,
				const SampleBuffer* pSamples,
				PathVertex* pPath,
				RandomGen& random,
				const DifferentialGeom* diffGeom = nullptr,
				Color* pColorConnectToCam = nullptr) const;
			Color ConnectToCamera(const Scene* pScene,
				const DifferentialGeom& diffGeom,
				const SampleBuffer* pSamples,
				const PathState& pathState,
				RandomGen& random,
				const DifferentialGeom* pSurfDiffGeom) const;
			void SampleCamera(const Scene* pScene,
				const RayDifferential& primRay,
				PathState& initPathState,
				const DifferentialGeom* pSurfDiffGeom = nullptr) const;
			Color ConnectToLight(const Scene* pScene,
				const RayDifferential& pathRay,
				const DifferentialGeom& diffGeom,
				const SampleBuffer* pSamples,
				const PathState& cameraPathState,
				RandomGen& random) const;
			Color HittingLightSource(const Scene* pScene,
				const RayDifferential& pathRay,
				const DifferentialGeom& diffGeom,
				const SampleBuffer* pSamples,
				const Light* pLight,
				const PathState& cameraPathState,
				RandomGen& random) const;
			Color ConnectVertex(const Scene* pScene,
				const DifferentialGeom& cameraDiffGeom,
				const SampleBuffer* pSamples,
				const PathVertex& lightVertex,
				const PathState& cameraState,
				RandomGen& random) const;
			bool SampleScattering(const Scene* pScene,
				const RayDifferential& rayIn,
				const DifferentialGeom& diffGeom,
				const Sample& bsdfSample,
				PathState& pathState,
				RandomGen& random) const;

			inline float MIS(float fVal) const { return fVal * fVal; } // Use power heuristic
		};
	}
}