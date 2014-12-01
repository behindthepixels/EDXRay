#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Light.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "Graphics/Color.h"
#include "Graphics/Texture.h"

namespace EDX
{
	namespace RayTracer
	{
		class EnvironmentalLight : public Light
		{
		private:
			RefPtr<Texture2D<Color>> mpMap;
			bool mIsEnvMap;

		public:
			EnvironmentalLight(const Color& intens,
				const uint sampCount = 1)
				: Light(sampCount)
			{
				mpMap = new ConstantTexture2D<Color>(intens);
				mIsEnvMap = false;
			}

			EnvironmentalLight(const char* path,
				const uint sampCount = 1)
				: Light(sampCount)
			{
				mpMap = new ImageTexture<Color, Color>(path, 1.0f);
				mIsEnvMap = true;
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

				return Emit(-*pDir);
			}

			Color Emit(const Vector3& dir) const
			{
				float s = Math::SphericalPhi(dir) * float(Math::EDX_INV_2PI);
				float t = Math::SphericalTheta(dir) * float(Math::EDX_INV_PI);

				Vector2 diff[2] = { Vector2::ZERO, Vector2::ZERO };
				return mpMap->Sample(Vector2(s, 1.0f - t), diff, TextureFilter::Linear);
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