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
		public:
			virtual ~Sampler() {}

			virtual void GenerateSamples(
				const int pixelX,
				const int pixelY,
				CameraSample* pSamples,
				RandomGen& random) = 0;
			virtual void AdvanceSampleIndex() {}

			virtual void StartPixel(const int pixelX, const int pixelY) {}
			virtual float Get1D() = 0;
			virtual Vector2 Get2D() = 0;
			virtual Sample GetSample() = 0;

			virtual UniquePtr<Sampler> Clone(const int seed) const = 0;
		};
	}
}