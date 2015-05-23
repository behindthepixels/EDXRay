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

			float microfacetPdf;
			const Vector3 wh = GGX_SampleNormal(sample.u, sample.v, &microfacetPdf, mRoughness * mRoughness);
			if (microfacetPdf == 0.0f)
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
				float dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh));
				*pPdf *= dwh_dwi;

				if (pSampledTypes != nullptr)
					*pSampledTypes = ReflectScatter;


				return GetColor(diffGeom) * Eval(wo, wi, types);
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
					swap(etai, etat);

				const float ODotH = Math::Dot(wo, wh), IDotH = Math::Dot(wi, wh);
				float sqrtDenom = etai * ODotH + etat * IDotH;
				float dwh_dwi = (etat * etat * Math::Abs(IDotH)) / (sqrtDenom * sqrtDenom);
				*pPdf *= dwh_dwi;

				if (pSampledTypes != nullptr)
					*pSampledTypes = RefractScatter;

				return GetColor(diffGeom) * Eval(wo, wi, types);
			}

			return Color::BLACK;
		}
	}
}