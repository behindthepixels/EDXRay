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
		class EnvironmentalLight : public Light
		{
		private:
			Color	mIntensity;

		public:
			EnvironmentalLight(const Color& intens,
				const uint sampCount = 1)
				: Light(sampCount)
				, mIntensity(intens)
			{
			}

			Color Illuminate(const Vector3& pos, const Sample& lightSample, Vector3* pDir, VisibilityTester* pVisTest, float* pPdf) const
			{
				Vector3 vDir = Sampling::UniformSampleSphere(lightSample.u, lightSample.v);
				*pDir = Vector3(vDir.x, vDir.z, -vDir.y);
				pVisTest->SetRay(pos, *pDir);
				*pPdf = Sampling::UniformSpherePDF();

				//if (pfCosAtLight)
				//{
				//	*pfCosAtLight = 1.0f;
				//}
				//if (pfEmitPDFW)
				//{
				//	Vector3 vSphCenter;
				//	*pfEmitPDFW = Sampling::UniformSpherePDF() * mfInvSceneBoundArea;
				//}

				return mIntensity;
			}

			Color Emit(const Vector3& dir) const
			{
				return mIntensity;
			}

			float Pdf(const Vector3& pos, const Vector3& dir) const
			{
				return Sampling::UniformSpherePDF();
			}

			bool IsDelta() const
			{
				return false;
			}
		};

	}
}