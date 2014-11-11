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
				, p1D(nullptr)
				, p2D(nullptr)
			{
			}
			~SampleBuffer();

			inline int Request1DArray(int count) { int ret = count1D; count1D += count; return ret; }
			inline int Request2DArray(int count) { int ret = count2D; count2D += count; return ret; }

			SampleBuffer* Duplicate(uint count) const;
			void Validate();
		};

		struct SampleOffsets
		{
			uint offset1d, offset2d;

			SampleOffsets()
			{
			}
			SampleOffsets(uint count, SampleBuffer* pSampleBuf)
			{
				offset1d = pSampleBuf->Request1DArray(count);
				offset2d = pSampleBuf->Request2DArray(count);
			}
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
			Sample(const SampleOffsets& offsets, const SampleBuffer* pSampleBuf)
			{
				w = pSampleBuf->p1D[offsets.offset1d];
				u = pSampleBuf->p2D[offsets.offset2d].u;
				v = pSampleBuf->p2D[offsets.offset2d].v;
			}
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