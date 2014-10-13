#pragma once

#include "../ForwardDecl.h"
#include "DifferentialGeom.h"
#include "Math/Vector.h"
#include "Graphics/Texture.h"
#include "Graphics/Color.h"
#include "Memory/RefPtr.h"

namespace EDX
{
	namespace RayTracer
	{
		enum ScatterType
		{
			BSDF_REFLECTION = 1 << 0,
			BSDF_TRANSMISSION = 1 << 1,
			BSDF_DIFFUSE = 1 << 2,
			BSDF_GLOSSY = 1 << 3,
			BSDF_SPECULAR = 1 << 4,
			BSDF_ALL_TYPES = BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_SPECULAR,
			BSDF_ALL_REFLECTION = BSDF_REFLECTION | BSDF_ALL_TYPES,
			BSDF_ALL_TRANSMISSION = BSDF_TRANSMISSION | BSDF_ALL_TYPES,
			BSDF_ALL = BSDF_ALL_REFLECTION | BSDF_ALL_TRANSMISSION
		};

		enum class BSDFType
		{
			Diffuse, Mirror, Glass
		};

		class BSDF
		{
		public:
			const ScatterType mScatterType;
			const BSDFType mBSDFType;
			RefPtr<Texture2D<Color>> mpTexture;

		public:
			BSDF(ScatterType t, BSDFType t2, const Color& color);
			BSDF(ScatterType t, BSDFType t2, const char* pFile);
			virtual ~BSDF() {}

			bool MatchesTypes(ScatterType flags) const { return (mScatterType & flags) == mScatterType; }
			bool IsSpecular() const { return (ScatterType(BSDF_SPECULAR | BSDF_DIFFUSE | BSDF_GLOSSY) & mScatterType) == ScatterType(BSDF_SPECULAR); }

			virtual Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types = BSDF_ALL) const;
			virtual float Pdf(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types = BSDF_ALL) const;
			virtual Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const = 0;

			__forceinline const Color GetColor(const DifferentialGeom& diffGoem) const
			{
				Vector2 differential[2] = {
					(diffGoem.mDudx, diffGoem.mDvdx),
					(diffGoem.mDudy, diffGoem.mDvdy)
				};
				return mpTexture->Sample(diffGoem.mTexcoord, differential, TextureFilter::Linear);
			}

			const ScatterType GetScatterType() const { return mScatterType; }
			const BSDFType GetBSDFType() const { return mBSDFType; }
			const Texture2D<Color>* GetTexture() const { return mpTexture.Ptr(); }

			static BSDF* CreateBSDF(const BSDFType type, const Color& color);
			static BSDF* CreateBSDF(const BSDFType type, const char* strTexPath);

		private:
			virtual Color Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types = BSDF_ALL) const = 0;
			virtual float Pdf(const Vector3& vOut, const Vector3& vIn, ScatterType types = BSDF_ALL) const = 0;
		};

		class LambertianDiffuse : public BSDF
		{
		public:
			LambertianDiffuse(const Color cColor = Color::WHITE)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE), BSDFType::Diffuse, cColor)
			{
			}
			LambertianDiffuse(const char* pFile)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE), BSDFType::Diffuse, pFile)
			{
			}

			Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const;

		private:
			float Pdf(const Vector3& vIn, const Vector3& vOut, ScatterType types = BSDF_ALL) const;
			Color Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types = BSDF_ALL) const;
		};

		class Mirror : public BSDF
		{
		public:
			Mirror(const Color cColor = Color::WHITE)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_SPECULAR), BSDFType::Mirror, cColor)
			{
			}
			Mirror(const char* strTexPath)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_SPECULAR), BSDFType::Mirror, strTexPath)
			{
			}

			Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types = BSDF_ALL) const;
			float Pdf(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGoem, ScatterType types = BSDF_ALL) const;
			Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const;

		private:
			Color Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types = BSDF_ALL) const;
			float Pdf(const Vector3& vIn, const Vector3& vOut, ScatterType types = BSDF_ALL) const;
		};

		class Glass : public BSDF
		{
		private:
			float mEtai, mEtat;

		public:
			Glass(const Color cColor = Color::WHITE, float etai = 1.0f, float etat = 1.5f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_SPECULAR), BSDFType::Glass, cColor)
				, mEtai(etai)
				, mEtat(etat)
			{
			}
			Glass(const char* strTexPath, float etai = 1.0f, float etat = 1.5f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_SPECULAR), BSDFType::Glass, strTexPath)
				, mEtai(etai)
				, mEtat(etat)
			{
			}

			Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types = BSDF_ALL) const;
			float Pdf(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGoem, ScatterType types = BSDF_ALL) const;
			Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const;

		private:
			Color Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types = BSDF_ALL) const;
			float Pdf(const Vector3& vIn, const Vector3& vOut, ScatterType types = BSDF_ALL) const;
			float Fresnel(float fCosi) const;
		};

		namespace BSDFCoordinate
		{
			inline float CosTheta(const Vector3& vec) { return vec.z; }
			inline float AbsCosTheta(const Vector3& vec) { return Math::Abs(vec.z); }
			inline float SinTheta2(const Vector3& vec) { return max(0.0f, 1.0f - CosTheta(vec) * CosTheta(vec)); }
			inline float SinTheta(const Vector3& vec) { return Math::Sqrt(SinTheta2(vec)); }
			inline float CosPhi(const Vector3& vec)
			{
				float sintheta = SinTheta(vec);
				if (sintheta == 0.0f) return 1.0f;
				return Math::Clamp(vec.x / sintheta, -1.0f, 1.0f);
			}

			inline float SinPhi(const Vector3& vec)
			{
				float sintheta = SinTheta(vec);
				if (sintheta == 0.0f)
					return 0.0f;
				return Math::Clamp(vec.y / sintheta, -1.0f, 1.0f);
			}

			inline bool SameHemisphere(const Vector3& vec1, const Vector3& vec2) { return vec1.z * vec2.z > 0.0f; }
		};
	}
}