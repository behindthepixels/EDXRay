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
			Ray transformedRay = TransformRay(ray, mSceneScaleInv);

			if (!mAccel->Intersect(transformedRay, pIsect))
				return false;

			ray.mMax = pIsect->mDist;

			return true;
		}

		bool Scene::Occluded(const Ray& ray) const
		{
			Ray transformedRay = TransformRay(ray, mSceneScaleInv);

			return mAccel->Occluded(transformedRay);
		}

		void Scene::PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const
		{
			Assert(pDiffGeom);
			mPrimitives[pDiffGeom->mPrimId]->PostIntersect(ray, pDiffGeom);

			pDiffGeom->mPosition = Matrix::TransformPoint(pDiffGeom->mPosition, mSceneScale);
			pDiffGeom->mNormal = Math::Normalize(Matrix::TransformNormal(pDiffGeom->mNormal, mSceneScaleInv));
			pDiffGeom->mGeomNormal = Math::Normalize(Matrix::TransformNormal(pDiffGeom->mGeomNormal, mSceneScaleInv));
			pDiffGeom->mShadingFrame = Frame(pDiffGeom->mNormal);

			pDiffGeom->mDpdu = Matrix::TransformVector(pDiffGeom->mDpdu, mSceneScale);
			pDiffGeom->mDpdv = Matrix::TransformVector(pDiffGeom->mDpdv, mSceneScale);
			pDiffGeom->mDndu = Matrix::TransformVector(pDiffGeom->mDndu, mSceneScale);
			pDiffGeom->mDndv = Matrix::TransformVector(pDiffGeom->mDndv, mSceneScale);
		}

		BoundingBox Scene::WorldBounds() const
		{
			return Matrix::TransformBBox(mAccel->WorldBounds(), mSceneScale);
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

		void Scene::SetScale(const float scale)
		{
			mSceneScale = Matrix::Scale(scale, scale, scale);

			const float invScale = 1.0f / scale;
			mSceneScaleInv = Matrix::Scale(invScale, invScale, invScale);
		}
	}
}