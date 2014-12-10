#pragma once

#include "../Core/BSDF.h"

namespace EDX
{
	namespace RayTracer
	{
		class RoughConductor : public BSDF
		{
		private:
			float mRoughness;

		public:
			RoughConductor(const Color reflectance = Color::WHITE, float roughness = 1.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_GLOSSY), BSDFType::Diffuse, reflectance)
				, mRoughness(roughness)
			{
			}
			RoughConductor(const char* pFile, float roughness = 1.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_GLOSSY), BSDFType::Diffuse, pFile)
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
			float Pdf(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				bool reflect = BSDFCoordinate::CosTheta(wi) * BSDFCoordinate::CosTheta(wo) > 0.0f;
				if (!reflect)
					return 0.0f;

				Vector3 wh = Math::Normalize(wo + wi);
				if (BSDFCoordinate::CosTheta(wh) < 0.0f)
					wh *= -1.0f;

				float dwh_dwi = 1.0f / (4.0f * Math::Dot(wi, wh));
				float whProb = GGX_Pdf(wh, mRoughness);

				return Math::Abs(whProb * dwh_dwi);
			}

			float Eval(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				bool reflect = BSDFCoordinate::CosTheta(wi) * BSDFCoordinate::CosTheta(wo) > 0.0f;
				if (!reflect)
					return 0.0f;

				Vector3 wh = Math::Normalize(wo + wi);
				float D = GGX_D(wh, mRoughness);
				if (D == 0.0f)
					return 0.0f;

				float F = BSDF::FresnelConductor(Math::Dot(wo, wh), 0.4f, 1.6f);
				float G = GGX_G(wo, wi, wh, mRoughness);

				return F * D * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
			}
		};
	}
}