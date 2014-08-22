#include "Scene.h"
#include "Primitive.h"
#include "../Tracer/BVH.h"
#include "TriangleMesh.h"
#include "Light.h"
#include "DifferentialGeom.h"

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

		void Scene::PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const
		{
			assert(pDiffGeom);
			mPrimitives[pDiffGeom->mPrimId]->GetMesh()->PostIntersect(ray, pDiffGeom);
		}

		void Scene::AddPrimitive(Primitive* pPrim)
		{
			mPrimitives.push_back(pPrim);
		}

		void Scene::AddLight(Light* pLight)
		{
			mLights.push_back(pLight);
		}

		void Scene::InitAccelerator()
		{
			mAccel = new BVH2();
			mAccel->Construct(mPrimitives);
		}
	}
}