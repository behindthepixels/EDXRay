#pragma once

#include "EDXPrerequisites.h"

#include "../ForwardDecl.h"
#include "Memory/RefPtr.h"

namespace EDX
{
	namespace RayTracer
	{
		class Scene
		{
		private:
			vector<RefPtr<Primitive>>	mPrimitives;
			RefPtr<BVH2>				mAccel;

		public:
			Scene();

			bool Intersect(const Ray& ray, Intersection* pIsect) const;
			bool Occluded(const Ray& ray) const;

			void PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const;

			void AddPrimitive(Primitive* pPrim);

			void InitAccelerator();
		};
	}
}