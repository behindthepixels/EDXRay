#include "RoughConductor.h"
#include "../Core/Sampler.h"

namespace EDX
{
	namespace RayTracer
	{
		Color RoughConductor::SampleScattered(const Vector3& _wo, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf,
			ScatterType types, ScatterType* pSampledTypes) const
		{
			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			Vector3 wo = diffGeom.WorldToLocal(_wo), wi;

			float microfacetPdf;
			float roughness = GetValue(mRoughness.Ptr(), diffGeom, TextureFilter::Linear);
			roughness = Math::Clamp(roughness, 0.02f, 1.0f);
			float sampleRough = roughness * roughness;
			Vector3 wh = GGX_SampleNormal(sample.u, sample.v, &microfacetPdf, sampleRough);

			if (microfacetPdf == 0.0f)
				return 0.0f;

			wi = Math::Reflect(-wo, wh);
			if (BSDFCoordinate::CosTheta(wo) < 0.0f || !BSDFCoordinate::SameHemisphere(wo, wi))
				return Color::BLACK;

			*pvIn = diffGeom.LocalToWorld(wi);

			float dwh_dwi = 1.0f / (4.0f * Math::AbsDot(wi, wh));
			*pPdf = microfacetPdf * dwh_dwi;

			if (Math::Dot(_wo, diffGeom.mGeomNormal) * Math::Dot(*pvIn, diffGeom.mGeomNormal) > 0.0f)
				types = ScatterType(types & ~BSDF_TRANSMISSION);
			else
				types = ScatterType(types & ~BSDF_REFLECTION);

			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			if (pSampledTypes != nullptr)
				*pSampledTypes = mScatterType;

			float D = GGX_D(wh, sampleRough);
			if (D == 0.0f)
				return 0.0f;

			float F = BSDF::FresnelConductor(Math::Dot(wo, wh), 0.4f, 1.6f);
			float G = GGX_G(wo, wi, wh, sampleRough);

			return GetValue(mpTexture.Ptr(), diffGeom) * F * D * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
		}
	}
}