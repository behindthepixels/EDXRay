#include "Principled.h"
#include "../Core/Sampling.h"
#include "../Core/Sampler.h"

namespace EDX
{
	namespace RayTracer
	{
		Color Principled::SampleScattered(const Vector3& _wo, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf,
			ScatterType types, ScatterType* pSampledTypes) const
		{
			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			const Vector3 wo = diffGeom.WorldToLocal(_wo);

			float microfacetPdf;
			const Vector3 wh = GGX_SampleNormal(sample.u, sample.v, &microfacetPdf, mRoughness * mRoughness);
			if (microfacetPdf == 0.0f)
				return 0.0f;

			float normalRef = Math::Lerp(0.0f, 0.08f, mSpecular);
			float F = Fresnel_Schlick(Math::Dot(wo, wh), normalRef);
			float prob = F;

			Vector3 wi;
			if (sample.w <= prob) // Sample reflection
			{
				wi = Math::Reflect(-wo, wh);
				if (BSDFCoordinate::CosTheta(wi) * BSDFCoordinate::CosTheta(wo) <= 0.0f)
				{
					*pPdf = 0.0f;
					return Color::BLACK;
				}

				*pvIn = diffGeom.LocalToWorld(wi);

				*pPdf = microfacetPdf * prob;
				float dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh));
				*pPdf *= dwh_dwi;
				*pPdf += BSDFCoordinate::AbsCosTheta(wi) * float(Math::EDX_INV_PI) * (1.0f - prob);

				if (pSampledTypes != nullptr)
					*pSampledTypes = ScatterType(BSDF_REFLECTION | BSDF_GLOSSY);
			}
			else
			{
				wi = Sampling::CosineSampleHemisphere(sample.u, sample.v);
				if (BSDFCoordinate::CosTheta(wo) * BSDFCoordinate::CosTheta(wi) <= 0.0f)
				{
					*pPdf = 0.0f;
					return Color::BLACK;
				}

				if (wo.z < 0.0f)
					wi.z *= -1.0f;

				*pvIn = diffGeom.LocalToWorld(wi);
				*pPdf = BSDFCoordinate::AbsCosTheta(wi) * float(Math::EDX_INV_PI) * (1.0f - prob);
				float specPdf = microfacetPdf * prob;
				float dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh));
				specPdf *= dwh_dwi;
				*pPdf += specPdf;

				if (pSampledTypes != nullptr)
					*pSampledTypes = ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE);
			}

			if (Math::Dot(_wo, diffGeom.mGeomNormal) * Math::Dot(*pvIn, diffGeom.mGeomNormal) > 0.0f)
				types = ScatterType(types & ~BSDF_TRANSMISSION);
			else
				types = ScatterType(types & ~BSDF_REFLECTION);

			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			Color albedo = GetColor(diffGeom);
			float specTint = Math::Max(mSpecularTint, mMetallic);
			Color specAlbedo = Math::Lerp(albedo, Color::WHITE, 1.0f - specTint);

			float OneMinusODotH = 1.0f - Math::Dot(wo, wh);
			specAlbedo = Math::Lerp(specAlbedo, Color::WHITE, OneMinusODotH * OneMinusODotH * OneMinusODotH);

			return albedo * (1.0f - mMetallic) * DiffuseTerm(wo, wi, types) + specAlbedo * SpecularTerm(wo, wi);
		}
	}
}