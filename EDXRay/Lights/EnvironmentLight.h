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
		class EnvironmentLight : public Light
		{
		private:
			RefPtr<Texture2D<Color>>			mpMap;
			RefPtr<Sampling::Distribution2D>	mpDistribution;
			Array2f								mLuminance;
			float								mScale;
			float								mRotation;
			bool								mIsTexture;

		public:
			EnvironmentLight(const Color& intens,
				const uint sampCount = 1)
				: Light(sampCount)
			{
				mIsTexture = false;
				mScale = 1.0f;
				mpMap = new ConstantTexture2D<Color>(intens);
			}

			EnvironmentLight(const char* path,
				const float scale = 1.0f,
				const float rotate = 0.0f,
				const uint sampCount = 1)
				: Light(sampCount)
			{
				mIsTexture = true;
				mScale = scale;
				mRotation = Math::ToRadians(rotate);
				mpMap = new ImageTexture<Color, Color>(path, 1.0f);

				CalcLuminanceDistribution();
			}

			EnvironmentLight(const Color& turbidity,
				const Color& groundAlbedo,
				const float sunElevation,
				const float rotate = 0.0f,
				const int resX = 1200,
				const int resY = 600,
				const uint sampCount = 1)
				: Light(sampCount)
			{
				mIsTexture = true;
				mScale = 1.0f;

				float sunElevationRad = Math::ToRadians(sunElevation);
				mRotation = Math::ToRadians(rotate);

				static const int NUM_CHANNELS = 3;
				ArHosekSkyModelState* skyModelState[NUM_CHANNELS];
				for (auto i = 0; i < NUM_CHANNELS; i++)
					skyModelState[i] = arhosek_rgb_skymodelstate_alloc_init(turbidity[i], groundAlbedo[i], sunElevationRad);

				const float sunZenith = float(Math::EDX_PI_2) - sunElevationRad;
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
								skyRadiance[Vector2i(x, y)][i] = 3000.0f;
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
				if (mIsTexture)
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
				float s = Math::SphericalPhi(negDir);
				s += mRotation;
				if (s > float(Math::EDX_TWO_PI))
					s -= float(Math::EDX_TWO_PI);
				s *= float(Math::EDX_INV_2PI);
				s = 1.0f - s;
				float t = Math::SphericalTheta(negDir) * float(Math::EDX_INV_PI);

				Vector2 diff[2] = { Vector2::ZERO, Vector2::ZERO };
				return mpMap->Sample(Vector2(s, t), diff, TextureFilter::TriLinear) * mScale;
			}

			float Pdf(const Vector3& pos, const Vector3& dir) const
			{
				if (mIsTexture)
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

			bool IsEnvironmentLight() const
			{
				return true;
			}
			bool IsDelta() const
			{
				return false;
			}
			Texture2D<Color>* GetTexture() const
			{
				return mpMap.Ptr();
			}
			bool IsTexture() const
			{
				return mIsTexture;
			}
			float GetRotation() const
			{
				return mRotation;
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
						u += mRotation * float(Math::EDX_INV_2PI);
						if (u >= 1.0f)
							u -= 1.0f;
						Vector2 diff[2] = { Vector2::ZERO, Vector2::ZERO };
						mLuminance[Vector2i(x, y)] = mpMap->Sample(Vector2(u, v), diff, TextureFilter::Linear).Luminance() * sinTheta;
					}
				}

				mpDistribution = new Sampling::Distribution2D(mLuminance.Data(), width, height);
			}
		};

	}
}