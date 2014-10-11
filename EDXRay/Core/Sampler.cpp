#include "Sampler.h"

#include "Memory/Memory.h"
#include "RNG/Random.h"

namespace EDX
{
	namespace RayTracer
	{
		void SampleBuffer::Validate()
		{
			SafeDelete(p1D);
			SafeDelete(p2D);

			if (count1D > 0)
				p1D = new float[count1D];
			if (count2D > 0)
				p2D = new Vector2[count2D];
		}


		Sample::Sample(RandomGen& random)
		{
			u = random.Float();
			v = random.Float();
			w = random.Float();
		}
	}
}