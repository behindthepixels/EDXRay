#pragma once

#include "../Core/BSDF.h"

namespace EDX
{
	namespace RayTracer
	{
		class Principled : public BSDF
		{
		private:
			float mRoughness;
			float mSpecular;
			float mMetallic;
			float mSpecularTint;

		public:
			Principled(const Color& reflectance = Color::WHITE,
				float roughness = 0.3f,
				float specular = 0.5f,
				float matellic = 0.0f,
				float specTint = 0.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE | BSDF_GLOSSY), BSDFType::Principled, reflectance)
				, mRoughness(roughness)
				, mSpecular(specular)
				, mMetallic(matellic)
				, mSpecularTint(specTint)
			{
			}
			Principled(const RefPtr<Texture2D<Color>>& pTex,
				float roughness = 0.3f,
				float specular = 0.5f,
				float matellic = 0.0f,
				float specTint = 0.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE | BSDF_GLOSSY), BSDFType::Principled, pTex)
				, mRoughness(roughness)
				, mSpecular(specular)
				, mMetallic(matellic)
				, mSpecularTint(specTint)
			{
			}
			Principled(const char* pFile,
				float roughness = 0.3f,
				float specular = 0.5f,
				float matellic = 0.0f,
				float specTint = 0.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE | BSDF_GLOSSY), BSDFType::Principled, pFile)
				, mRoughness(roughness)
				, mSpecular(specular)
				, mMetallic(matellic)
				, mSpecularTint(specTint)
			{
			}

			Color SampleScattered(const Vector3& _wo,
				const Sample& sample,
				const DifferentialGeom& diffGeom,
				Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL,
				ScatterType* pSampledTypes = NULL) const;

		private:
			float Pdf(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
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
				float specTint = Math::Max(mSpecularTint, mMetallic);
				Color specAlbedo = Math::Lerp(albedo, Color::WHITE, 1.0f - specTint);
				specAlbedo = Math::Lerp(specAlbedo, Color::WHITE, (1.0f - wo.z) * (1.0f - wo.z) * (1.0f - wo.z));

				return albedo * (1.0f - mMetallic) * DiffuseTerm(wo, wi, types) + specAlbedo * SpecularTerm(wo, wi);
			}

			float Eval(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				return 0.0f;
			}

			float DiffuseTerm(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				if (mMetallic == 1.0f)
					return 0.0f;

				Vector3 wh = Math::Normalize(wo + wi);
				float cosD = Math::AbsDot(wh, wi);
				float oneMinusCosL = 1.0f - BSDFCoordinate::AbsCosTheta(wi);
				float oneMinusCosV = 1.0f - BSDFCoordinate::AbsCosTheta(wo);
				float F_D90 = 0.5f + 2.0f * cosD * cosD * mRoughness;

				return float(Math::EDX_INV_PI) * (1.0f + (F_D90 - 1.0f) * oneMinusCosL * oneMinusCosL * oneMinusCosL * oneMinusCosL * oneMinusCosL) *
					(1.0f + (F_D90 - 1.0f) * oneMinusCosV * oneMinusCosV * oneMinusCosV * oneMinusCosV * oneMinusCosV);
			}

			float SpecularTerm(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				if (BSDFCoordinate::CosTheta(wo) * BSDFCoordinate::CosTheta(wi) <= 0.0f)
					return 0.0f;

				Vector3 wh = Math::Normalize(wo + wi);
				float D = GGX_D(wh, mRoughness * mRoughness);
				if (D == 0.0f)
					return 0.0f;

				float normalRef = Math::Lerp(0.0f, 0.08f, mSpecular);
				float F = Fresnel_Schlick(Math::Dot(wo, wh), normalRef);

				float roughForG = (0.5f + 0.5f * mRoughness);
				float G = GGX_G(wo, wi, wh, roughForG * roughForG);

				return F * D * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
			}

			float Fresnel_Schlick(const float cosD, const float normalReflectance) const
			{
				float oneMinusCosD = 1.0f - cosD;
				float fresnel =  normalReflectance +
					(1.0f - normalReflectance) * oneMinusCosD * oneMinusCosD * oneMinusCosD * oneMinusCosD * oneMinusCosD;
				float fresnelConductor = BSDF::FresnelConductor(cosD, 0.4f, 1.6f);

				return Math::Lerp(fresnel, fresnelConductor, mMetallic);
			}

		public:
			int GetParameterCount() const { return BSDF::GetParameterCount() + 4; }
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

				return "";
			}
			bool GetParameter(const string& name, float* pVal, float* pMin = nullptr, float* pMax = nullptr) const
			{
				if (BSDF::GetParameter(name, pVal, pMin, pMax))
					return true;

				if (name == "Roughness")
				{
					*pVal = this->mRoughness;
					if (pMin)
						*pMin = 0.02f;
					if (pMax)
						*pMax = 1.0f;
					return true;
				}
				else if (name == "Specular")
				{
					*pVal = this->mSpecular;
					if (pMin)
						*pMin = 0.0f;
					if (pMax)
						*pMax = 1.0f;
					return true;
				}
				else if (name == "Metallic")
				{
					*pVal = this->mMetallic;
					if (pMin)
						*pMin = 0.0f;
					if (pMax)
						*pMax = 1.0f;
					return true;
				}
				else if (name == "SpecularTint")
				{
					*pVal = this->mSpecularTint;
					if (pMin)
						*pMin = 0.0f;
					if (pMax)
						*pMax = 1.0f;
					return true;
				}

				return false;
			}
			void SetParameter(const string& name, const float value)
			{
				BSDF::SetParameter(name, value);

				if (name == "Roughness")
					this->mRoughness = value;
				else if (name == "Specular")
					this->mSpecular = value;
				else if (name == "Metallic")
					this->mMetallic = value;
				else if (name == "SpecularTint")
					this->mSpecularTint = value;

				return;
			}
		};
	}
}