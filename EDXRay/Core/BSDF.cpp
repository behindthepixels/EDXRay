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
		BSDF* BSDF::CreateBSDF(const BSDFType type, const Color& color)
		{
			switch (type)
			{
			case BSDFType::Diffuse:
				return new LambertianDiffuse(color);
			case BSDFType::Mirror:
				return new Mirror(color);
			case BSDFType::Glass:
				return new Glass(color);
			}

			assert(0);
			return NULL;
		}
		BSDF* BSDF::CreateBSDF(const BSDFType type, const char* strTexPath)
		{
			switch (type)
			{
			case BSDFType::Diffuse:
				return new LambertianDiffuse(strTexPath);
			case BSDFType::Mirror:
				return new Mirror(strTexPath);
			case BSDFType::Glass:
				return new Glass(strTexPath);
			}

			assert(0);
			return NULL;
		}

		BSDF::BSDF(ScatterType t, BSDFType t2, const Color& color)
			: mType(t), mBSDFType(t2)
		{
			mpTexture = new ConstantTexture2D<Color>(color);
		}
		BSDF::BSDF(ScatterType t, BSDFType t2, const char* pFile)
			: mType(t), mBSDFType(t2)
		{
			mpTexture = new ImageTexture<Color, Color4b>(pFile);
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
			mpTexture->SetFilter(TextureFilter::Linear);
			return mpTexture->Sample(diffGoem.mTexcoord, nullptr);
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

		// -----------------------------------------------------------------------------------------------------------------------
		// Mirror Implementation
		// -----------------------------------------------------------------------------------------------------------------------
		Color Mirror::Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types) const
		{
			return Color::BLACK;
		}

		Color Mirror::Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types) const
		{
			return Color::BLACK;
		}

		float Mirror::PDF(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGoem, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		float Mirror::PDF(const Vector3& vIn, const Vector3& vOut, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		Color Mirror::SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pfPDF, ScatterType types /* = BSDF_ALL */, ScatterType* pSampledTypes /* = NULL */) const
		{
			if (!MatchesTypes(types))
			{
				*pfPDF = 0.0f;
				return Color::BLACK;
			}

			Vector3 vWo = diffGoem.WorldToLocal(vOut), vWi;
			vWi = Vector3(-vWo.x, -vWo.y, vWo.z);

			*pvIn = diffGoem.LocalToWorld(vWi);
			*pfPDF = 1.0f;
			if (pSampledTypes != NULL)
			{
				*pSampledTypes = mType;
			}

			return GetColor(diffGoem) / BSDFCoordinate::AbsCosTheta(vWi);
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// Glass Implementation
		// -----------------------------------------------------------------------------------------------------------------------
		Color Glass::Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types) const
		{
			return Color::BLACK;
		}

		Color Glass::Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types) const
		{
			return Color::BLACK;
		}

		float Glass::PDF(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGoem, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		float Glass::PDF(const Vector3& vIn, const Vector3& vOut, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		Color Glass::SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pfPDF, ScatterType types /* = BSDF_ALL */, ScatterType* pSampledTypes /* = NULL */) const
		{
			bool bSampleReflect = (types & mTypeRef) == mTypeRef;
			bool bSampleRefract = (types & mTypeRfr) == mTypeRfr;

			if (!bSampleReflect && !bSampleRefract)
			{
				*pfPDF = 0.0f;
				return Color::BLACK;
			}

			bool bSampleBoth = bSampleReflect == bSampleRefract;

			Vector3 vWo = diffGoem.WorldToLocal(vOut), vWi;

			float fFresnel = Fresnel(BSDFCoordinate::CosTheta(vWo));
			float fProb = fFresnel;//0.5f * fFresnel + 0.25f;

			if (sample.w <= fProb && bSampleBoth || (bSampleReflect && !bSampleBoth)) // Sample reflection
			{
				vWi = Vector3(-vWo.x, -vWo.y, vWo.z);

				*pvIn = diffGoem.LocalToWorld(vWi);
				*pfPDF = !bSampleBoth ? 1.0f : fProb;
				if (pSampledTypes != NULL)
				{
					*pSampledTypes = mTypeRef;
				}

				return fFresnel * GetColor(diffGoem) / BSDFCoordinate::AbsCosTheta(vWi);
			}
			else if (sample.w > fProb && bSampleBoth || (bSampleRefract && !bSampleBoth)) // Sample refraction
			{
				bool bEntering = BSDFCoordinate::CosTheta(vWo) > 0.0f;
				float etai = mfEtai, etat = mfEtat;
				if (!bEntering)
					swap(etai, etat);

				float fSini2 = BSDFCoordinate::SinTheta2(vWo);
				float fEta = etai / etat;
				float fSint2 = fEta * fEta * fSini2;

				if (fSint2 > 1.0f)
					return Color::BLACK;

				float fCost = Math::Sqrt(Math::Max(0.0f, 1.0f - fSint2));
				if (bEntering)
					fCost = -fCost;
				float fSintOverSini = fEta;

				vWi = Vector3(fSintOverSini * -vWo.x, fSintOverSini * -vWo.y, fCost);

				*pvIn = diffGoem.LocalToWorld(vWi);
				*pfPDF = !bSampleBoth ? 1.0f : 1.0f - fProb;
				if (pSampledTypes != NULL)
				{
					*pSampledTypes = mTypeRfr;
				}

				return (1.0f - fFresnel) * GetColor(diffGoem) / BSDFCoordinate::AbsCosTheta(vWi);
			}

			return Color::BLACK;
		}

		float Glass::Fresnel(float fCosi) const
		{
			fCosi = Math::Clamp(fCosi, -1.0f, 1.0f);

			bool bEntering = fCosi > 0.0f;
			float ei = mfEtai, et = mfEtat;
			if (!bEntering)
				swap(ei, et);

			float fSint = ei / et * Math::Sqrt(Math::Max(0.0f, 1.0f - fCosi * fCosi));

			if (fSint >= 1.0f)
				return 1.0f;
			else
			{
				float fCost = Math::Sqrt(Math::Max(0.0f, 1.0f - fSint * fSint));
				fCosi = Math::Abs(fCosi);

				float fPara = ((mfEtat * fCosi) - (mfEtai * fCost)) /
					((mfEtat * fCosi) + (mfEtai * fCost));
				float fPerp = ((mfEtai * fCosi) - (mfEtat * fCost)) /
					((mfEtai * fCosi) + (mfEtat * fCost));

				return (fPara * fPara + fPerp * fPerp) / 2.0f;
			}
		}
	}
}