#include "BidirectionalPathTracing.h"
#include "../Core/Scene.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/BSDF.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "Math/Ray.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		Color BidirPathTracingIntegrator::Li(const RayDifferential& ray, const Scene* pScene, const SampleBuffer* pSampleBuf, RandomGen& random, MemoryArena& memory) const
		{
			return Color(0);
		}

		void BidirPathTracingIntegrator::RequestSamples(const Scene* pScene, SampleBuffer* pSampleBuf)
		{
			assert(pSampleBuf);
		}

		BidirPathTracingIntegrator::~BidirPathTracingIntegrator()
		{
		}
	}
}