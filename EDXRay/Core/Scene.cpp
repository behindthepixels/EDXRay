#include "Scene.h"
#include "Primitive.h"
#include "../Tracer/BVH.h"
#include "TriangleMesh.h"
#include "Light.h"
#include "../Lights/AreaLight.h"
#include "DifferentialGeom.h"
#include "Medium.h"

namespace EDX
{
	namespace RayTracer
	{
		Scene::Scene()
			: mEnvMap(nullptr)
			, mDirty(true)
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
			for (const auto& it : pPrim->mMediumInterfaces)
			{
				if (auto ptr = it.GetInside())
					mMedia.push_back(ptr);
				if (auto ptr = it.GetOutside())
					mMedia.push_back(ptr);
			}
			mDirty = true;
		}

		void Scene::AddLight(Light* pLight)
		{
			if (pLight->IsEnvironmentLight())
			{
				bool foundEnvLight = false;
				for (auto& it : mLights)
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
			else if (pLight->IsAreaLight())
			{
				mPrimitives.push_back(((AreaLight*)pLight)->GetPrimitive());
				mLights.push_back(pLight);
			}
			else
				mLights.push_back(pLight);
		}

		void Scene::InitAccelerator()
		{
			if (mDirty)
			{
				mAccel = new BVH2();
				mAccel->Construct(mPrimitives);

				mDirty = false;
			}
		}
	}
}