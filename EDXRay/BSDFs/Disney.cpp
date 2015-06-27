#include "Disney.h"
#include "../Core/Sampling.h"
#include "../Core/Sampler.h"

namespace EDX
{
	namespace RayTracer
	{
		Color Disney::SampleScattered(const Vector3& _wo,
			const Sample& sample,
			const DifferentialGeom& diffGeom,
			Vector3* pvIn,
			float* pPdf,
			ScatterType types,
			ScatterType* pSampledTypes) const
		{
			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			const Vector3 wo = diffGeom.WorldToLocal(_wo);

			float microfacetPdf;
			Vector3 wh = GGX_SampleNormal(sample.u, sample.v, &microfacetPdf, mRoughness * mRoughness);
			if (microfacetPdf == 0.0f)
				return 0.0f;

			float normalRef = Math::Lerp(0.0f, 0.08f, mSpecular);
			float ODotH = Math::Dot(wo, wh);
			float F = Fresnel_Schlick(ODotH, normalRef);
			float prob = F;

			Vector3 wi;
			if (sample.w <= prob) // Sample reflection
			{
				wi = Math::Reflect(-wo, wh);
				if (BSDFCoordinate::CosTheta(wo) < 0.0f || !BSDFCoordinate::SameHemisphere(wo, wi))
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
				if (BSDFCoordinate::CosTheta(wo) < 0.0f || !BSDFCoordinate::SameHemisphere(wo, wi))
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

				wh = Math::Normalize(wo + wi);
				ODotH = Math::Dot(wo, wh);
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

			float IDotH = Math::Dot(wi, wh);
			Color albedo = GetColor(diffGeom);
			Color specAlbedo = Math::Lerp(Math::Lerp(albedo, Color::WHITE, 1.0f - mSpecularTint), albedo, mMetallic);
			Color sheenAlbedo = Math::Lerp(Color::WHITE, albedo, mSheenTint);

			float OneMinusODotH = 1.0f - Math::Dot(wo, wh);
			specAlbedo = Math::Lerp(specAlbedo, Color::WHITE, OneMinusODotH * OneMinusODotH * OneMinusODotH);

			// Sheen term
			Color sheenTerm = Fresnel_Schlick(ODotH, 0.0f) * mSheen * sheenAlbedo;

			return (1.0f - mMetallic) * (albedo * Math::Lerp(DiffuseTerm(wo, wi, IDotH), SubsurfaceTerm(wo, wi, IDotH), mSubsurface) + sheenTerm) + specAlbedo * SpecularTerm(wo, wi, wh, ODotH, &F);
		}
	}
}