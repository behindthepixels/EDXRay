#pragma once

#include "EDXPrerequisites.h"

#include "Math/Vector.h"
#include "Math/Ray.h"
#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		class VisibilityTester
		{
		private:
			Ray mRay;

		public:
			void SetSegment(const Vector3 &pt1, const Vector3 &pt2)
			{
				float fDist = Math::Distance(pt1, pt2);
				mRay = Ray(pt1, (pt2 - pt1) / fDist, fDist, 0.0f, 0);
			}
			void SetRay(const Vector3 &pt, const Vector3 &vDir)
			{
				mRay = Ray(pt, vDir, float(Math::EDX_INFINITY), 0.0f, 0);
			}

			bool Unoccluded(const Scene* pScene) const;
		};

		class Light
		{
		protected:
			uint mSampleCount;

		public:
			Light(uint sampCount)
				: mSampleCount(sampCount)
			{
			}

			virtual ~Light() {}
			virtual Color Illuminate(const Vector3& pos,
				const Sample& lightSample,
				Vector3* pDir,
				VisibilityTester* pVisTest,
				float* pPdf,
				float* pCosAtLight = nullptr,
				float* pEmitPdfW = nullptr) const = 0;
			virtual Color Sample(const Sample& lightSample0,
				const Sample& lightSample1,
				Ray* pRay,
				Vector3* pNormal,
				float* pPdf,
				float* pDirectPdf = nullptr) const = 0;
			virtual Color Emit(const Vector3& dir,
				const Vector3& normal = Vector3::ZERO,
				float* pPdf = nullptr,
				float* pDirectPdf = nullptr) const = 0;
			virtual float Pdf(const Vector3& pos, const Vector3& dir) const = 0;
			virtual bool IsEnvironmentLight() const { return false; }
			virtual bool IsDelta() const = 0;
			virtual bool IsFinite() const = 0;
			float GetSampleCount() const { return mSampleCount; }
		};
	}
}