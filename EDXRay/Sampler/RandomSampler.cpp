
#include "RandomSampler.h"

#include "Math/Vec2.h"
#include "RNG/Random.h"

namespace EDX
{
	namespace RayTracer
	{
		void RandomSampler::GenerateSamples(SampleBuffer* pSamples, RandomGen& random)
		{
			assert(pSamples);

			pSamples->imageX = random.Float();
			pSamples->imageY = random.Float();
			pSamples->lensU = random.Float();
			pSamples->lensV = random.Float();
			pSamples->time = random.Float();

			for (auto i = 0; i < pSamples->count1D; i++)
			{
				pSamples->p1D[i] = random.Float();
			}

			for (auto i = 0; i < pSamples->count2D; i++)
			{
				pSamples->p2D[i].u = random.Float();
				pSamples->p2D[i].v = random.Float();
			}
		}
	}
}