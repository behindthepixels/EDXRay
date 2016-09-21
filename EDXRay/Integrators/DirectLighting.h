#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"
#include "../Core/Integrator.h"


namespace EDX
{
	namespace RayTracer
	{
		class DirectLightingIntegrator : public Integrator
		{
		private:
			uint mMaxDepth;
			SampleOffsets* mpLightSampleOffsets;
			SampleOffsets* mpBSDFSampleOffsets;

		public:
			DirectLightingIntegrator(int depth)
				: mMaxDepth(depth)
				, mpLightSampleOffsets(nullptr)
				, mpBSDFSampleOffsets(nullptr)
			{
			}
			~DirectLightingIntegrator();

		public:
			Color Li(const RayDifferential& ray, const Scene* pScene, Sampler* pSampler, RandomGen& random, MemoryPool& memory) const;
			void RequestSamples(const Scene* pScene, SampleBuffer* pSamples);
		};
	}
}