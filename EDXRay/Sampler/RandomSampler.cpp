
#include "RandomSampler.h"

namespace EDX
{
	namespace RayTracer
	{
		void RandomSampler::GenerateSamples(Sample* pSamples, RandomGen& random)
		{
			assert(pSamples);

			pSamples->imageX = random.Float();
			pSamples->imageY = random.Float();
			pSamples->lensU = random.Float();
			pSamples->lensV = random.Float();
			pSamples->time = random.Float();


		}
	}
}