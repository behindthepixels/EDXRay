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
			Vector3 wo = diffGeom.WorldToLocal(_wo), wi;

			float microfacetPdf;
			Vector3 wh = GGX_SampleNormal(sample.u, sample.v, &microfacetPdf, mRoughness);
			if (microfacetPdf == 0.0f)
				return 0.0f;

			float fresnel = FresnelDielectric(Math::Dot(wo, wh), mEtai, mEtat);
			float prob = fresnel;


			if (sample.w <= prob && sampleBoth || (sampleReflect && !sampleBoth)) // Sample reflection
			{
				wi = Math::Reflect(-wo, wh);

				*pvIn = diffGeom.LocalToWorld(wi);
				*pPdf = !sampleBoth ? microfacetPdf : microfacetPdf * prob;
				if (pSampledTypes != nullptr)
					*pSampledTypes = ReflectScatter;

				return GetColor(diffGeom) * fresnel * GGX_D(wh, mRoughness) * GGX_G(wo, wi, wh, mRoughness) * Math::AbsDot(wo, wh) /
					(BSDFCoordinate::AbsCosTheta(wo) * BSDFCoordinate::AbsCosTheta(wh));
			}
			else if (sample.w > prob && sampleBoth || (sampleRefract && !sampleBoth)) // Sample refraction
			{
				wi = Math::Refract(-wo, wh, mEtat / mEtai);
				*pvIn = diffGeom.LocalToWorld(wi);
				*pPdf = !sampleBoth ? microfacetPdf : microfacetPdf * (1.0f - prob);
				if (pSampledTypes != nullptr)
					*pSampledTypes = RefractScatter;

				return GetColor(diffGeom) * (1.0f - fresnel) * GGX_D(wh, mRoughness) * GGX_G(wo, wi, wh, mRoughness) * Math::AbsDot(wo, wh) /
					(BSDFCoordinate::AbsCosTheta(wo) * BSDFCoordinate::AbsCosTheta(wh));
			}

			return Color::BLACK;
		}
	}
}