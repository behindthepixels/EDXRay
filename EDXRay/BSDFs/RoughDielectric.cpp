#include "RoughDielectric.h"
#include "../Core/Sampler.h"

namespace EDX
{
	namespace RayTracer
	{
		Color RoughDielectric::SampleScattered(const Vector3& _wo, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf,
			ScatterType types, ScatterType* pSampledTypes) const
		{
			bool sampleReflect = (types & ReflectScatter) == ReflectScatter;
			bool sampleRefract = (types & RefractScatter) == RefractScatter;

			if (!sampleReflect && !sampleRefract)
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			bool sampleBoth = sampleReflect == sampleRefract;
			const Vector3 wo = diffGeom.WorldToLocal(_wo);

			float roughness = GetValue(mRoughness.Get(), diffGeom, TextureFilter::Linear);
			roughness = Math::Clamp(roughness, 0.02f, 1.0f);
			float sampleRough = roughness * roughness;

			float microfacetPdf;
			const Vector3 wh = GGX_SampleVisibleNormal(Math::Sign(BSDFCoordinate::CosTheta(wo)) * wo, sample.u, sample.v, &microfacetPdf, sampleRough);
			if (microfacetPdf == 0.0f)
				return 0.0f;

			float D = GGX_D(wh, sampleRough);
			if (D == 0.0f)
				return 0.0f;

			float F = FresnelDielectric(Math::Dot(wo, wh), mEtai, mEtat);
			float prob = F;

			Vector3 wi;
			if (sample.w <= prob && sampleBoth || (sampleReflect && !sampleBoth)) // Sample reflection
			{
				wi = Math::Reflect(-wo, wh);
				if (BSDFCoordinate::CosTheta(wi) * BSDFCoordinate::CosTheta(wo) <= 0.0f)
				{
					*pPdf = 0.0f;
					return Color::BLACK;
				}

				*pvIn = diffGeom.LocalToWorld(wi);

				*pPdf = !sampleBoth ? microfacetPdf : microfacetPdf * prob;
				float dwh_dwi = 1.0f / (4.0f * Math::Dot(wi, wh));
				*pPdf *= Math::Abs(dwh_dwi);

				float G = GGX_G(wo, wi, wh, sampleRough);

				if (pSampledTypes != nullptr)
					*pSampledTypes = ReflectScatter;


				return GetValue(mpTexture.Get(), diffGeom) * Math::Abs(F * D * G / (4.0f * BSDFCoordinate::CosTheta(wi) * BSDFCoordinate::CosTheta(wo)));
			}
			else if (sample.w > prob && sampleBoth || (sampleRefract && !sampleBoth)) // Sample refraction
			{
				wi = Math::Refract(-wo, wh, mEtat / mEtai);
				if (BSDFCoordinate::CosTheta(wi) * BSDFCoordinate::CosTheta(wo) >= 0.0f || wi == Vector3::ZERO)
				{
					*pPdf = 0.0f;
					return Color::BLACK;
				}

				*pvIn = diffGeom.LocalToWorld(wi);

				*pPdf = !sampleBoth ? microfacetPdf : microfacetPdf * (1.0f - prob);
				bool entering = BSDFCoordinate::CosTheta(wo) > 0.0f;
				float etai = mEtai, etat = mEtat;
				if (!entering)
					Swap(etai, etat);

				const float ODotH = Math::Dot(wo, wh), IDotH = Math::Dot(wi, wh);
				float sqrtDenom = etai * ODotH + etat * IDotH;
				if (sqrtDenom == 0.0f)
				{
					*pPdf = 0.0f;
					return Color::BLACK;
				}

				float dwh_dwi = (etat * etat * IDotH) / (sqrtDenom * sqrtDenom);
				*pPdf *= Math::Abs(dwh_dwi);

				if (pSampledTypes != nullptr)
					*pSampledTypes = RefractScatter;

				float G = GGX_G(wo, wi, wh, sampleRough);

				float value = ((1 - F) * D * G * etat * etat * ODotH * IDotH) /
					(sqrtDenom * sqrtDenom * BSDFCoordinate::CosTheta(wo) * BSDFCoordinate::CosTheta(wi));

				// TODO: Fix solid angle compression when tracing radiance
				float factor = 1.0f;

				return GetValue(mpTexture.Get(), diffGeom) * Math::Abs(value * factor * factor);
			}

			return Color::BLACK;
		}
	}
}