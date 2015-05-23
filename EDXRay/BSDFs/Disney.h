#pragma once

#include "../Core/BSDF.h"

namespace EDX
{
	namespace RayTracer
	{
		class Disney : public BSDF
		{
		private:
			float mRoughness;
			float mSpecular;
			float mMetallic;
			float mSpecularTint;
			float mSheen;
			float mSheenTint;
			float mSubsurface;

		public:
			Disney(const Color& reflectance = Color::WHITE,
				float roughness = 0.1f,
				float specular = 0.5f,
				float matellic = 0.0f,
				float specTint = 0.0f,
				float sheen = 0.0f,
				float sheenTint = 0.5f,
				float subsurface = 0.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE | BSDF_GLOSSY), BSDFType::Disney, reflectance)
				, mRoughness(roughness)
				, mSpecular(specular)
				, mMetallic(matellic)
				, mSpecularTint(specTint)
				, mSheen(sheen)
				, mSheenTint(sheenTint)
				, mSubsurface(subsurface)
			{
			}
			Disney(const RefPtr<Texture2D<Color>>& pTex,
				const bool isTextured,
				float roughness = 0.1f,
				float specular = 0.5f,
				float matellic = 0.0f,
				float specTint = 0.0f,
				float sheen = 0.0f,
				float sheenTint = 0.5f,
				float subsurface = 0.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE | BSDF_GLOSSY), BSDFType::Disney, pTex, isTextured)
				, mRoughness(roughness)
				, mSpecular(specular)
				, mMetallic(matellic)
				, mSpecularTint(specTint)
				, mSheen(sheen)
				, mSheenTint(sheenTint)
				, mSubsurface(subsurface)
			{
			}
			Disney(const char* pFile,
				float roughness = 0.1f,
				float specular = 0.5f,
				float matellic = 0.0f,
				float specTint = 0.0f,
				float sheen = 0.0f,
				float sheenTint = 0.5f,
				float subsurface = 0.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE | BSDF_GLOSSY), BSDFType::Disney, pFile)
				, mRoughness(roughness)
				, mSpecular(specular)
				, mMetallic(matellic)
				, mSpecularTint(specTint)
				, mSheen(sheen)
				, mSheenTint(sheenTint)
				, mSubsurface(subsurface)
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
			float Pdf(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				if (!MatchesTypes(types))
					return 0.0f;

				Vector3 wh = Math::Normalize(wo + wi);
				if (wh == Vector3::ZERO)
					return 0.0f;

				float microfacetPdf = GGX_Pdf(wh, mRoughness * mRoughness);
				float pdf = microfacetPdf;
				float dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh));
				pdf *= dwh_dwi;
				pdf += BSDFCoordinate::AbsCosTheta(wi) * float(Math::EDX_INV_PI);

				return pdf;
			}

			Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const
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

				Color albedo = GetColor(diffGeom);
				Color specAlbedo = Math::Lerp(Math::Lerp(albedo, Color::WHITE, 1.0f - mSpecularTint), albedo, mMetallic);
				Color sheenAlbedo = Math::Lerp(Color::WHITE, albedo, mSheenTint);

				Vector3 wh = Math::Normalize(wo + wi);
				float ODotH = Math::Dot(wo, wh);
				float IDotH = Math::Dot(wi, wh);
				float OneMinusODotH = 1.0f - ODotH;
				specAlbedo = Math::Lerp(specAlbedo, Color::WHITE, OneMinusODotH * OneMinusODotH * OneMinusODotH);

				// Sheen term
				float F = Fresnel_Schlick(ODotH, 0.0f);
				Color sheenTerm = F * mSheen * sheenAlbedo;

				return (1.0f - mMetallic) * (albedo * Math::Lerp(DiffuseTerm(wo, wi, IDotH), SubsurfaceTerm(wo, wi, IDotH), mSubsurface) + sheenTerm) + specAlbedo * SpecularTerm(wo, wi, wh, ODotH, nullptr);
			}

			float Eval(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				return 0.0f;
			}

			float DiffuseTerm(const Vector3& wo, const Vector3& wi, const float IDotH) const
			{
				if (mMetallic == 1.0f)
					return 0.0f;

				float oneMinusCosL = 1.0f - BSDFCoordinate::AbsCosTheta(wi);
				float oneMinusCosLSqr = oneMinusCosL * oneMinusCosL;
				float oneMinusCosV = 1.0f - BSDFCoordinate::AbsCosTheta(wo);
				float oneMinusCosVSqr = oneMinusCosV * oneMinusCosV;
				float F_D90 = 0.5f + 2.0f * IDotH * IDotH * mRoughness;

				return float(Math::EDX_INV_PI) * (1.0f + (F_D90 - 1.0f) * oneMinusCosLSqr * oneMinusCosLSqr * oneMinusCosL) *
					(1.0f + (F_D90 - 1.0f) * oneMinusCosVSqr * oneMinusCosVSqr * oneMinusCosV);
			}

			float SpecularTerm(const Vector3& wo,
				const Vector3& wi,
				const Vector3& wh,
				const float ODotH,
				const float* pFresnel = nullptr) const
			{
				if (BSDFCoordinate::CosTheta(wo) * BSDFCoordinate::CosTheta(wi) <= 0.0f)
					return 0.0f;

				float D = GGX_D(wh, mRoughness * mRoughness);
				if (D == 0.0f)
					return 0.0f;

				float normalRef = Math::Lerp(0.0f, 0.08f, mSpecular);
				float F = pFresnel ? *pFresnel : Fresnel_Schlick(ODotH, normalRef);

				float roughForG = (0.5f + 0.5f * mRoughness);
				float G = GGX_G(wo, wi, wh, roughForG * roughForG);

				return F * D * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
			}

			float SubsurfaceTerm(const Vector3& wo, const Vector3& wi, const float IDotH) const
			{
				if (mSubsurface == 0.0f)
					return 0.0f;

				float oneMinusCosL = 1.0f - BSDFCoordinate::AbsCosTheta(wi);
				float oneMinusCosLSqr = oneMinusCosL * oneMinusCosL;
				float oneMinusCosV = 1.0f - BSDFCoordinate::AbsCosTheta(wo);
				float oneMinusCosVSqr = oneMinusCosV * oneMinusCosV;
				float F_ss90 = IDotH * IDotH * mRoughness;

				float S = (1.0f + (F_ss90 - 1.0f) * oneMinusCosLSqr * oneMinusCosLSqr * oneMinusCosL) *
					(1.0f + (F_ss90 - 1.0f) * oneMinusCosVSqr * oneMinusCosVSqr * oneMinusCosV);

				return float(Math::EDX_INV_PI) * 1.25f * (S * (1.0f / (BSDFCoordinate::AbsCosTheta(wo) + BSDFCoordinate::AbsCosTheta(wi)) - 0.5f) + 0.5f);
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

		public:
			int GetParameterCount() const
			{
				return BSDF::GetParameterCount() + 7;
			}

			string GetParameterName(const int idx) const
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
					ret.Value = this->mRoughness;
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

				return ret;
			}

			void SetParameter(const string& name, const Parameter& param)
			{
				BSDF::SetParameter(name, param);

				if (name == "Roughness")
					this->mRoughness = param.Value;
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

				return;
			}
		};
	}
}