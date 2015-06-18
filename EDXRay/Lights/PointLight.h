#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Light.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		class PointLight : public Light
		{
		private:
			Vector3	mPosition;
			Color	mIntensity;

		public:
			PointLight(const Vector3& pos,
				const Color& intens, 
				const uint sampCount = 1)
				: Light(sampCount)
				, mPosition(pos)
				, mIntensity(intens)
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
				*pDir = Math::Normalize(mPosition - pos);
				*pPdf = 1.0f;
				pVisTest->SetSegment(pos, mPosition);

				if (pCosAtLight != NULL)
				{
					*pCosAtLight = 1.0f;
				}
				if (pEmitPdfW != NULL)
				{
					*pEmitPdfW = Sampling::UniformSpherePDF();
				}

				return mIntensity / Math::DistanceSquared(mPosition, pos);
			}

			Color Sample(const RayTracer::Sample& lightSample1,
				const RayTracer::Sample& lightSample2,
				Ray* pRay,
				Vector3* pNormal,
				float* pPdf,
				float* pDirectPdf = nullptr) const override
			{
				Vector3 dir = Sampling::UniformSampleSphere(lightSample1.u, lightSample1.v);
				*pRay = Ray(mPosition, dir);
				*pNormal = Vector3(dir);
				*pPdf = Sampling::UniformHemispherePDF();
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
				return true;
			}
		};

	}
}