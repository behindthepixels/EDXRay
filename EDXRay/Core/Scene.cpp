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
			bool hit = false;
			for (auto i = 0; i < mPrimitives.size(); i++)
			{
				auto it = mPrimitives[i].Ptr();
				if (it->Intersect(ray, pIsect))
					hit = true;
			}

			return hit;
		}

		void Scene::AddPrimitive(Primitive* pPrim)
		{
			mPrimitives.push_back(pPrim);
		}

		void Scene::InitAccelerator()
		{
			mAccel = new BVH2();
			mAccel->Build(mPrimitives);
		}
	}
}