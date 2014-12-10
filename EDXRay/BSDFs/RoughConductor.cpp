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
			Vector3 wh = GGX_SampleNormal(sample.u, sample.v, &microfacetPdf, mRoughness);

			if (microfacetPdf == 0.0f)
				return 0.0f;

			wi = Math::Reflect(-wo, wh);
			*pvIn = diffGeom.LocalToWorld(wi);

			if (BSDFCoordinate::CosTheta(wo) * BSDFCoordinate::CosTheta(wi) <= 0.0f)
				return Color::BLACK;

			float dwh_dwi = 1.0f / (4.0f * Math::Dot(wi, wh));
			*pPdf = microfacetPdf * dwh_dwi;

			if (pSampledTypes != nullptr)
				*pSampledTypes = mScatterType;

			return GetColor(diffGeom) * Eval(wo, wi, types);
		}
	}
}