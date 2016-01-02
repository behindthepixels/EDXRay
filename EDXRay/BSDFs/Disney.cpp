#include "Disney.h"
#include "../Core/Sampling.h"
#include "../Core/Sampler.h"

#include <ctime>

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

			*pPdf = 0.0f;
			const Vector3 wo = diffGeom.WorldToLocal(_wo);
			Vector3 wi;
			Vector3 wh_Coat;
			bool sampleCoat = false;
			float probCoat = 0.0f;
			if (mClearCoat > 0.0f)
			{
				float coatRough = Math::Lerp(0.005f, 0.1f, mClearCoatGloss);
				float coatPdf = 0.0f;
				wh_Coat = GGX_SampleVisibleNormal(wo, rand() / float(RAND_MAX), rand() / float(RAND_MAX), &coatPdf, coatRough);
				if (coatPdf > 0.0f)
				{
					probCoat = mClearCoat * Fresnel_Schlick_Coat(Math::AbsDot(wo, wh_Coat));

					if ((rand() / float(RAND_MAX)) < probCoat)
					{
						sampleCoat = true;

						wi = Math::Reflect(-wo, wh_Coat);

						*pvIn = diffGeom.LocalToWorld(wi);

						float dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh_Coat));
						coatPdf *= probCoat * dwh_dwi;
						*pPdf += coatPdf;

						if (pSampledTypes != nullptr)
							*pSampledTypes = ScatterType(BSDF_REFLECTION | BSDF_GLOSSY);
					}
				}

			}

			float microfacetPdf;
			float roughness = GetValue(mRoughness.Ptr(), diffGeom, TextureFilter::Linear);
			roughness = Math::Clamp(roughness, 0.02f, 1.0f);

			Vector3 wh;
			if (!sampleCoat)
			{
				wh = GGX_SampleVisibleNormal(wo, sample.u, sample.v, &microfacetPdf, roughness * roughness);
			}
			else
			{
				wh = wh_Coat;
				microfacetPdf = GGX_Pdf_VisibleNormal(wo, wh, roughness * roughness);
			}

			if (wh == Vector3::ZERO || microfacetPdf == 0.0f)
				return 0.0f;

			float normalRef = Math::Lerp(0.0f, 0.08f, mSpecular);
			float ODotH = Math::Dot(wo, wh);
			float F = Fresnel_Schlick(ODotH, normalRef);
			float prob = F;

			if (sample.w <= prob) // Sample reflection
			{
				if (!sampleCoat)
				{
					wi = Math::Reflect(-wo, wh);
					*pvIn = diffGeom.LocalToWorld(wi);

					if (pSampledTypes != nullptr)
						*pSampledTypes = ScatterType(BSDF_REFLECTION | BSDF_GLOSSY);
				}

				float specPdf = microfacetPdf * prob;
				float dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh));
				specPdf *= dwh_dwi;
				*pPdf += specPdf * (1.0f - probCoat);
				*pPdf += BSDFCoordinate::AbsCosTheta(wi) * float(Math::EDX_INV_PI) * (1.0f - prob) * (1.0f - probCoat);
			}
			else
			{
				if (!sampleCoat)
				{
					wi = Sampling::CosineSampleHemisphere(sample.u, sample.v);
					if (wo.z < 0.0f)
						wi.z *= -1.0f;

					*pvIn = diffGeom.LocalToWorld(wi);

					wh = Math::Normalize(wo + wi);
					ODotH = Math::Dot(wo, wh);

					if (pSampledTypes != nullptr)
						*pSampledTypes = ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE);
				}

				float diffusePdf = BSDFCoordinate::AbsCosTheta(wi) * float(Math::EDX_INV_PI) * (1.0f - prob);
				float specPdf = GGX_Pdf_VisibleNormal(wo, wh, roughness * roughness) * prob;
				float dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh));
				specPdf *= dwh_dwi;

				*pPdf += diffusePdf * (1.0f - probCoat);
				*pPdf += specPdf * (1.0f - probCoat);
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
			Color albedo = GetValue(mpTexture.Ptr(), diffGeom);
			Color specAlbedo = Math::Lerp(Math::Lerp(albedo, Color::WHITE, 1.0f - mSpecularTint), albedo, mMetallic);
			Color sheenAlbedo = Math::Lerp(Color::WHITE, albedo, mSheenTint);

			float OneMinusODotH = 1.0f - Math::Dot(wo, wh);
			specAlbedo = Math::Lerp(specAlbedo, Color::WHITE, OneMinusODotH * OneMinusODotH * OneMinusODotH);

			// Sheen term
			Color sheenTerm = Fresnel_Schlick(ODotH, 0.0f) * mSheen * sheenAlbedo;

			return (1.0f - mMetallic)
				* (albedo * Math::Lerp(DiffuseTerm(wo, wi, IDotH, roughness), SubsurfaceTerm(wo, wi, IDotH, roughness), mSubsurface)
				+ sheenTerm)
				+ specAlbedo * SpecularTerm(wo, wi, wh, ODotH, roughness, &F)
				+ Color(ClearCoatTerm(wo, wi, wh, IDotH, mClearCoatGloss));
		}
	}
}