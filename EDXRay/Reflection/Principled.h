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

		public:
			Principled(const Color reflectance = Color::WHITE, float roughness = 1.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_GLOSSY), BSDFType::Diffuse, reflectance)
				, mRoughness(roughness)
			{
			}
			Principled(const char* pFile, float roughness = 1.0f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_GLOSSY), BSDFType::Diffuse, pFile)
				, mRoughness(roughness)
			{
			}

			Color SampleScattered(const Vector3& _wo,
				const Sample& sample,
				const DifferentialGeom& diffGoem,
				Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL,
				ScatterType* pSampledTypes = NULL) const;

		private:
			float Pdf(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				bool reflect = BSDFCoordinate::CosTheta(wi) > 0 && BSDFCoordinate::CosTheta(wo);
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
				bool reflect = BSDFCoordinate::CosTheta(wi) > 0 && BSDFCoordinate::CosTheta(wo);
				if (!reflect)
					return 0.0f;

				Vector3 wh = Math::Normalize(wo + wi);
				float D = GGX_D(wh, mRoughness);
				if (D == 0.0f)
					return 0.0f;

				float F = BSDF::Fresnel(Math::Dot(wi, wh), 1.5, 1.0f);
				float G = GGX_G(wo, wi, wh, mRoughness);

				return F * D * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
			}

		private:
			static float GGX_D(const Vector3& wh, float alpha)
			{
				const float tanTheta2 = BSDFCoordinate::TanTheta2(wh),
					cosTheta2 = BSDFCoordinate::CosTheta2(wh);

				const float root = alpha / (cosTheta2 * (alpha * alpha + tanTheta2));

				return float(Math::EDX_INV_PI) * (root * root);
			}

			static Vector3 GGX_SampleNormal(float u1, float u2, float* pPdf, float alpha)
			{
				float roughnessSqr = alpha * alpha;
				float tanThetaMSqr = roughnessSqr * u1 / (1.0f - u1);
				float cosThetaH = 1.0f / std::sqrt(1 + tanThetaMSqr);

				float cosThetaH2 = cosThetaH * cosThetaH,
					cosThetaH3 = cosThetaH2 * cosThetaH,
					temp = roughnessSqr + tanThetaMSqr;

				*pPdf = float(Math::EDX_INV_PI) * roughnessSqr / (cosThetaH3 * temp * temp);

				float sinThetaH = Math::Sqrt(Math::Max(0.0f, 1.0f - cosThetaH2));
				float phiH = u2 * float(Math::EDX_TWO_PI);

				Vector3 dir = Math::SphericalDirection(sinThetaH, cosThetaH, phiH);
				return Vector3(dir.x, dir.z, dir.y);
			}

			static float GGX_G(const Vector3& wo, const Vector3& wi, const Vector3& wh, float alpha)
			{
				auto SmithG1 = [&](const Vector3& v, const Vector3& wh)
				{
					const float tanTheta = Math::Abs(BSDFCoordinate::TanTheta(v));

					if (tanTheta == 0.0f)
						return 1.0f;

					if (Math::Dot(v, wh) * BSDFCoordinate::CosTheta(v) <= 0)
						return 0.0f;

					const float root = alpha * tanTheta;
					return 2.0f / (1.0f + std::sqrt(1.0f + root*root));
				};

				return SmithG1(wo, wh) * SmithG1(wi, wh);
			}

			static float GGX_Pdf(const Vector3& wh, float alpha)
			{
				return GGX_D(wh, alpha) * BSDFCoordinate::CosTheta(wh);
			}

		};
	}
}