#include "Scene.h"
#include "Primitive.h"
#include "../Tracer/BVH.h"
#include "../Tracer/BVHBuildTask.h"
#include "TriangleMesh.h"
#include "Light.h"
#include "../Lights/AreaLight.h"
#include "DifferentialGeom.h"
#include "Medium.h"

#include "Graphics/ObjMesh.h"

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
			Assert(pDiffGeom);
			mPrimitives[pDiffGeom->mPrimId]->PostIntersect(ray, pDiffGeom);
		}

		BoundingBox Scene::WorldBounds() const
		{
			return mAccel->WorldBounds();
		}

		void Scene::AddPrimitive(Primitive* pPrim)
		{
			mPrimitives.Add(UniquePtr<Primitive>(pPrim));
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
						it.Reset(pLight);
					}
				}
				if (!foundEnvLight)
					mLights.Add(UniquePtr<Light>(pLight));

				mEnvMap = pLight;
			}
			else if (pLight->IsAreaLight())
			{
				mPrimitives.Add(UniquePtr<Primitive>(((AreaLight*)pLight)->GetPrimitive()));
				mLights.Add(UniquePtr<Light>(pLight));
			}
			else
				mLights.Add(UniquePtr<Light>(pLight));
		}

		void Scene::InitAccelerator()
		{
			if (mDirty)
			{
				mAccel = MakeUnique<BVH2>();

				Array<Primitive*> primArray;
				for (const auto& it : mPrimitives)
				{
					primArray.Add(it.Get());
				}


				mAccel->Construct(primArray);

				mDirty = false;
			}
		}
	}
}