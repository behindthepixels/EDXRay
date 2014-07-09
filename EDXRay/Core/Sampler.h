#pragma once

#include "EDXPrerequisites.h"

#include "../ForwardDecl.h"
#include "Math/Vec2.h"

#include "RNG/Random.h"

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

		struct Sample1D
		{
			float u;
		};
		struct Sample1DArray
		{
			Sample1D*	pVals;
			uint		size;

			Sample1DArray(int _size) : size(_size) { pVals = new Sample1D[size]; }
			~Sample1DArray() {}
		};

		struct Sample2D
		{
			float u, v;
		};
		struct Sample2DArray
		{
			Sample2D*	pVals;
			uint		size;

			Sample2DArray(int _size) : size(_size) { pVals = new Sample2D[size]; }
			~Sample2DArray() {}
		};

		struct Sample : public CameraSample
		{
			vector<Sample1DArray> samples1D;
			vector<Sample2DArray> samples2D;

			inline int Request1DArray(int iCount) { samples1D.push_back(Sample1DArray(iCount)); return samples1D.size() - 1; }
			inline int Request2DArray(int iCount) { samples2D.push_back(Sample2DArray(iCount)); return samples2D.size() - 1; }
		};

		class Sampler
		{
		public:
			virtual void GenerateSamples(Sample* pSamples, RandomGen& random) = 0;
		};
	}
}