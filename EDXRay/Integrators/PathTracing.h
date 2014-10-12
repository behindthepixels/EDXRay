#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Integrator.h"


namespace EDX
{
	namespace RayTracer
	{
		class PathTracingIntegrator : public Integrator
		{
		private:
			uint mMaxDepth;
			SampleOffsets* mpLightSampleOffsets;
			SampleOffsets* mpBSDFSampleOffsets;
			SampleOffsets* mpScatterOffsets;

		public:
			PathTracingIntegrator(int depth)
				: mMaxDepth(depth)
			{
			}
			~PathTracingIntegrator();

		public:
			Color Li(const RayDifferential& ray, const Scene* pScene, const SampleBuffer* pSamples, RandomGen& random, MemoryArena& memory) const;
			void RequestSamples(const Scene* pScene, SampleBuffer* pSampleBuf);
		};
	}
}