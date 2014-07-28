#pragma once

#include "EDXPrerequisites.h"

namespace EDX
{
	namespace RayTracer
	{
		class Intersection
		{
		public:
			int		mPrimId;
			float	mU, mV;
			float	mDist;
		};

		class DifferentialGeom : public Intersection
		{

		};
	}
}