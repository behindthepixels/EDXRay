#pragma once

#include "EDXPrerequisites.h"

#include "Math/Vector.h"
#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		struct CameraSample
		{
			float imageX, imageY;
			float lensU, lensV;
			float time;
		};

		struct SampleBuffer : public CameraSample
		{
			float* p1D;
			Vector2* p2D;

			int count1D, count2D;

			SampleBuffer()
				: count1D(0)
				, count2D(0)
			{
			}

			inline int Request1DArray(int count) { count1D += count; return count1D - 1; }
			inline int Request2DArray(int count) { count2D += count; return count2D - 1; }
			void Validate();
		};

		struct Sample
		{
			float u, v, w;

			Sample()
				: u(0.0f)
				, v(0.0f)
				, w(0.0f)
			{
			}

			Sample(RandomGen& random);
		};

		class Sampler
		{
		private:
			SampleBuffer mSample;

		public:
			virtual ~Sampler() {}

			const SampleBuffer& GetConfigSample() const
			{
				return mSample;
			}
			virtual void GenerateSamples(SampleBuffer* pSamples, RandomGen& random) = 0;
		};
	}
}