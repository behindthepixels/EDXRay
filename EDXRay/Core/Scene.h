#pragma once

#include "EDXPrerequisites.h"

#include "../ForwardDecl.h"
#include "Primitive.h"
#include "Memory/RefPtr.h"

namespace EDX
{
	namespace RayTracer
	{
		class Scene
		{
		private:
			vector<Primitive>	mPrimitives;
			RefPtr<BVH4>			mBVH;

		public:
			bool Intersect(const Ray& ray, Intersection* pIsect) const;
			bool Occluded(const Ray& ray) const;
		};
	}
}