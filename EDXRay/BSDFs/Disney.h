#pragma once

#include "../Core/BSDF.h"

namespace EDX
{
	namespace RayTracer
	{
		class Disney : public BSDF
		{
		private:
			UniquePtr<Texture2D<float>> mRoughness;
			float mSpecular;
			float mMetallic;
			float mSpecularTint;
			float mSheen;
			float mSheenTint;
			float mSubsurface;
			float mClearCoat;
			float mClearCoatGloss;

		public:
			Disney(const Color& reflectance = Color::WHITE,
				float roughness = 0.1f,
				float specular = 0.5f,
				float matellic = 0.0f,
				float specTint = 0.0f,
				float sheen = 0.0f,
				float sheenTint = 0.5f,
				float subsurface = 0.0f,
				float clearCoat = 0.0f,
				float clearCoatGloss = 0.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE | BSDF_GLOSSY), BSDFType::Disney, reflectance)
				, mRoughness(new ConstantTexture2D<float>(roughness))
				, mSpecular(specular)
				, mMetallic(matellic)
				, mSpecularTint(specTint)
				, mSheen(sheen)
				, mSheenTint(sheenTint)
				, mSubsurface(subsurface)
				, mClearCoat(clearCoat)
				, mClearCoatGloss(clearCoatGloss)
			{
			}

