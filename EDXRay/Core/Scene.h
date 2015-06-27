#pragma once

#include "EDXPrerequisites.h"

#include "Math/BoundingBox.h"
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
			Light*						mEnvMap;
			RefPtr<BVH2>				mAccel;
			bool						mDirty;

		public:
			Scene();

			// Tracing methods
			bool Intersect(const Ray& ray, Intersection* pIsect) const;
			void PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const;
			bool Occluded(const Ray& ray) const;

			BoundingBox WorldBounds() const;

			// Scene management
			void AddPrimitive(Primitive* pPrim);
			void AddLight(Light* pLight);

			// Scene elements getter
			const vector<RefPtr<Primitive>>& GetPrimitives() const { return mPrimitives; }
			const vector<RefPtr<Light>>& GetLights() const { return mLights; }
			const Light* GetLight(const uint id) const { return mLights[id].Ptr(); }
			const Light* GetEnvironmentMap() const { return mEnvMap; }
			uint GetNumLight() const { return mLights.size(); }

			const Light* ChooseLightSource(const float lightIdSample, float* pPdf) const
			{
				auto lightCount = mLights.size();
				auto lightId = Math::FloorToInt(lightIdSample * lightCount);
				if (lightId == lightCount)
					lightId = lightCount - 1;

				if(pPdf)
					*pPdf = 1.0f / float(lightCount);

				return GetLight(lightId);
			}
			float LightPdf(const Light* pLight) const
			{
				return 1.0f / float(mLights.size());
			}

			void InitAccelerator();
		};
	}
}