#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Light.h"
#include "../Core/Scene.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		class DirectionalLight : public Light
		{
		private:
			Color	mIntensity;
			Vector3 mDirection;
			Frame	mDirFrame;
			const Scene* mpScene;

		public:
			DirectionalLight(const Vector3& dir,
				const Color& intens,
				const Scene* pScene,
				const uint sampCount = 1)
				: Light(sampCount)
				, mDirection(Math::Normalize(dir))
				, mDirFrame(-mDirection)
				, mIntensity(intens)
				, mpScene(pScene)
			{
			}

			Color Illuminate(const Vector3& pos,
				const RayTracer::Sample& lightSample,
				Vector3* pDir,
				VisibilityTester* pVisTest,
				float* pPdf,
				float* pCosAtLight = nullptr,
				float* pEmitPdfW = nullptr) const override
			{
				*pDir = mDirection;
				*pPdf = 1.0f;
				pVisTest->SetRay(pos, mDirection);

				if (pCosAtLight)
					*pCosAtLight = 1.f;

				if (pEmitPdfW)
				{
					Vector3 center;
					float radius;
					mpScene->WorldBounds().BoundingSphere(&center, &radius);
					*pEmitPdfW = Sampling::ConcentricDiscPdf() / (radius * radius);
				}

				return mIntensity;
			}

			Color Sample(const RayTracer::Sample& lightSample1,
				const RayTracer::Sample& lightSample2,
				Ray* pRay,
				Vector3* pNormal,
				float* pPdf,
				float* pDirectPdf = nullptr) const override
			{
				float f1, f2;
				Sampling::ConcentricSampleDisk(lightSample1.u, lightSample2.v, &f1, &f2);

				Vector3 center;
				float radius;
				mpScene->WorldBounds().BoundingSphere(&center, &radius);

				Vector3 origin = center + radius * (f1 * mDirFrame.mX + f2 * mDirFrame.mY);
				*pRay = Ray(origin + radius * mDirection, -mDirection);
				*pNormal = pRay->mDir;
				*pPdf = Sampling::ConcentricDiscPdf() / (radius * radius);
				if (pDirectPdf)
					*pDirectPdf = 1.0f;

				return mIntensity;
			}

			Color Emit(const Vector3& dir,
				const Vector3& normal = Vector3::ZERO,
				float* pPdf = nullptr,
				float* pDirectPdf = nullptr) const override
			{
				return Color::BLACK;
			}

			float Pdf(const Vector3& pos, const Vector3& dir) const override
			{
				return 0.0f;
			}

			bool IsDelta() const override
			{
				return true;
			}

			bool IsFinite() const override
			{
				return false;
			}
		};

	}
}