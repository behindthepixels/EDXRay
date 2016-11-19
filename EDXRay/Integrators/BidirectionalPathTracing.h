#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Integrator.h"
#include "../Core/Sampler.h"


namespace EDX
{
	namespace RayTracer
	{
		class BidirPathTracingIntegrator : public TiledIntegrator
		{
		public:
			struct PathState
			{
				Vector3 Origin;             // Path origin
				Vector3 Direction;          // Where to go next
				Color Throughput;         // Path throughput
				uint  PathLength : 30; // Number of path segments, including this
				bool  IsFiniteLight : 1; // Just generate by finite light
				bool  SpecularPath : 1; // All scattering events so far were specular

				float DVCM; // MIS quantity used for vertex connection and merging
				float DVC;  // MIS quantity used for vertex connection
			};

			struct PathVertex
			{
				Color Throughput; // Path throughput (including emission)
				uint  PathLength; // Number of segments between source and vertex

				// Stores all required local information, including incoming direction.
				DifferentialGeom DiffGeom;
				Vector3 InDir;

				float DVCM; // MIS quantity used for vertex connection and merging
				float DVC;  // MIS quantity used for vertex connection
			};

		private:
			const Camera* mpCamera;
			Film* mpFilm;
			uint mMaxDepth;

		public:
			BidirPathTracingIntegrator(int depth, const Camera* pCam, Film* pFilm, const RenderJobDesc& jobDesc, const TaskSynchronizer& taskSync)
				: TiledIntegrator(jobDesc, taskSync)
				, mMaxDepth(depth)
				, mpCamera(pCam)
				, mpFilm(pFilm)
			{
			}
			~BidirPathTracingIntegrator();

		public:
			Color Li(const RayDifferential& ray,
				const Scene* pScene,
				Sampler* pSampler,
				RandomGen& random,
				MemoryPool& memory) const override;

		public:
			static PathState SampleLightSource(const Scene* pScene,
				Sampler* pSampler,
				RandomGen& random);
			static int GenerateLightPath(const Scene* pScene,
				Sampler* pSampler,
				const int maxDepth,
				PathVertex* pPath,
				const Camera* pCamera,
				Film* pFilm,
				int* pVertexCount,
				const bool bConnectToCamera,
				RandomGen& random,
				const int RRDepth = 3);
			static Color ConnectToCamera(const Scene* pScene,
				const DifferentialGeom& diffGeom,
				const PathVertex& pathVertex,
				const Camera* pCamera,
				Sampler* pSampler,
				Film* pFilm,
				Vector3* pRasterPos,
				RandomGen& random);
			static void SampleCamera(const Scene* pScene,
				const RayDifferential& primRay,
				const Camera* pCamera,
				Film* pFilm,
				PathState& initPathState);
			static Color ConnectToLight(const Scene* pScene,
				const RayDifferential& pathRay,
				const DifferentialGeom& diffGeom,
				Sampler* pSampler,
				const PathState& cameraPathState,
				RandomGen& random);
			static Color HittingLightSource(const Scene* pScene,
				const RayDifferential& pathRay,
				const DifferentialGeom& diffGeom,
				const Light* pLight,
				const PathState& cameraPathState,
				RandomGen& random);
			static Color ConnectVertex(const Scene* pScene,
				const DifferentialGeom& cameraDiffGeom,
				const PathVertex& lightVertex,
				const PathState& cameraState,
				RandomGen& random);
			static bool SampleScattering(const Scene* pScene,
				const RayDifferential& rayIn,
				const DifferentialGeom& diffGeom,
				const Sample& bsdfSample,
				PathState& pathState,
				RandomGen& random,
				const int RRDepth = -1);

			inline static float MIS(float fVal)
			{
				// Use power heuristic
				return fVal * fVal;
			}
		};
	}
}