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
			Vector3 wi, wh;
			bool sampleCoat = false;
			Sample remappedSample = sample;

			if (mClearCoat > 0.0f)
			{
				Color albedo = GetValue(mpTexture.Ptr(), diffGeom);
				float coatWeight = mClearCoat / (mClearCoat + albedo.Luminance());
				float FresnelCoat = Fresnel_Schlick_Coat(BSDFCoordinate::AbsCosTheta(wo));
				float probCoat = (FresnelCoat * coatWeight) /
					(FresnelCoat * coatWeight +
					(1 - FresnelCoat) * (1 - coatWeight));

				if (sample.v < probCoat)
				{
					sampleCoat = true;
					remappedSample.v /= probCoat;

					float coatRough = Math::Lerp(0.005f, 0.1f, mClearCoatGloss);
					float coatPdf;
					wh = GGX_SampleVisibleNormal(wo, remappedSample.u, remappedSample.v, &coatPdf, coatRough);
					wi = Math::Reflect(-wo, wh);
				}
				else
				{
					sampleCoat = false;
					remappedSample.v = (sample.v - probCoat) / (1.0f - probCoat);
				}
			}


			if (!sampleCoat)
			{
				float normalRef = Math::Lerp(0.0f, 0.08f, mSpecular);
				float ODotH = Math::Dot(wo, wh);
				float probSpec = Fresnel_Schlick(ODotH, normalRef);

				if (remappedSample.w <= probSpec)
				{
					float microfacetPdf;
					float roughness = GetValue(mRoughness.Ptr(), diffGeom, TextureFilter::Linear);
					roughness = Math::Clamp(roughness, 0.02f, 1.0f);

					wh = GGX_SampleVisibleNormal(wo, remappedSample.u, remappedSample.v, &microfacetPdf, roughness * roughness);
					wi = Math::Reflect(-wo, wh);
				}
				else
				{
					wi = Sampling::CosineSampleHemisphere(remappedSample.u, remappedSample.v);
					if (wo.z < 0.0f)
						wi.z *= -1.0f;
				}
			}

			if (wi.z <= 0.0f)
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			*pvIn = diffGeom.LocalToWorld(wi);
			*pPdf = PdfInner(wo, wi, diffGeom, types);

			if (Math::Dot(_wo, diffGeom.mGeomNormal) * Math::Dot(*pvIn, diffGeom.mGeomNormal) > 0.0f)
				types = ScatterType(types & ~BSDF_TRANSMISSION);
			else
				types = ScatterType(types & ~BSDF_REFLECTION);

			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			return EvalTransformed(wo, wi, diffGeom, types);
		}
	}
}