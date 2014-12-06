#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Light.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "Graphics/Color.h"
#include "Graphics/Texture.h"

#include "SkyLight/ArHosekSkyModel.h"

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

				CalcLuminanceDistribution();
			}

			EnvironmentalLight(const Color& turbidity,
				const Color& groundAlbedo,
				const float sunElevation,
				const int resX = 1200,
				const int resY = 600,
				const uint sampCount = 1)
				: Light(sampCount)
			{
				mIsEnvMap = true;

				static const int NUM_CHANNELS = 3;
				ArHosekSkyModelState* skyModelState[NUM_CHANNELS];
				for (auto i = 0; i < NUM_CHANNELS; i++)
					skyModelState[i] = arhosek_rgb_skymodelstate_alloc_init(turbidity[i], groundAlbedo[i], sunElevation);

				const float sunZenith = float(Math::EDX_PI_2) - sunElevation;
				Array<2, Color> skyRadiance;
				skyRadiance.Init(Vector2i(resX, resY));
				for (auto y = 0; y < resY * 0.5f; y++)
				{
					float v = (y + 0.5f) / float(resY);
					for (auto x = 0; x < resX; x++)
					{
						float u = (x + 0.5f) / float(resX);
						float phi = u * float(Math::EDX_TWO_PI);
						float theta = v * float(Math::EDX_PI);

						float cosGamma = Math::Cos(theta) * Math::Cos(sunZenith)
							+ Math::Sin(theta) * Math::Sin(sunZenith)
							* Math::Cos(phi - float(Math::EDX_PI));
						float gamma = acosf(cosGamma);

						for (auto i = 0; i < NUM_CHANNELS; i++)
						{
							float r = arhosek_tristim_skymodel_radiance(skyModelState[i], theta, gamma, i);
							assert(Math::NumericValid(r));
							skyRadiance[Vector2i(x, y)][i] = r * 0.011f;
							if (gamma < 0.02)
								skyRadiance[Vector2i(x, y)][i] = 1900.0f;
						}
					}
				}

				for (auto i = 0; i < NUM_CHANNELS; i++)
					arhosekskymodelstate_free(skyModelState[i]);

				mpMap = new ImageTexture<Color, Color>(skyRadiance.Data(), resX, resY);

				CalcLuminanceDistribution();
			}

			Color Illuminate(const Vector3& pos, const Sample& lightSample, Vector3* pDir, VisibilityTester* pVisTest, float* pPdf) const
			{
				if (mIsEnvMap)
				{
					float u, v;
					mpDistribution->SampleContinuous(lightSample.u, lightSample.v, &u, &v, pPdf);
					if (*pPdf == 0.0f)
						return Color::BLACK;

					float phi = u * float(Math::EDX_TWO_PI);
					float theta = v * float(Math::EDX_PI);
					float sinTheta = Math::Sin(theta);
					*pPdf = sinTheta != 0.0f ? *pPdf / (2.0f * float(Math::EDX_PI) * float(Math::EDX_PI) * sinTheta) : 0.0f;

					*pDir = Math::SphericalDirection(sinTheta,
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
				Vector3 negDir = -dir;
				float s = Math::SphericalPhi(negDir) * float(Math::EDX_INV_2PI);
				float t = Math::SphericalTheta(negDir) * float(Math::EDX_INV_PI);

				Vector2 diff[2] = { Vector2::ZERO, Vector2::ZERO };
				return mpMap->Sample(Vector2(s, t), diff, TextureFilter::Linear);
			}

			float Pdf(const Vector3& pos, const Vector3& dir) const
			{
				if (mIsEnvMap)
				{
					float theta = Math::SphericalTheta(dir);
					float phi = Math::SphericalPhi(dir);
					float sinTheta = Math::Sin(theta);
					return mpDistribution->Pdf(phi * float(Math::EDX_INV_2PI), theta * float(Math::EDX_INV_PI)) /
						(2.0f * float(Math::EDX_PI) * float(Math::EDX_PI) * sinTheta);
				}
				else
					return Sampling::UniformSpherePDF();
			}

			bool IsDelta() const
			{
				return false;
			}

		private:
			void CalcLuminanceDistribution()
			{
				auto width = mpMap->Width();
				auto height = mpMap->Height();
				mLuminance.Init(Vector2i(width, height));
				for (auto y = 0; y < height; y++)
				{
					float v = (y + 0.5f) / float(height);
					float sinTheta = Math::Sin(float(Math::EDX_PI) * (y + 0.5f) / float(height));
					for (auto x = 0; x < width; x++)
					{
						float u = (x + 0.5f) / float(width);
						Vector2 diff[2] = { Vector2::ZERO, Vector2::ZERO };
						mLuminance[Vector2i(x, y)] = mpMap->Sample(Vector2(u, v), diff, TextureFilter::Linear).Luminance() * sinTheta;
					}
				}

				mpDistribution = new Sampling::Distribution2D(mLuminance.Data(), width, height);
			}
		};

	}
}