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
			vector<RefPtr<Light>>		mLights;
			RefPtr<BVH2>				mAccel;

		public:
			Scene();

			// Tracing methods
			bool Intersect(const Ray& ray, Intersection* pIsect) const;
			bool Occluded(const Ray& ray) const;

			void PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const;

			// Scene management
			void AddPrimitive(Primitive* pPrim);
			void AddLight(Light* pLight);
			const vector<RefPtr<Primitive>>& GetPrimitives() const { return mPrimitives; }
			const vector<RefPtr<Light>>& GetLights() const { return mLights; }

			void InitAccelerator();
		};
	}
}