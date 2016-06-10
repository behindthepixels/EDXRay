#include "BSDF.h"
#include "../BSDFs/RoughConductor.h"
#include "../BSDFs/RoughDielectric.h"
#include "../BSDFs/Disney.h"
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
				return new RoughConductor(color, 0.3f);
			case BSDFType::RoughDielectric:
				return new RoughDielectric(color, 0.5f);
			case BSDFType::Disney:
				return new Disney(color);
			}

			assert(0);
			return NULL;
		}
		BSDF* BSDF::CreateBSDF(const BSDFType type, const RefPtr<Texture2D<Color>>& pTex, const RefPtr<Texture2D<Color>>& pNormal)
		{
			switch (type)
			{
			case BSDFType::Diffuse:
				return new LambertianDiffuse(pTex, pNormal);
			case BSDFType::Mirror:
				return new Mirror(pTex, pNormal);
			case BSDFType::Glass:
				return new Glass(pTex, pNormal);
			case BSDFType::RoughConductor:
				return new RoughConductor(pTex, pNormal, 0.3f);
			case BSDFType::RoughDielectric:
				return new RoughDielectric(pTex, pNormal, 0.3f);
			case BSDFType::Disney:
				return new Disney(pTex, pNormal);
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
				return new RoughConductor(strTexPath, 0.3f);
			case BSDFType::RoughDielectric:
				return new RoughDielectric(strTexPath, 0.3f);
			case BSDFType::Disney:
				return new Disney(strTexPath);
			}

			assert(0);
			return NULL;
		}

		BSDF::BSDF(ScatterType t, BSDFType t2, const Color& color)
			: mScatterType(t), mBSDFType(t2)
		{
			mpTexture = new ConstantTexture2D<Color>(color);
		}
		BSDF::BSDF(ScatterType t, BSDFType t2, const RefPtr<Texture2D<Color>>& pTex, const RefPtr<Texture2D<Color>>& pNormal)
			: mScatterType(t), mBSDFType(t2)
		{
			mpTexture = pTex;
			mpNormalMap = pNormal;
		}
		BSDF::BSDF(ScatterType t, BSDFType t2, const char* pFile)
			: mScatterType(t), mBSDFType(t2)
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

			return GetValue(mpTexture.Ptr(), diffGeom) * EvalInner(vWo, vWi, diffGeom, types);
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

			return PdfInner(vWo, vWi, diffGeom, types);
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
			float alphaSqr = alpha * alpha;
			float tanThetaMSqr = alphaSqr * u1 / (1.0f - u1 + 1e-10f);
			float cosThetaH = 1.0f / std::sqrt(1 + tanThetaMSqr);

			float cosThetaH2 = cosThetaH * cosThetaH,
				cosThetaH3 = cosThetaH2 * cosThetaH,
				temp = alphaSqr + tanThetaMSqr;

			*pPdf = float(Math::EDX_INV_PI) * alphaSqr / (cosThetaH3 * temp * temp);

			float sinThetaH = Math::Sqrt(Math::Max(0.0f, 1.0f - cosThetaH2));
			float phiH = u2 * float(Math::EDX_TWO_PI);

			Vector3 dir = Math::SphericalDirection(sinThetaH, cosThetaH, phiH);
			return Vector3(dir.x, dir.z, dir.y);
		}

		float BSDF::SmithG(const Vector3& v, const Vector3& wh, float alpha)
		{
			const float tanTheta = Math::Abs(BSDFCoordinate::TanTheta(v));

			if (tanTheta == 0.0f)
				return 1.0f;

			if (Math::Dot(v, wh) * BSDFCoordinate::CosTheta(v) <= 0)
				return 0.0f;

			const float root = alpha * tanTheta;
			return 2.0f / (1.0f + std::sqrt(1.0f + root*root));
		}

		float BSDF::GGX_G(const Vector3& wo, const Vector3& wi, const Vector3& wh, float alpha)
		{
			return SmithG(wo, wh, alpha) * SmithG(wi, wh, alpha);
		}

		float BSDF::GGX_Pdf(const Vector3& wh, float alpha)
		{
			return GGX_D(wh, alpha) * BSDFCoordinate::CosTheta(wh);
		}

		Vector2 BSDF::ImportanceSampleGGX_VisibleNormal_Unit(float thetaI, float u1, float u2)
		{
			Vector2 Slope;

			// Special case (normal incidence)
			if (thetaI < 1e-4f)
			{
				float SinPhi, CosPhi;
				float R = Math::Sqrt(Math::Max(u1 / ((1 - u1) + 1e-6f), 0.0f));
				Math::SinCos(2 * float(Math::EDX_PI) * u2, SinPhi, CosPhi);
				return Vector2(R * CosPhi, R * SinPhi);
			}

			// Precomputations
			float TanThetaI = tan(thetaI);
			float a = 1 / TanThetaI;
			float G1 = 2.0f / (1.0f + sqrt(max(1.0f + 1.0f / (a*a), 0.0f)));

			// Simulate X component
			float A = 2.0f * u1 / G1 - 1.0f;
			if (Math::Abs(A) == 1)
				A -= (A >= 0.0f ? 1.0f : -1.0f) * 1e-4f;

			float Temp = 1.0f / (A*A - 1.0f);
			float B = TanThetaI;
			float D = sqrt(max(B*B*Temp*Temp - (A*A - B*B) * Temp, 0.0f));
			float Slope_x_1 = B * Temp - D;
			float Slope_x_2 = B * Temp + D;
			Slope.x = (A < 0.0f || Slope_x_2 > 1.0f / TanThetaI) ? Slope_x_1 : Slope_x_2;

			// Simulate Y component
			float S;
			if (u2 > 0.5f)
			{
				S = 1.0f;
				u2 = 2.0f * (u2 - 0.5f);
			}
			else
			{
				S = -1.0f;
				u2 = 2.0f * (0.5f - u2);
			}

			// Improved fit
			float z =
				(u2 * (u2 * (u2 * (-0.365728915865723) + 0.790235037209296) -
				0.424965825137544) + 0.000152998850436920) /
				(u2 * (u2 * (u2 * (u2 * 0.169507819808272 - 0.397203533833404) -
				0.232500544458471) + 1) - 0.539825872510702);

			Slope.y = S * z * sqrt(1.0f + Slope.x * Slope.x);

			return Slope;
		}

		float BSDF::GGX_Pdf_VisibleNormal(const Vector3& wi, const Vector3& H, float Alpha)
		{
			float D = GGX_D(H, Alpha);

			return SmithG(wi, H, Alpha) * Math::AbsDot(wi, H) * D / (BSDFCoordinate::AbsCosTheta(wi) + 1e-4f);
		}


		Vector3 BSDF::GGX_SampleVisibleNormal(const Vector3& _wi, float u1, float u2, float* pPdf, float Alpha)
		{
			// Stretch wi
			Vector3 wi = Math::Normalize(Vector3(
				Alpha * _wi.x,
				Alpha * _wi.y,
				_wi.z
				));

			// Get polar coordinates
			float Theta = 0, Phi = 0;
			if (wi.z < float(0.99999f))
			{
				Theta = Math::Acos(wi.z);
				Phi = Math::Atan2(wi.y, wi.x);
			}
			float SinPhi, CosPhi;
			Math::SinCos(Phi, SinPhi, CosPhi);

			// Simulate P22_{wi}(Slope.x, Slope.y, 1, 1)
			Vector2 Slope = ImportanceSampleGGX_VisibleNormal_Unit(Theta, u1, u2);

			// Step 3: rotate
			Slope = Vector2(
				CosPhi * Slope.x - SinPhi * Slope.y,
				SinPhi * Slope.x + CosPhi * Slope.y);

			// Unstretch
			Slope.x *= Alpha;
			Slope.y *= Alpha;

			// Compute normal
			float Normalization = (float)1 / Math::Sqrt(Slope.x*Slope.x
				+ Slope.y*Slope.y + (float) 1.0);

			Vector3 H = Vector3(
				-Slope.x * Normalization,
				-Slope.y * Normalization,
				Normalization
				);

			*pPdf = GGX_Pdf_VisibleNormal(_wi, H, Alpha);

			return H;
		}


		// -----------------------------------------------------------------------------------------------------------------------
		// Lambertian BRDF Implementation
		// -----------------------------------------------------------------------------------------------------------------------
		float LambertianDiffuse::EvalInner(const Vector3& wo, const Vector3& wi, const DifferentialGeom& diffGeom, ScatterType types) const
		{
			if (BSDFCoordinate::CosTheta(wo) <= 0.0f || !BSDFCoordinate::SameHemisphere(wo, wi))
				return 0.0f;

			return float(Math::EDX_INV_PI);
		}

		float LambertianDiffuse::PdfInner(const Vector3& wo, const Vector3& wi, const DifferentialGeom& diffGeom, ScatterType types /* = BSDF_ALL */) const
		{
			if (BSDFCoordinate::CosTheta(wo) <= 0.0f || !BSDFCoordinate::SameHemisphere(wo, wi))
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
			if (BSDFCoordinate::CosTheta(vWo) <= 0.0f || !BSDFCoordinate::SameHemisphere(vWo, vWi))
				return 0.0f;

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

			*pPdf = PdfInner(vWo, vWi, diffGeom, types);

			if (pSampledTypes != NULL)
			{
				*pSampledTypes = mScatterType;
			}

			return GetValue(mpTexture.Ptr(), diffGeom) * EvalInner(vWo, vWi, diffGeom, types);
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// Mirror Implementation
		// -----------------------------------------------------------------------------------------------------------------------
		Color Mirror::Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const
		{
			return Color::BLACK;
		}

		float Mirror::EvalInner(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const
		{
			return 0.0f;
		}

		float Mirror::Pdf(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		float Mirror::PdfInner(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types /* = BSDF_ALL */) const
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

			return GetValue(mpTexture.Ptr(), diffGeom) / BSDFCoordinate::AbsCosTheta(vWi);
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// Glass Implementation
		// -----------------------------------------------------------------------------------------------------------------------
		Color Glass::Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const
		{
			return Color::BLACK;
		}

		float Glass::EvalInner(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types) const
		{
			return 0.0f;
		}

		float Glass::Pdf(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types /* = BSDF_ALL */) const
		{
			return 0.0f;
		}

		float Glass::PdfInner(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types /* = BSDF_ALL */) const
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

				return fresnel * Color::WHITE / BSDFCoordinate::AbsCosTheta(vWi);
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

				return (1.0f - fresnel) * eta * eta * GetValue(mpTexture.Ptr(), diffGeom) / BSDFCoordinate::AbsCosTheta(vWi);
			}

			return Color::BLACK;
		}
	}
}