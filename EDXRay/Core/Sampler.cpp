#include "Sampler.h"

#include "Memory/Memory.h"
#include "RNG/Random.h"

namespace EDX
{
	namespace RayTracer
	{
		SampleBuffer* SampleBuffer::Duplicate() const
		{
			SampleBuffer* pRet = new SampleBuffer;

			pRet->count1D = count1D;
			pRet->count2D = count2D;
			pRet->Validate();

			return pRet;
		}

		void SampleBuffer::Validate()
		{
			SafeDeleteArray(p1D);
			SafeDeleteArray(p2D);

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