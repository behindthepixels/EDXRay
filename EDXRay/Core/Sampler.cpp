#include "Sampler.h"

#include "Core/Memory.h"
#include "Core/Random.h"

namespace EDX
{
	namespace RayTracer
	{
		Sample::Sample(RandomGen& random)
		{
			u = random.Float();
			v = random.Float();
			w = random.Float();
		}
	}
}