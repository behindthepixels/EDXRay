#include "Scene.h"
#include "Primitive.h"
#include "../Tracer/BVH.h"

namespace EDX
{
	namespace RayTracer
	{
		Scene::Scene()
		{
		}

		bool Scene::Intersect(const Ray& ray, Intersection* pIsect) const
		{
			return mAccel->Intersect(ray, pIsect);
		}

		bool Scene::Occluded(const Ray& ray) const
		{
			return mAccel->Occluded(ray);
		}

		void Scene::AddPrimitive(Primitive* pPrim)
		{
			mPrimitives.push_back(pPrim);
		}

		void Scene::InitAccelerator()
		{
			mAccel = new BVH2();
			mAccel->Construct(mPrimitives);
		}
	}
}