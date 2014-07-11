#include "Sampler.h"

#include "Math/Vec2.h"
#include "Memory/Memory.h"
#include "RNG/Random.h"

namespace EDX
{
	namespace RayTracer
	{
		void Sample::Validate()
		{
			SafeDelete(p1D);
			SafeDelete(p2D);

			p1D = new float[count1D];
			p2D = new Vector2[count2D];
		}
	}
}