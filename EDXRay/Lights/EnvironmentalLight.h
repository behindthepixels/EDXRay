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
			RefPtr<Texture2D<Color>>			mpMap;
			RefPtr<Sampling::Distribution2D>	mpDistribution;
			Array2f								mLuminance;
			bool mIsEnvMap;

		public:
			EnvironmentalLight(const Color& intens,
				const uint sampCount = 1)
				: Light(sampCount)
			{
				mIsEnvMap = false;
				mpMap = new ConstantTexture2D<Color>(intens);
			}

			EnvironmentalLight(const char* path,
				const uint sampCount = 1)
				: Light(sampCount)
			{
				mIsEnvMap = true;
				mpMap = new ImageTexture<Color, Color>(path, 1.0f);

				auto width = mpMap->Width();
				auto height = mpMap->Height();
				mLuminance.Init(Vector2i(width, height));
				for (auto y = 0; y < height; y++)
				{
					float v = y / float(height);
					float sinTheta = Math::Sin(float(Math::EDX_PI) * (y + 0.5f) / float(height));
					for (auto x = 0; x < width; x++)
					{
						float u = x / float(width);
						Vector2 diff[2] = { Vector2::ZERO, Vector2::ZERO };
						mLuminance[Vector2i(x, y)] = mpMap->Sample(Vector2(u, v), diff, TextureFilter::Linear).Luminance() * sinTheta;
					}
				}

				mpDistribution = new Sampling::Distribution2D(mLuminance.Data(), width, height);
			}

			Color Illuminate(const Vector3& pos, const Sample& lightSample, Vector3* pDir, VisibilityTester* pVisTest, float* pPdf) const
			{
				if (mIsEnvMap)
				{
					float u, v;
					mpDistribution->SampleContinuous(lightSample.u, lightSample.v, &u, &v, pPdf);

					float phi = u * float(Math::EDX_TWO_PI);
					float theta = v * float(Math::EDX_PI);

					*pDir = Math::SphericalDirection(Math::Sin(theta),
						Math::Cos(theta),
						phi);
				}
				else
				{
					Vector3 dir = Sampling::UniformSampleSphere(lightSample.u, lightSample.v);
					*pDir = Vector3(dir.x, dir.z, -dir.y);
					*pPdf = Sampling::UniformSpherePDF();
				}

				pVisTest->SetRay(pos, *pDir);

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