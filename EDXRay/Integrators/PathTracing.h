#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Integrator.h"
#include "../Core/Sampler.h"


namespace EDX
{
	namespace RayTracer
	{
		class PathTracingIntegrator : public TiledIntegrator
		{
		private:
			uint mMaxDepth;

		public:
			PathTracingIntegrator(int depth, const RenderJobDesc& jobDesc, const TaskSynchronizer& taskSync)
				: TiledIntegrator(jobDesc, taskSync)
				, mMaxDepth(depth)
			{
			}
			~PathTracingIntegrator()
			{
			}

		public:
			Color Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryPool& memory) const;
		};
	}
}