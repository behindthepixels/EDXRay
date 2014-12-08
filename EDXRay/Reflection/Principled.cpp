#include "Principled.h"
#include "../Core/Sampler.h"

namespace EDX
{
	namespace RayTracer
	{
		Color Principled::SampleScattered(const Vector3& _wo, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pPdf,
			ScatterType types, ScatterType* pSampledTypes) const
		{
			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			Vector3 wo = diffGoem.WorldToLocal(_wo), wi;

			float microfacetPdf;
			Vector3 wh = GGX_SampleNormal(sample.u, sample.v, &microfacetPdf, mRoughness);

			if (microfacetPdf == 0.0f)
				return 0.0f;

			wi = Math::Reflect(-wo, wh);
			*pvIn = diffGoem.LocalToWorld(wi);

			if (BSDFCoordinate::CosTheta(wo) * BSDFCoordinate::CosTheta(wi) <= 0.0f)
				return Color::BLACK;

			float dwh_dwi = 1.0f / (4.0f * Math::Dot(wi, wh));
			*pPdf = microfacetPdf * dwh_dwi;

			return GetColor(diffGoem) * Eval(wo, wi, types);
		}
	}
}