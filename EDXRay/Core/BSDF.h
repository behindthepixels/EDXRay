#pragma once

#include "../ForwardDecl.h"
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
			const ScatterType mType;
			const BSDFType mBSDFType;
			RefPtr<Texture2D<Color>> mpTexture;

		public:
			BSDF(ScatterType t, BSDFType t2, const Color& color);
			BSDF(ScatterType t, BSDFType t2, const char* pFile);
			virtual ~BSDF() {}

			bool MatchesTypes(ScatterType flags) const { return (mType & flags) == mType; }
			bool IsSpecular() const { return (ScatterType(BSDF_SPECULAR | BSDF_DIFFUSE | BSDF_GLOSSY) & mType) == ScatterType(BSDF_SPECULAR); }
			virtual Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types = BSDF_ALL) const;
			virtual float PDF(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGoem, ScatterType types = BSDF_ALL) const;
			virtual Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pfPDF,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const = 0;
			Color GetColor(const DifferentialGeom& diffGoem) const;
			BSDFType GetBSDFType() const { return mBSDFType; }

			static BSDF* CreateBSDF(const BSDFType type, const Color color);
			static BSDF* CreateBSDF(const BSDFType type, const char* strTexPath);

		private:
			virtual Color Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types = BSDF_ALL) const = 0;
			virtual float PDF(const Vector3& vOut, const Vector3& vIn, ScatterType types = BSDF_ALL) const = 0;
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

			Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGoem, Vector3* pvIn, float* pfPDF,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const;

		private:
			float PDF(const Vector3& vIn, const Vector3& vOut, ScatterType types = BSDF_ALL) const;
			Color Eval(const Vector3& vOut, const Vector3& vIn, ScatterType types = BSDF_ALL) const;
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