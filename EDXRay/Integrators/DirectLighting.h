#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"
#include "../Core/Integrator.h"


namespace EDX
{
	namespace RayTracer
	{
		class DirectLightingIntegrator : public TiledIntegrator
		{
		private:
			uint mMaxDepth;

		public:
			DirectLightingIntegrator(int depth, const RenderJobDesc& jobDesc, const TaskSynchronizer& taskSync)
				: TiledIntegrator(jobDesc, taskSync)
				, mMaxDepth(depth)
			{
			}
			~DirectLightingIntegrator();

		public:
			Color Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryPool& memory) const;
		};
	}
}