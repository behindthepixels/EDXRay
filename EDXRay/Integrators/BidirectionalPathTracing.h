#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Integrator.h"


namespace EDX
{
	namespace RayTracer
	{
		class BidirPathTracingIntegrator : public Integrator
		{
		private:
			uint mMaxDepth;

		public:
			BidirPathTracingIntegrator(int depth)
				: mMaxDepth(depth)
			{
			}
			~BidirPathTracingIntegrator();

		public:
			Color Li(const RayDifferential& ray,
				const Scene* pScene,
				const SampleBuffer* pSamples,
				RandomGen& random,
				MemoryArena& memory) const;
			void RequestSamples(const Scene* pScene, SampleBuffer* pSampleBuf);
		};
	}
}