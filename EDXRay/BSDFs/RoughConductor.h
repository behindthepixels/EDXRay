#pragma once

#include "../Core/BSDF.h"

namespace EDX
{
	namespace RayTracer
	{
		class RoughConductor : public BSDF
		{
		private:
			RefPtr<Texture2D<float>> mRoughness;

		public:
			RoughConductor(const Color& reflectance = Color::WHITE, float roughness = 0.3f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_GLOSSY), BSDFType::RoughConductor, reflectance)
			{
				mRoughness = new ConstantTexture2D<float>(roughness);
			}
			RoughConductor(const RefPtr<Texture2D<Color>>& pTex, const RefPtr<Texture2D<Color>>& pNormal, float roughness = 0.3f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_GLOSSY), BSDFType::RoughConductor, pTex, pNormal)
			{
				mRoughness = new ConstantTexture2D<float>(roughness);
			}
			RoughConductor(const char* pFile, float roughness = 1.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_GLOSSY), BSDFType::RoughConductor, pFile)
			{
				mRoughness = new ConstantTexture2D<float>(roughness);
			}

			Color SampleScattered(const Vector3& _wo,
				const Sample& sample,
				const DifferentialGeom& diffGeom,
				Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL,
				ScatterType* pSampledTypes = NULL) const;

		private:
			float PdfInner(const Vector3& wo, const Vector3& wi, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const
			{
				if (BSDFCoordinate::CosTheta(wo) < 0.0f || !BSDFCoordinate::SameHemisphere(wo, wi))
					return 0.0f;

				Vector3 wh = Math::Normalize(wo + wi);

				if (BSDFCoordinate::CosTheta(wh) < 0.0f)
					wh *= -1.0f;

				float roughness = GetValue(mRoughness.Ptr(), diffGeom, TextureFilter::Linear);

				float dwh_dwi = 1.0f / (4.0f * Math::Dot(wi, wh));
				float whProb = GGX_Pdf(wh, roughness * roughness);

				return Math::Abs(whProb * dwh_dwi);
			}

			float EvalInner(const Vector3& wo, const Vector3& wi, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const
			{
				if (BSDFCoordinate::CosTheta(wo) < 0.0f || !BSDFCoordinate::SameHemisphere(wo, wi))
					return 0.0f;

				Vector3 wh = Math::Normalize(wo + wi);

				float roughness = GetValue(mRoughness.Ptr(), diffGeom, TextureFilter::Linear);
				float sampleRough = roughness * roughness;
				float D = GGX_D(wh, sampleRough);
				if (D == 0.0f)
					return 0.0f;

				float F = BSDF::FresnelConductor(Math::Dot(wo, wh), 0.4f, 1.6f);
				float G = GGX_G(wo, wi, wh, sampleRough);

				return F * D * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
			}

		public:
			int GetParameterCount() const
			{
				return BSDF::GetParameterCount() + 1;
			}

			string GetParameterName(const int idx) const
			{
				if (idx < BSDF::GetParameterCount())
					return BSDF::GetParameterName(idx);

				if (idx == GetParameterCount() - 1)
					return "Roughness";

				return "";
			}

			Parameter GetParameter(const string& name) const
			{
				Parameter ret = BSDF::GetParameter(name);
				if (ret.Type != Parameter::None)
					return ret;

				if (name == "Roughness")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mRoughness->GetValue();
					ret.Min = 0.01f;
					ret.Max = 1.0f;

					return ret;
				}

				return ret;
			}

			void SetParameter(const string& name, const Parameter& param)
			{
				BSDF::SetParameter(name, param);

				if (name == "Roughness")
				{
					if (param.Type == Parameter::Float)
					{
						if (this->mRoughness->IsConstant())
							this->mRoughness->SetValue(param.Value);
						else
							this->mRoughness = new ConstantTexture2D<float>(param.Value);
					}
					else if (param.Type == Parameter::TextureMap)
						this->mRoughness = new ImageTexture<float, float>(param.TexPath, 1.0f);
				}

				return;
			}
		};
	}
}