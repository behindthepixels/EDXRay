#pragma once

#include "EDXPrerequisites.h"

#include "Math/BoundingBox.h"
#include "Math/Matrix.h"
#include "../ForwardDecl.h"

#define USE_EMBREE 1 && _WIN64 // Only supports embree in x64

#if USE_EMBREE
#define EMBREE_STATIC_LIB
#include "embree2/rtcore.h"
#endif // USE_EMBREE

namespace EDX
{
	namespace RayTracer
	{
		class Scene
		{
		private:
			Array<UniquePtr<Primitive>>	mPrimitives;
			Array<UniquePtr<Light>>		mLights;
			Light*						mEnvMap;
			UniquePtr<BVH2>				mAccel;
			bool						mDirty;
			Array<UniquePtr<const Medium>> mMedia;

			Matrix						mSceneScale;
			Matrix						mSceneScaleInv;

#if USE_EMBREE
			// Embree
			RTCDevice mpEmbreeDevice = nullptr;
			RTCScene mpEmbreeScene = nullptr;
#endif // USE_EMBREE

		public:
			Scene();
			~Scene();

			// Tracing methods
			bool Intersect(const Ray& ray, Intersection* pIsect) const;
			void PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const;
			bool Occluded(const Ray& ray) const;

			BoundingBox WorldBounds() const;

			// Scene management
			void AddPrimitive(Primitive* pPrim);
			void AddLight(Light* pLight);

			// Scene elements getter
			const Array<UniquePtr<Primitive>>& GetPrimitives() const { return mPrimitives; }
			const Array<UniquePtr<Light>>& GetLights() const { return mLights; }
			const Light* GetLight(const uint id) const { return mLights[id].Get(); }
			const Light* GetEnvironmentMap() const { return mEnvMap; }
			uint GetNumLight() const { return mLights.Size(); }

			const Light* ChooseLightSource(const float lightIdSample, float* pPdf) const
			{
				auto lightCount = mLights.Size();
				auto lightId = Math::FloorToInt(lightIdSample * lightCount);
				if (lightId == lightCount)
					lightId = lightCount - 1;

				if (pPdf)
					*pPdf = 1.0f / float(lightCount);

				return GetLight(lightId);
			}
			float LightPdf(const Light* pLight) const
			{
				return 1.0f / float(mLights.Size());
			}

			void InitAccelerator();

			void SetScale(const float scale);
			const Matrix& GetScaleMatrix() const
			{
				return mSceneScale;
			}
		};
	}
}