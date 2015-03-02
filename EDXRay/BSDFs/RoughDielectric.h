#pragma once

#include "../Core/BSDF.h"

namespace EDX
{
	namespace RayTracer
	{
		class RoughDielectric : public BSDF
		{
		private:
			float mRoughness;
			float mEtai, mEtat;

			static const ScatterType ReflectScatter = ScatterType(BSDF_REFLECTION | BSDF_GLOSSY);
			static const ScatterType RefractScatter = ScatterType(BSDF_TRANSMISSION | BSDF_GLOSSY);

		public:
			RoughDielectric(const Color& reflectance = Color::WHITE, float roughness = 0.06f, float etai = 1.0f, float etat = 1.5f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY), BSDFType::RoughDielectric, reflectance)
				, mEtai(etai)
				, mEtat(etat)
				, mRoughness(roughness)
			{
			}
			RoughDielectric(const RefPtr<Texture2D<Color>>& pTex, float roughness = 0.06f, float etai = 1.0f, float etat = 1.5f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY), BSDFType::RoughDielectric, pTex)
				, mEtai(etai)
				, mEtat(etat)
				, mRoughness(roughness)
			{
			}
			RoughDielectric(const char* pFile, float roughness = 0.05f, float etai = 1.0f, float etat = 1.5f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY), BSDFType::RoughDielectric, pFile)
				, mEtai(etai)
				, mEtat(etat)
				, mRoughness(roughness)
			{
			}

			Color SampleScattered(const Vector3& _wo,
				const Sample& sample,
				const DifferentialGeom& diffGeom,
				Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL,
				ScatterType* pSampledTypes = NULL) const;

		private:
			float Pdf(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types /* = BSDF_ALL */) const
			{
				Vector3 vWo = diffGeom.WorldToLocal(vOut);
				Vector3 vWi = diffGeom.WorldToLocal(vIn);

				return Pdf(vWo, vWi, types);
			}

			float Pdf(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				bool sampleReflect = (types & ReflectScatter) == ReflectScatter;
				bool sampleRefract = (types & RefractScatter) == RefractScatter;
				const float ODotN = BSDFCoordinate::CosTheta(wo), IDotN = BSDFCoordinate::CosTheta(wi);
				const float fac = ODotN * IDotN;
				if (fac == 0.0f)
					return 0.0f;

				bool reflect = fac > 0.0f;
				bool entering = ODotN > 0.0f;

				Vector3 wh;
				float dwh_dwi;
				if (reflect)
				{
					if (!sampleReflect)
						return 0.0f;

					wh = Math::Normalize(wo + wi);
					if (!entering)
						wh *= -1.0f;

					dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh));
				}
				else
				{
					if (!sampleRefract)
						return 0.0f;

					float etai = mEtai, etat = mEtat;
					if (!entering)
						swap(etai, etat);

					wh = -Math::Normalize(etai * wo + etat * wi);

					const float ODotH = Math::Dot(wo, wh), IDotH = Math::Dot(wi, wh);
					float sqrtDenom = etai * ODotH + etat * IDotH;
					dwh_dwi = (etat * etat * Math::Abs(IDotH)) / (sqrtDenom * sqrtDenom);
				}

				float whProb = GGX_Pdf(wh, mRoughness);
				if (sampleReflect && sampleRefract)
				{
					float F = BSDF::FresnelDielectric(Math::Dot(wo, wh), mEtai, mEtat);
					//F = 0.5f * F + 0.25f;
					whProb *= reflect ? F : 1.0f - F;
				}

				assert(Math::NumericValid(whProb));
				assert(Math::NumericValid(dwh_dwi));
				return Math::Abs(whProb * dwh_dwi);
			}

			Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const
			{
				Vector3 vWo = diffGeom.WorldToLocal(vOut);
				Vector3 vWi = diffGeom.WorldToLocal(vIn);

				return GetColor(diffGeom) * Eval(vWo, vWi, types);
			}

			float Eval(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				const float ODotN = BSDFCoordinate::CosTheta(wo), IDotN = BSDFCoordinate::CosTheta(wi);
				const float fac = ODotN * IDotN;
				if (fac == 0.0f)
					return 0.0f;

				bool reflect = fac > 0.0f;
				bool entering = ODotN > 0.0f;

				float etai = mEtai, etat = mEtat;
				if (!entering)
					swap(etai, etat);

				Vector3 wh;
				if (reflect)
				{
					if ((ReflectScatter & types) != ReflectScatter)
						return 0.0f;

					wh = wo + wi;
					if (wh == Vector3::ZERO)
						return 0.0f;

					wh = Math::Normalize(wh);
					if (!entering)
						wh *= -1.0f;
				}
				else
				{
					if ((RefractScatter & types) != RefractScatter)
						return 0.0f;

					wh = -Math::Normalize(etai * wo + etat * wi);
				}

				float D = GGX_D(wh, mRoughness);
				if (D == 0.0f)
					return 0.0f;

				float F = BSDF::FresnelDielectric(Math::Dot(wo, wh), mEtai, mEtat);
				float G = GGX_G(wo, wi, wh, mRoughness);

				if (reflect)
				{
					return F * D * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
				}
				else
				{
					const float ODotH = Math::Dot(wo, wh), IDotH = Math::Dot(wi, wh);

					float sqrtDenom = etai * ODotH + etat * IDotH;
					float value = ((1 - F) * D * G * etat * etat * ODotH * IDotH) /
						(sqrtDenom * sqrtDenom * ODotN * IDotN);

					// TODO: Fix solid angle compression when tracing radiance
					float factor = 1.0f;

					assert(Math::NumericValid(value));
					return Math::Abs(value * factor * factor);
				}
			}

		public:
			int GetParameterCount() const { return BSDF::GetParameterCount() + 2; }
			string GetParameterName(const int idx) const
			{
				if (idx < BSDF::GetParameterCount())
					return BSDF::GetParameterName(idx);

				int baseParamCount = BSDF::GetParameterCount();
				if (idx == baseParamCount + 0)
					return "Roughness";
				else if (idx == baseParamCount + 1)
					return "IOR";

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
						*pMin = 0.001f;
					if (pMax)
						*pMax = 1.0f;
					return true;
				}
				else if (name == "IOR")
				{
					*pVal = this->mEtat;
					if (pMin)
						*pMin = 1.0f;
					if (pMax)
						*pMax = 1.8f;
					return true;
				}

				return false;
			}
			void SetParameter(const string& name, const float value)
			{
				BSDF::SetParameter(name, value);

				if (name == "Roughness")
					this->mRoughness = value;
				else if (name == "IOR")
					this->mEtat = value;

				return;
			}
		};
	}
}