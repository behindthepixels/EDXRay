#include "BSDF.h"
#include "../BSDFs/RoughConductor.h"
#include "../BSDFs/RoughDielectric.h"
#include "../BSDFs/Principled.h"
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
			case BSDFType::RoughConductor:
				return new RoughConductor(color, 0.08f);
			case BSDFType::RoughDielectric:
				return new RoughDielectric(color, 0.3f);
			case BSDFType::Principled:
				return new Principled(color);
			}

			assert(0);
			return NULL;
		}
		BSDF* BSDF::CreateBSDF(const BSDFType type, const RefPtr<Texture2D<Color>>& pTex, const bool isTextured)
		{
			switch (type)
			{
			case BSDFType::Diffuse:
				return new LambertianDiffuse(pTex, isTextured);
			case BSDFType::Mirror:
				return new Mirror(pTex, isTextured);
			case BSDFType::Glass:
				return new Glass(pTex, isTextured);
			case BSDFType::RoughConductor:
				return new RoughConductor(pTex, isTextured, 0.08f);
			case BSDFType::RoughDielectric:
				return new RoughDielectric(pTex, isTextured, 0.3f);
			case BSDFType::Principled:
				return new Principled(pTex, isTextured);
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
			case BSDFType::RoughConductor:
				return new RoughConductor(strTexPath, 0.08f);
			case BSDFType::RoughDielectric:
				return new RoughDielectric(strTexPath, 0.3f);
			case BSDFType::Principled:
				return new Principled(strTexPath);
			}

			assert(0);
			return NULL;
		}

		BSDF::BSDF(ScatterType t, BSDFType t2, const Color& color)
			: mScatterType(t), mBSDFType(t2), mTextured(false)
		{
			mpTexture = new ConstantTexture2D<Color>(color);
		}
		BSDF::BSDF(ScatterType t, BSDFType t2, const RefPtr<Texture2D<Color>>& pTex, const bool isTextured)
			: mScatterType(t), mBSDFType(t2), mTextured(isTextured)
		{
			mpTexture = pTex;
		}
		BSDF::BSDF(ScatterType t, BSDFType t2, const char* pFile)
			: mScatterType(t), mBSDFType(t2), mTextured(true)
		{
			mpTexture = new ImageTexture<Color, Color4b>(pFile);
		}


		Color BSDF::Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const
		{
			if (Math::Dot(vOut, diffGeom.mGeomNormal) * Math::Dot(vIn, diffGeom.mGeomNormal) > 0.0f)
				types = ScatterType(types & ~BSDF_TRANSMISSION);
			else
				types = ScatterType(types & ~BSDF_REFLECTION);

			if (!MatchesTypes(types))
			{
				return Color::BLACK;
			}

			Vector3 vWo = diffGeom.WorldToLocal(vOut);
			Vector3 vWi = diffGeom.WorldToLocal(vIn);

			return GetColor(diffGeom) * Eval(vWo, vWi, types);
		}

		float BSDF::Pdf(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types /* = BSDF_ALL */) const
		{
			if (Math::Dot(vOut, diffGeom.mGeomNormal) * Math::Dot(vIn, diffGeom.mGeomNormal) > 0.0f)
				types = ScatterType(types & ~BSDF_TRANSMISSION);
			else
				types = ScatterType(types & ~BSDF_REFLECTION);

			if (!MatchesTypes(types))
			{
				return 0.0f;
			}

			Vector3 vWo = diffGeom.WorldToLocal(vOut);
			Vector3 vWi = diffGeom.WorldToLocal(vIn);

			return Pdf(vWo, vWi, types);
		}

		float BSDF::FresnelDielectric(float cosi, float etai, float etat)
		{
			cosi = Math::Clamp(cosi, -1.0f, 1.0f);

			bool entering = cosi > 0.0f;
			float ei = etai, et = etat;
			if (!entering)
				swap(ei, et);

			float sint = ei / et * Math::Sqrt(Math::Max(0.0f, 1.0f - cosi * cosi));

			if (sint >= 1.0f)
				return 1.0f;
			else
			{
				float cost = Math::Sqrt(Math::Max(0.0f, 1.0f - sint * sint));
				cosi = Math::Abs(cosi);

				float para = ((etat * cosi) - (etai * cost)) /
					((etat * cosi) + (etai * cost));
				float perp = ((etai * cosi) - (etat * cost)) /
					((etai * cosi) + (etat * cost));

				return 0.5f * (para * para + perp * perp);
			}
		}

		float BSDF::FresnelConductor(float cosi, const float& eta, const float k)
		{
			float tmp = (eta*eta + k*k) * cosi * cosi;
			float Rparl2 = (tmp - (2.f * eta * cosi) + 1) /
				(tmp + (2.f * eta * cosi) + 1);
			float tmp_f = eta*eta + k*k;
			float Rperp2 =
				(tmp_f - (2.f * eta * cosi) + cosi * cosi) /
				(tmp_f + (2.f * eta * cosi) + cosi * cosi);

			return 0.5f * (Rparl2 + Rperp2);
		}

		float BSDF::GGX_D(const Vector3& wh, float alpha)
		{
			if (wh.z <= 0.0f)
				return 0.0f;

			const float tanTheta2 = BSDFCoordinate::TanTheta2(wh),
				cosTheta2 = BSDFCoordinate::CosTheta2(wh);

			const float root = alpha / (cosTheta2 * (alpha * alpha + tanTheta2));

			return float(Math::EDX_INV_PI) * (root * root);
		}

		Vector3 BSDF::GGX_SampleNormal(float u1, float u2, float* pPdf, float alpha)
		{
			float roughnessSqr = alpha * alpha;
			float tanThetaMSqr = roughnessSqr * u1 / (1.0f - u1 + 1e-10f);
			float cosThetaH = 1.0f / std::sqrt(1 + tanThetaMSqr);

			float cosThetaH2 = cosThetaH * cosThetaH,
				cosThetaH3 = cosThetaH2 * cosThetaH,
				temp = roughnessSqr + tanThetaMSqr;

			*pPdf = float(Math::EDX_INV_PI) * roughnessSqr / (cosThetaH3 * temp * temp);

			float sinThetaH = Math::Sqrt(Math::Max(0.0f, 1.0f - cosThetaH2));
			float phiH = u2 * float(Math::EDX_TWO_PI);

			Vector3 dir = Math::SphericalDirection(sinThetaH, cosThetaH, phiH);
			return Vector3(dir.x, dir.z, dir.y);
		}

		float BSDF::GGX_G(const Vector3& wo, const Vector3& wi, const Vector3& wh, float alpha)
		{
			auto SmithG1 = [&](const Vector3& v, const Vector3& wh)
			{
				const float tanTheta = Math::Abs(BSDFCoordinate::TanTheta(v));

				if (tanTheta == 0.0f)
					return 1.0f;

				if (Math::Dot(v, wh) * BSDFCoordinate::CosTheta(v) <= 0)
					return 0.0f;

				const float root = alpha * tanTheta;
				return 2.0f / (1.0f + std::sqrt(1.0f + root*root));
			};

			return SmithG1(wo, wh) * SmithG1(wi, wh);
		}

		float BSDF::GGX_Pdf(const Vector3& wh, float alpha)
		{
			return GGX_D(wh, alpha) * BSDFCoordinate::CosTheta(wh);
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// Lambertian BRDF Implementation
		// -----------------------------------------------------------------------------------------------------------------------
		float LambertianDiffuse::Eval(const Vector3& wo, const Vector3& wi, ScatterType types) const
		{
			return float(Math::EDX_INV_PI);
		}

		float LambertianDiffuse::Pdf(const Vector3& wo, const Vector3& wi, ScatterType types /* = BSDF_ALL */) const
		{
			if (!BSDFCoordinate::SameHemisphere(wo, wi))
				return 0.0f;

			return BSDFCoordinate::AbsCosTheta(wi) * float(Math::EDX_INV_PI);
		}

		Color LambertianDiffuse::SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf,
			ScatterType types, ScatterType* pSampledTypes) const
		{
			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			Vector3 vWo = diffGeom.WorldToLocal(vOut), vWi;
			vWi = Sampling::CosineSampleHemisphere(sample.u, sample.v);

			if (vWo.z < 0.0f)
				vWi.z *= -1.0f;

			*pvIn = diffGeom.LocalToWorld(vWi);

			if (Math::Dot(vOut, diffGeom.mGeomNormal) * Math::Dot(*pvIn, diffGeom.mGeomNormal) > 0.0f)
				types = ScatterType(types & ~BSDF_TRANSMISSION);
			else
				types = ScatterType(types & ~BSDF_REFLECTION);

			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			*pPdf = Pdf(vWo, vWi, types);
			if (pSampledTypes != NULL)
			{
				*pSampledTypes = mScatterType;
			}

			return GetColor(diffGeom) * Eval(vWo, vWi, types);
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// Mirror Implementation
		// -----------------------------------------------------------------------------------------------------------------------
		Color Mirror::Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const
		{
			return Color::BLACK;
		}

		float Mirror::Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types) const
		{
			return 0.0f;
		}

		float Mirror::Pdf(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		float Mirror::Pdf(const Vector3& vIn, const Vector3& vOut, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		Color Mirror::SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf, ScatterType types /* = BSDF_ALL */, ScatterType* pSampledTypes /* = NULL */) const
		{
			if (!MatchesTypes(types))
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			Vector3 vWo = diffGeom.WorldToLocal(vOut), vWi;
			vWi = Vector3(-vWo.x, -vWo.y, vWo.z);

			*pvIn = diffGeom.LocalToWorld(vWi);
			*pPdf = 1.0f;
			if (pSampledTypes != NULL)
			{
				*pSampledTypes = mScatterType;
			}

			return GetColor(diffGeom) / BSDFCoordinate::AbsCosTheta(vWi);
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// Glass Implementation
		// -----------------------------------------------------------------------------------------------------------------------
		Color Glass::Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const
		{
			return Color::BLACK;
		}

		float Glass::Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types) const
		{
			return 0.0f;
		}

		float Glass::Pdf(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		float Glass::Pdf(const Vector3& vIn, const Vector3& vOut, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		Color Glass::SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf, ScatterType types /* = BSDF_ALL */, ScatterType* pSampledTypes /* = NULL */) const
		{
			bool sampleReflect = (types & (BSDF_REFLECTION | BSDF_SPECULAR)) == (BSDF_REFLECTION | BSDF_SPECULAR);
			bool sampleRefract = (types & (BSDF_TRANSMISSION | BSDF_SPECULAR)) == (BSDF_TRANSMISSION | BSDF_SPECULAR);

			if (!sampleReflect && !sampleRefract)
			{
				*pPdf = 0.0f;
				return Color::BLACK;
			}

			bool sampleBoth = sampleReflect == sampleRefract;

			Vector3 vWo = diffGeom.WorldToLocal(vOut), vWi;

			float fresnel = FresnelDielectric(BSDFCoordinate::CosTheta(vWo), mEtai, mEtat);
			float prob = 0.5f * fresnel + 0.25f;

			if (sample.w <= prob && sampleBoth || (sampleReflect && !sampleBoth)) // Sample reflection
			{
				vWi = Vector3(-vWo.x, -vWo.y, vWo.z);

				*pvIn = diffGeom.LocalToWorld(vWi);
				*pPdf = !sampleBoth ? 1.0f : prob;
				if (pSampledTypes != NULL)
				{
					*pSampledTypes = ScatterType(BSDF_REFLECTION | BSDF_SPECULAR);
				}

				return fresnel * GetColor(diffGeom).Luminance() / BSDFCoordinate::AbsCosTheta(vWi);
			}
			else if (sample.w > prob && sampleBoth || (sampleRefract && !sampleBoth)) // Sample refraction
			{
				bool entering = BSDFCoordinate::CosTheta(vWo) > 0.0f;
				float etai = mEtai, etat = mEtat;
				if (!entering)
					swap(etai, etat);

				float sini2 = BSDFCoordinate::SinTheta2(vWo);
				float eta = etai / etat;
				float sint2 = eta * eta * sini2;

				if (sint2 > 1.0f)
					return Color::BLACK;

				float cost = Math::Sqrt(Math::Max(0.0f, 1.0f - sint2));
				if (entering)
					cost = -cost;
				float sintOverSini = eta;

				vWi = Vector3(sintOverSini * -vWo.x, sintOverSini * -vWo.y, cost);

				*pvIn = diffGeom.LocalToWorld(vWi);
				*pPdf = !sampleBoth ? 1.0f : 1.0f - prob;
				if (pSampledTypes != NULL)
				{
					*pSampledTypes = ScatterType(BSDF_TRANSMISSION | BSDF_SPECULAR);
				}

				return (1.0f - fresnel) * GetColor(diffGeom) / BSDFCoordinate::AbsCosTheta(vWi);
			}

			return Color::BLACK;
		}
	}
}