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
			: mEnvMap(nullptr)
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
			mPrimitives[pDiffGeom->mPrimId]->PostIntersect(ray, pDiffGeom);
		}

		BoundingBox Scene::WorldBounds() const
		{
			return mAccel->WorldBounds();
		}

		void Scene::AddPrimitive(Primitive* pPrim)
		{
			mPrimitives.push_back(pPrim);
		}

		void Scene::AddLight(Light* pLight)
		{
			if (pLight->IsEnvironmentLight())
			{
				bool foundEnvLight = false;
				for (auto it : mLights)
				{
					if (it->IsEnvironmentLight())
					{
						foundEnvLight = true;
						it = pLight;
					}
				}
				if (!foundEnvLight)
					mLights.push_back(pLight);

				mEnvMap = pLight;
			}
			else
				mLights.push_back(pLight);
		}

		void Scene::InitAccelerator()
		{
			mAccel = new BVH2();
			mAccel->Construct(mPrimitives);
		}
	}
}