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

#if USE_EMBREE
#include "embree2/rtcore_ray.h"
#endif

namespace EDX
{
	namespace RayTracer
	{
		Scene::Scene()
			: mEnvMap(nullptr)
			, mDirty(true)
		{
		}

		Scene::~Scene()
		{
#if USE_EMBREE
			if(mpEmbreeDevice)
				rtcDeleteDevice(mpEmbreeDevice);

			if (mpEmbreeScene)
				rtcDeleteScene(mpEmbreeScene);
#endif
		}

		bool Scene::Intersect(const Ray& ray, Intersection* pIsect) const
		{
			Ray transformedRay = TransformRay(ray, mSceneScaleInv);

#if USE_EMBREE
			RTCRay embreeRay;
			embreeRay.org[0] = transformedRay.mOrg.x;
			embreeRay.org[1] = transformedRay.mOrg.y;
			embreeRay.org[2] = transformedRay.mOrg.z;
			embreeRay.dir[0] = transformedRay.mDir.x;
			embreeRay.dir[1] = transformedRay.mDir.y;
			embreeRay.dir[2] = transformedRay.mDir.z;
			embreeRay.tnear = transformedRay.mMin;
			embreeRay.tfar = transformedRay.mMax;
			embreeRay.time = 0.0f;
			embreeRay.mask = -1;
			embreeRay.geomID = RTC_INVALID_GEOMETRY_ID;
			embreeRay.primID = RTC_INVALID_GEOMETRY_ID;
			embreeRay.instID = RTC_INVALID_GEOMETRY_ID;

			rtcIntersect(mpEmbreeScene, embreeRay);

			if (embreeRay.geomID == RTC_INVALID_GEOMETRY_ID)
				return false;

			pIsect->mPrimId = embreeRay.geomID;
			pIsect->mTriId = embreeRay.primID;
			pIsect->mDist = embreeRay.tfar;
			pIsect->mU = embreeRay.u;
			pIsect->mV = embreeRay.v;

#else
			if (!mAccel->Intersect(transformedRay, pIsect))
				return false;

#endif // USE_EMBREE

			ray.mMax = pIsect->mDist;

			return true;
		}

		bool Scene::Occluded(const Ray& ray) const
		{
			Ray transformedRay = TransformRay(ray, mSceneScaleInv);

#if USE_EMBREE
			RTCRay embreeRay;
			embreeRay.org[0] = transformedRay.mOrg.x;
			embreeRay.org[1] = transformedRay.mOrg.y;
			embreeRay.org[2] = transformedRay.mOrg.z;
			embreeRay.dir[0] = transformedRay.mDir.x;
			embreeRay.dir[1] = transformedRay.mDir.y;
			embreeRay.dir[2] = transformedRay.mDir.z;
			embreeRay.tnear = transformedRay.mMin;
			embreeRay.tfar = transformedRay.mMax;
			embreeRay.time = 0.0f;
			embreeRay.geomID = RTC_INVALID_GEOMETRY_ID;
			embreeRay.primID = RTC_INVALID_GEOMETRY_ID;
			embreeRay.instID = RTC_INVALID_GEOMETRY_ID;

			rtcOccluded(mpEmbreeScene, embreeRay);

			return embreeRay.geomID != RTC_INVALID_GEOMETRY_ID;
#else
			return mAccel->Occluded(transformedRay);

#endif // USE_EMBREE
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
#if USE_EMBREE
			RTCBounds embreeBounds;
			rtcGetBounds(mpEmbreeScene, embreeBounds);

			BoundingBox worldBounds = BoundingBox(
					Vector3(embreeBounds.lower_x, embreeBounds.lower_y, embreeBounds.lower_z),
					Vector3(embreeBounds.upper_x, embreeBounds.upper_y, embreeBounds.upper_z)
				);

			return Matrix::TransformBBox(worldBounds, mSceneScale);

#else
			return Matrix::TransformBBox(mAccel->WorldBounds(), mSceneScale);

#endif // USE_EMBREE
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

#if USE_EMBREE
		void AlphaTest(void* userPtr, RTCRay& ray)
		{
			Primitive* prim = (Primitive*)userPtr;
			
			if (prim->GetBSDF(ray.primID)->GetTexture()->HasAlpha())
			{
				const TriangleMesh* mesh = prim->GetMesh();
				const Vector2& texcoord1 = mesh->GetTexCoordAt(3 * ray.primID);
				const Vector2& texcoord2 = mesh->GetTexCoordAt(3 * ray.primID + 1);
				const Vector2& texcoord3 = mesh->GetTexCoordAt(3 * ray.primID + 2);

				const Vector2 texCoord = (1.0f - ray.u - ray.v) * texcoord1 +
					ray.u * texcoord2 +
					ray.v * texcoord3;

				if (prim->GetBSDF(ray.primID)->GetTexture()->Sample(texCoord, nullptr, TextureFilter::Nearest).a == 0)
					ray.geomID = RTC_INVALID_GEOMETRY_ID; // reject hit
			}
		}
#endif // USE_EMBREE

		void Scene::InitAccelerator()
		{
			_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
			_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

#if USE_EMBREE
			mpEmbreeDevice = rtcNewDevice(nullptr);
			mpEmbreeScene = rtcDeviceNewScene(mpEmbreeDevice, RTC_SCENE_STATIC | RTC_SCENE_INCOHERENT | RTC_SCENE_HIGH_QUALITY, RTC_INTERSECT1);

			for (auto& it : mPrimitives)
			{
				uint geomID = rtcNewTriangleMesh(
					mpEmbreeScene,
					RTC_GEOMETRY_STATIC,
					it->GetMesh()->GetTriangleCount(),
					it->GetMesh()->GetVertexCount());

				rtcSetBuffer(mpEmbreeScene, geomID, RTC_VERTEX_BUFFER, it->GetMesh()->GetPositionBuffer(), 0, sizeof(Vector3));
				rtcSetBuffer(mpEmbreeScene, geomID, RTC_INDEX_BUFFER, it->GetMesh()->GetIndexBuffer(), 0, 3 * sizeof(uint));

				rtcSetIntersectionFilterFunction(mpEmbreeScene, geomID, (RTCFilterFunc)&AlphaTest);
				rtcSetOcclusionFilterFunction(mpEmbreeScene, geomID, (RTCFilterFunc)&AlphaTest);
				rtcSetUserData(mpEmbreeScene, geomID, it.Get());
			}

			rtcCommit(mpEmbreeScene);
#else
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
#endif // USE_EMBREE
		}

		void Scene::SetScale(const float scale)
		{
			mSceneScale = Matrix::Scale(scale, scale, scale);

			const float invScale = 1.0f / scale;
			mSceneScaleInv = Matrix::Scale(invScale, invScale, invScale);
		}
	}
}