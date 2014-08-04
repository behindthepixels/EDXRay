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

			Intersection()
				: mDist(float(Math::EDX_INFINITY))
			{
			}
		};

		class DifferentialGeom : public Intersection
		{

		};
	}
}