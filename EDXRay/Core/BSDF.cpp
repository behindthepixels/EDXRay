#include "BSDF.h"
#include "Math/EDXMath.h"
#include "DifferentialGeom.h"
#include "Sampling.h"
#include "Sampler.h"

namespace EDX
{
	namespace RayTracer
	{
		// -----------------------------------------------------------------------------------------------------------------------
		// BSDF interface implementation
		// -----------------------------------------------------------------------------------------------------------------------
		BSDF::BSDF(ScatterType t, BSDFType t2, const Color& color)
			: mType(t), mBSDFType(t2)
		{
			mBaseColor = color;
		}


		Color BSDF::Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types) const
		{
			if (Math::Dot(vOut, diffGoem.mGeomNormal) * Math::Dot(vIn, diffGoem.mGeomNormal) > 0.0f)
				types = ScatterType(types & ~BSDF_TRANSMISSION);
			else
				types = ScatterType(types & ~BSDF_REFLECTION);

			if (!MatchesTypes(types))
			{
				return Color::BLACK;
			}

			Vector3 vWo = diffGoem.WorldToLocal(vOut);
			Vector3 vWi = diffGoem.WorldToLocal(vIn);

			return GetColor(diffGoem) * Eval(vWo, vWi, types);
		}

		float BSDF::PDF(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types /* = BSDF_ALL */) const
		{
			if (!MatchesTypes(types))
			{
				return 0.0f;
			}

			Vector3 vWo = diffGoem.WorldToLocal(vOut);
			Vector3 vWi = diffGoem.WorldToLocal(vIn);

			return PDF(vWo, vWi, types);
		}

		Color BSDF::GetColor(const DifferentialGeom& diffGoem) const
		{
			return mBaseColor;
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// Lambertian BRDF Implementation
		// -----------------------------------------------------------------------------------------------------------------------
		Color LambertianDiffuse::Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types) const
		{
			if (!BSDFCoordinate::SameHemisphere(vOut, vIn))
			{
				return Color::BLACK;
			}

			return float(Math::EDX_INV_PI);
		}

		float LambertianDiffuse::PDF(const Vector3& vOut, const Vector3& vIn, ScatterType types /* = BSDF_ALL */) const
		{
			if (!BSDFCoordinate::SameHemisphere(vOut, vIn))
			{
				return 0.0f;
			}

			return BSDFCoordinate::AbsCosTheta(vIn) * float(Math::EDX_INV_PI);
		}

		Color LambertianDiffuse::SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pfPDF,
			ScatterType types, ScatterType* pSampledTypes) const
		{
			if (!MatchesTypes(types))
			{
				*pfPDF = 0.0f;
				return Color::BLACK;
			}

			Vector3 vWo = diffGoem.WorldToLocal(vOut), vWi;
			vWi = Sampling::CosineSampleHemisphere(sample.u, sample.v);

			if (vWo.z < 0.0f)
				vWi.z *= -1.0f;

			*pvIn = diffGoem.LocalToWorld(vWi);
			*pfPDF = PDF(vWo, vWi, types);
			if (pSampledTypes != NULL)
			{
				*pSampledTypes = mType;
			}

			return GetColor(diffGoem) * Eval(vWo, vWi, types);
		}
	}
}