			Disney(UniquePtr<Texture2D<Color>> pTex,
				UniquePtr<Texture2D<Color>> pNormal,
				float roughness = 0.1f,
				float specular = 0.5f,
				float matellic = 0.0f,
				float specTint = 0.0f,
				float sheen = 0.0f,
				float sheenTint = 0.5f,
				float subsurface = 0.0f,
				float clearCoat = 0.0f,
				float clearCoatGloss = 0.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE | BSDF_GLOSSY), BSDFType::Disney, Move(pTex), Move(pNormal))
				, mRoughness(new ConstantTexture2D<float>(roughness))
				, mSpecular(specular)
				, mMetallic(matellic)
				, mSpecularTint(specTint)
				, mSheen(sheen)
				, mSheenTint(sheenTint)
				, mSubsurface(subsurface)
				, mClearCoat(clearCoat)
				, mClearCoatGloss(clearCoatGloss)
			{
			}
			Disney(const char* pFile,
				float roughness = 0.1f,
				float specular = 0.5f,
				float matellic = 0.0f,
				float specTint = 0.0f,
				float sheen = 0.0f,
				float sheenTint = 0.5f,
				float subsurface = 0.0f,
				float clearCoat = 0.0f,
				float clearCoatGloss = 0.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE | BSDF_GLOSSY), BSDFType::Disney, pFile)
				, mRoughness(new ConstantTexture2D<float>(roughness))
				, mSpecular(specular)
				, mMetallic(matellic)
				, mSpecularTint(specTint)
				, mSheen(sheen)
				, mSheenTint(sheenTint)
				, mSubsurface(subsurface)
				, mClearCoat(clearCoat)
				, mClearCoatGloss(clearCoatGloss)
			{
			}

			Color SampleScattered(const Vector3& _wo,
				const Sample& sample,
				const DifferentialGeom& diffGeom,
				Vector3* pvIn,
				float* pPdf,
				ScatterType types = BSDF_ALL,
				ScatterType* pSampledTypes = nullptr) const;

		private:
			float PdfInner(const Vector3& wo, const Vector3& wi, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const
			{
				Vector3 wh = Math::Normalize(wo + wi);
				if (wh == Vector3::ZERO)
					return 0.0f;

				float roughness = GetValue(mRoughness.Get(), diffGeom, TextureFilter::Linear);
				roughness = Math::Clamp(roughness, 0.02f, 1.0f);

				float microfacetPdf = GGX_Pdf_VisibleNormal(wo, wh, roughness * roughness);
				float pdf = 0.0f;
				float dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh));
				float specPdf = microfacetPdf * dwh_dwi;

				pdf += specPdf;
				pdf += BSDFCoordinate::AbsCosTheta(wi) * float(Math::EDX_INV_PI);

				if (mClearCoat > 0.0f)
				{
					Color albedo = GetValue(mpTexture.Get(), diffGeom);
					float coatWeight = mClearCoat / (mClearCoat + albedo.Luminance());
					float FresnelCoat = Fresnel_Schlick_Coat(BSDFCoordinate::AbsCosTheta(wo));
					float probCoat = (FresnelCoat * coatWeight) /
						(FresnelCoat * coatWeight +
						(1 - FresnelCoat) * (1 - coatWeight));
					float coatRough = Math::Lerp(0.005f, 0.10f, mClearCoatGloss);
					float coatHalfPdf = GGX_Pdf_VisibleNormal(wo, wh, coatRough);
					float coatPdf = coatHalfPdf * dwh_dwi;

					pdf *= 1.0f - probCoat;
					pdf += coatPdf * probCoat;
				}

				return pdf;
			}

			Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const override
			{
				if (Math::Dot(vOut, diffGeom.mGeomNormal) * Math::Dot(vIn, diffGeom.mGeomNormal) > 0.0f)
					types = ScatterType(types & ~BSDF_TRANSMISSION);
				else
					types = ScatterType(types & ~BSDF_REFLECTION);

				if (!MatchesTypes(types))
				{
					return Color::BLACK;
				}

				Vector3 wo = diffGeom.WorldToLocal(vOut);
				Vector3 wi = diffGeom.WorldToLocal(vIn);
				
				return EvalTransformed(wo, wi, diffGeom, types);
			}

			Color EvalTransformed(const Vector3& wo, const Vector3& wi, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const
			{
				Color albedo = GetValue(mpTexture.Get(), diffGeom);
				float roughness = GetValue(mRoughness.Get(), diffGeom, TextureFilter::Linear);
				roughness = Math::Clamp(roughness, 0.02f, 1.0f);
				Color UntintedSpecAlbedo = albedo.Luminance();
				Color specAlbedo = Math::Lerp(Math::Lerp(albedo, UntintedSpecAlbedo, 1.0f - mSpecularTint), albedo, mMetallic);
				Color sheenAlbedo = Math::Lerp(UntintedSpecAlbedo, albedo, mSheenTint);

				Vector3 wh = Math::Normalize(wo + wi);
				float ODotH = Math::Dot(wo, wh);
				float IDotH = Math::Dot(wi, wh);
				float OneMinusODotH = 1.0f - ODotH;
				Color grazingSpecAlbedo = Math::Lerp(specAlbedo, UntintedSpecAlbedo, mMetallic);
				specAlbedo = Math::Lerp(specAlbedo, UntintedSpecAlbedo, OneMinusODotH * OneMinusODotH * OneMinusODotH);

				// Sheen term
				float F = Fresnel_Schlick(ODotH, 0.0f);
				Color sheenTerm = F * mSheen * sheenAlbedo;

				return (1.0f - mMetallic)
					* (albedo * Math::Lerp(DiffuseTerm(wo, wi, IDotH, roughness), SubsurfaceTerm(wo, wi, IDotH, roughness), mSubsurface)
					+ sheenTerm)
					+ specAlbedo * SpecularTerm(wo, wi, wh, ODotH, roughness, nullptr)
					+ Color(ClearCoatTerm(wo, wi, wh, IDotH, mClearCoatGloss));
			}

			float EvalInner(const Vector3& wo, const Vector3& wi, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const override
			{
				return 0.0f;
			}

			float DiffuseTerm(const Vector3& wo, const Vector3& wi, const float IDotH, const float roughness) const
			{
				if (mMetallic == 1.0f)
					return 0.0f;

				float oneMinusCosL = 1.0f - BSDFCoordinate::AbsCosTheta(wi);
				float oneMinusCosLSqr = oneMinusCosL * oneMinusCosL;
				float oneMinusCosV = 1.0f - BSDFCoordinate::AbsCosTheta(wo);
				float oneMinusCosVSqr = oneMinusCosV * oneMinusCosV;
				float F_D90 = 0.5f + 2.0f * IDotH * IDotH * roughness;

				return float(Math::EDX_INV_PI) * (1.0f + (F_D90 - 1.0f) * oneMinusCosLSqr * oneMinusCosLSqr * oneMinusCosL) *
					(1.0f + (F_D90 - 1.0f) * oneMinusCosVSqr * oneMinusCosVSqr * oneMinusCosV);
			}

			float SpecularTerm(const Vector3& wo,
				const Vector3& wi,
				const Vector3& wh,
				const float ODotH,
				const float roughness,
				const float* pFresnel = nullptr) const
			{
				if (BSDFCoordinate::CosTheta(wo) * BSDFCoordinate::CosTheta(wi) <= 0.0f)
					return 0.0f;

				float D = GGX_D(wh, roughness * roughness);
				if (D == 0.0f)
					return 0.0f;

				float normalRef = Math::Lerp(0.0f, 0.08f, mSpecular);
				float F = pFresnel ? *pFresnel : Fresnel_Schlick(ODotH, normalRef);

				float roughForG = (0.5f + 0.5f * roughness);
				float G = GGX_G(wo, wi, wh, roughForG * roughForG);

				return F * D * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
			}

			float SubsurfaceTerm(const Vector3& wo, const Vector3& wi, const float IDotH, const float roughness) const
			{
				if (mSubsurface == 0.0f)
					return 0.0f;

				float oneMinusCosL = 1.0f - BSDFCoordinate::AbsCosTheta(wi);
				float oneMinusCosLSqr = oneMinusCosL * oneMinusCosL;
				float oneMinusCosV = 1.0f - BSDFCoordinate::AbsCosTheta(wo);
				float oneMinusCosVSqr = oneMinusCosV * oneMinusCosV;
				float F_ss90 = IDotH * IDotH * roughness;

				float S = (1.0f + (F_ss90 - 1.0f) * oneMinusCosLSqr * oneMinusCosLSqr * oneMinusCosL) *
					(1.0f + (F_ss90 - 1.0f) * oneMinusCosVSqr * oneMinusCosVSqr * oneMinusCosV);

				return float(Math::EDX_INV_PI) * 1.25f * (S * (1.0f / (BSDFCoordinate::AbsCosTheta(wo) + BSDFCoordinate::AbsCosTheta(wi)) - 0.5f) + 0.5f);
			}

			float ClearCoatTerm(const Vector3& wo, const Vector3& wi, const Vector3& wh, const float IDotH, const float roughness) const
			{
				if (mClearCoat == 0.0f)
					return 0.0f;

				float rough = Math::Lerp(0.005f, 0.1f, roughness);

				float D = GGX_D(wh, rough);
				if (D == 0.0f)
					return 0.0f;

				float oneMinusCosD = 1.0f - IDotH;
				float oneMinusCosDSqr = oneMinusCosD * oneMinusCosD;
				float F = Fresnel_Schlick_Coat(IDotH);
				float G = GGX_G(wo, wi, wh, 0.25f);

				return mClearCoat * D * F * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
			}

			float Fresnel_Schlick(const float cosD, const float normalReflectance) const
			{
				float oneMinusCosD = 1.0f - cosD;
				float oneMinusCosDSqr = oneMinusCosD * oneMinusCosD;
				float fresnel = normalReflectance +
					(1.0f - normalReflectance) * oneMinusCosDSqr * oneMinusCosDSqr * oneMinusCosD;
				float fresnelConductor = BSDF::FresnelConductor(cosD, 0.4f, 1.6f);

				return Math::Lerp(fresnel, fresnelConductor, mMetallic);
			}

			float Fresnel_Schlick_Coat(const float cosD) const
			{
				float oneMinusCosD = 1.0f - cosD;
				float oneMinusCosDSqr = oneMinusCosD * oneMinusCosD;
				float fresnel = 0.04f +
					(1.0f - 0.04f) * oneMinusCosDSqr * oneMinusCosDSqr * oneMinusCosD;

				return fresnel;
			}

		public:
			int GetParameterCount() const
			{
				return BSDF::GetParameterCount() + 9;
			}

			String GetParameterName(const int idx) const
			{
				if (idx < BSDF::GetParameterCount())
					return BSDF::GetParameterName(idx);

				int baseParamCount = BSDF::GetParameterCount();
				if (idx == baseParamCount + 0)
					return "Roughness";
				else if (idx == baseParamCount + 1)
					return "Specular";
				else if (idx == baseParamCount + 2)
					return "Metallic";
				else if (idx == baseParamCount + 3)
					return "SpecularTint";
				else if (idx == baseParamCount + 4)
					return "Sheen";
				else if (idx == baseParamCount + 5)
					return "SheenTint";
				else if (idx == baseParamCount + 6)
					return "Subsurface";
				else if (idx == baseParamCount + 7)
					return "ClearCoat";
				else if (idx == baseParamCount + 8)
					return "ClearCoatGloss";

				return "";
			}

			Parameter GetParameter(const String& name) const
			{
				Parameter ret = BSDF::GetParameter(name);
				if (ret.Type != Parameter::None)
					return ret;

				if (name == "Roughness")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mRoughness->GetValue();
					ret.Min = 0.02f;
					ret.Max = 1.0f;

					return ret;
				}
				else if (name == "Specular")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mSpecular;
					ret.Min = 0.0f;
					ret.Max = 1.0f;

					return ret;
				}
				else if (name == "Metallic")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mMetallic;
					ret.Min = 0.0f;
					ret.Max = 1.0f;

					return ret;
				}
				else if (name == "SpecularTint")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mSpecularTint;
					ret.Min = 0.0f;
					ret.Max = 1.0f;

					return ret;
				}
				else if (name == "Sheen")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mSheen;
					ret.Min = 0.0f;
					ret.Max = 1.0f;

					return ret;
				}
				else if (name == "SheenTint")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mSheenTint;
					ret.Min = 0.0f;
					ret.Max = 1.0f;

					return ret;
				}
				else if (name == "Subsurface")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mSubsurface;
					ret.Min = 0.0f;
					ret.Max = 1.0f;

					return ret;
				}
				else if (name == "ClearCoat")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mClearCoat;
					ret.Min = 0.0f;
					ret.Max = 1.0f;

					return ret;
				}
				else if (name == "ClearCoatGloss")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mClearCoatGloss;
					ret.Min = 0.0f;
					ret.Max = 1.0f;

					return ret;
				}

				return ret;
			}

			void SetParameter(const String& name, const Parameter& param)
			{
				BSDF::SetParameter(name, param);

				if (name == "Roughness")
				{
					if (param.Type == Parameter::Float)
					{
						if (this->mRoughness->IsConstant())
							this->mRoughness->SetValue(param.Value);
						else
							this->mRoughness.Reset(new ConstantTexture2D<float>(param.Value));
					}
					else if (param.Type == Parameter::TextureMap)
						this->mRoughness.Reset(new ImageTexture<float, float>(param.TexPath, 1.0f));
				}
				else if (name == "Specular")
					this->mSpecular = param.Value;
				else if (name == "Metallic")
					this->mMetallic = param.Value;
				else if (name == "SpecularTint")
					this->mSpecularTint = param.Value;
				else if (name == "Sheen")
					this->mSheen = param.Value;
				else if (name == "SheenTint")
					this->mSheenTint = param.Value;
				else if (name == "Subsurface")
					this->mSubsurface = param.Value;
				else if (name == "ClearCoat")
					this->mClearCoat = param.Value;
				else if (name == "ClearCoatGloss")
					this->mClearCoatGloss = param.Value;

				return;
			}
		};
	}
}