#pragma once

#include "../ForwardDecl.h"
#include "DifferentialGeom.h"
#include "Math/Vector.h"
#include "Graphics/Texture.h"
#include "Graphics/Color.h"

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
			Diffuse,
			Mirror,
			Glass,
			RoughConductor,
			RoughDielectric,
			Disney
		};

		struct Parameter
		{
			enum
			{
				Float,
				Color,
				TextureMap,
				NormalMap,
				None
			} Type;

			union
			{
				struct { float R; float G; float B; };
				struct { float Value; float Min; float Max; };
				char TexPath[260 /*MAX_PATH*/];
			};
		};

		class IEditable
		{
		public:
			virtual int GetParameterCount() const = 0;
			virtual String GetParameterName(const int idx) const = 0;
			virtual Parameter GetParameter(const String& name) const = 0;
			virtual void SetParameter(const String& name, const Parameter& param) = 0;
		};

		class BSDF : public IEditable
		{
		protected:
			const ScatterType mScatterType;
			const BSDFType mBSDFType;
			UniquePtr<Texture2D<Color>> mpTexture;
			UniquePtr<Texture2D<Color>> mpNormalMap;

		public:
			BSDF(ScatterType t, BSDFType t2, const Color& color);
			BSDF(ScatterType t, BSDFType t2, UniquePtr<Texture2D<Color>> pTex, UniquePtr<Texture2D<Color>> pNormal);
			BSDF(ScatterType t, BSDFType t2, const char* pFile);
			virtual ~BSDF() {}

			bool MatchesTypes(ScatterType flags) const { return (mScatterType & flags) == mScatterType; }
			bool IsSpecular() const { return (ScatterType(BSDF_SPECULAR | BSDF_DIFFUSE | BSDF_GLOSSY) & mScatterType) == ScatterType(BSDF_SPECULAR); }

			virtual Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const;
			virtual float Pdf(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const;
			virtual Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const = 0;

			template<typename T>
			__forceinline const T GetValue(const Texture2D<T>* pTex,
				const DifferentialGeom& diffGeom,
				const TextureFilter filter = TextureFilter::TriLinear) const
			{
				Vector2 differential[2] = {
					(diffGeom.mDudx, diffGeom.mDvdx),
					(diffGeom.mDudy, diffGeom.mDvdy)
				};
				return pTex->Sample(diffGeom.mTexcoord, differential, filter);
			}

			const ScatterType GetScatterType() const { return mScatterType; }
			const BSDFType GetBSDFType() const { return mBSDFType; }

			const Texture2D<Color>* GetTexture() const { return mpTexture.Get(); }
			const Texture2D<Color>* GetNormalMap() const { return mpNormalMap.Get(); }

			UniquePtr<Texture2D<Color>> MoveTexture() { return Move(mpTexture); }
			UniquePtr<Texture2D<Color>> MoveNormalMap() { return Move(mpNormalMap); }

		protected:
			virtual float EvalInner(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const = 0;
			virtual float PdfInner(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const = 0;

		public:
			virtual int GetParameterCount() const
			{
				return 2;
			}

			virtual String GetParameterName(const int idx) const
			{
				Assert(idx < 2);
				switch (idx)
				{
				case 0:
				{
					if (!mpTexture->IsConstant())
						return "TextureMap";
					else
						return "Color";
				}
				case 1:
					return "NormalMap";
				default:
					return "";
				}
			}

			virtual Parameter GetParameter(const String& name) const
			{
				Parameter ret;

				if (!mpTexture->IsConstant() && name == "TextureMap")
				{
					ret.Type = Parameter::TextureMap;
					return ret;
				}
				else if (mpTexture->IsConstant() && name == "Color")
				{
					ret.Type = Parameter::Color;
					Color color = mpTexture->Sample(Vector2::ZERO, nullptr);
					ret.R = color.r;
					ret.G = color.g;
					ret.B = color.b;

					return ret;
				}
				else if (name == "NormalMap")
				{
					ret.Type = Parameter::NormalMap;
					return ret;
				}
				else
				{
					ret.Type = Parameter::None;
					return ret;
				}
			}

			virtual void SetParameter(const String& name, const Parameter& param)
			{
				if (name == "TextureMap")
				{
					mpTexture.Reset(new ImageTexture<Color, Color4b>(param.TexPath));
					return;
				}
				else if (name == "Color")
				{
					if (!mpTexture->IsConstant())
					{
						mpTexture.Reset(new ConstantTexture2D<Color>(Color(param.R, param.G, param.B)));
					}
					else
						mpTexture->SetValue(Color(param.R, param.G, param.B));
				}
				else if (name == "NormalMap")
				{
					mpNormalMap.Reset(new ImageTexture<Color, Color4b>(param.TexPath, 1.0f));
					return;
				}

				return;
			}

		public:
			static UniquePtr<BSDF> CreateBSDF(const BSDFType type, const Color& color);
			static UniquePtr<BSDF> CreateBSDF(const BSDFType type, BSDF* pBSDF);
			static UniquePtr<BSDF> CreateBSDF(const BSDFType type, const char* strTexPath);

			static float FresnelDielectric(float cosi, float etai, float etat);
			static float FresnelConductor(float cosi, const float& eta, const float k);

		protected:
			static float	GGX_D(const Vector3& wh, float alpha);
			static Vector3	GGX_SampleNormal(float u1, float u2, float* pPdf, float alpha);
			static float	SmithG(const Vector3& v, const Vector3& wh, float alpha);
			static float	GGX_G(const Vector3& wo, const Vector3& wi, const Vector3& wh, float alpha);
			static float	GGX_Pdf(const Vector3& wh, float alpha);

			static Vector2	ImportanceSampleGGX_VisibleNormal_Unit(float thetaI, float u1, float u2);
			static Vector3	GGX_SampleVisibleNormal(const Vector3& _wi, float u1, float u2, float* pPdf, float Roughness);
			static float	GGX_Pdf_VisibleNormal(const Vector3& wi, const Vector3& H, float Roughness);
		};

		class LambertianDiffuse : public BSDF
		{
		public:
			LambertianDiffuse(const Color& cColor = Color::WHITE)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE), BSDFType::Diffuse, cColor)
			{
			}
			LambertianDiffuse(UniquePtr<Texture2D<Color>> pTex, UniquePtr<Texture2D<Color>> pNormal)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE), BSDFType::Diffuse, Move(pTex), Move(pNormal))
			{
			}
			LambertianDiffuse(const char* pFile)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE), BSDFType::Diffuse, pFile)
			{
			}

			Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const;

		private:
			float PdfInner(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const override;
			float EvalInner(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const override;
		};

		class Mirror : public BSDF
		{
		public:
			Mirror(const Color& cColor = Color::WHITE)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_SPECULAR), BSDFType::Mirror, cColor)
			{
			}
			Mirror(UniquePtr<Texture2D<Color>> pTex, UniquePtr<Texture2D<Color>> pNormal)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_SPECULAR), BSDFType::Mirror, Move(pTex), Move(pNormal))
			{
			}
			Mirror(const char* strTexPath)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_SPECULAR), BSDFType::Mirror, strTexPath)
			{
			}

			Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const;
			float Pdf(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const;
			Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const;

		private:
			float EvalInner(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const override;
			float PdfInner(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const override;
		};

		class Glass : public BSDF
		{
		private:
			float mEtai, mEtat;

		public:
			Glass(const Color& cColor = Color::WHITE, float etai = 1.0f, float etat = 1.5f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_SPECULAR), BSDFType::Glass, cColor)
				, mEtai(etai)
				, mEtat(etat)
			{
			}
			Glass(UniquePtr<Texture2D<Color>> pTex, UniquePtr<Texture2D<Color>> pNormal, float etai = 1.0f, float etat = 1.5f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_SPECULAR), BSDFType::Glass, Move(pTex), Move(pNormal))
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

			Color Eval(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const;
			float Pdf(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const;
			Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const;

		public:
			int GetParameterCount() const
			{
				return BSDF::GetParameterCount() + 1;
			}

			String GetParameterName(const int idx) const
			{
				if (idx < BSDF::GetParameterCount())
					return BSDF::GetParameterName(idx);

				if (idx == GetParameterCount() - 1)
					return "IOR";

				return "";
			}

			Parameter GetParameter(const String& name) const
			{
				Parameter ret = BSDF::GetParameter(name);
				if (ret.Type != Parameter::None)
					return ret;

				if (name == "IOR")
				{
					ret.Type = Parameter::Float;
					ret.Value = this->mEtat;
					ret.Min = 1.0f + 1e-4f;
					ret.Max = 1.8f;

					return ret;
				}

				return ret;
			}
			
			void SetParameter(const String& name, const Parameter& param)
			{
				BSDF::SetParameter(name, param);

				if (name == "IOR")
					this->mEtat = param.Value;

				return;
			}

		private:
			float EvalInner(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const override;
			float PdfInner(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const override;
		};

		namespace BSDFCoordinate
		{
			inline float CosTheta(const Vector3& vec) { return vec.z; }
			inline float CosTheta2(const Vector3& vec) { return vec.z * vec.z; }
			inline float AbsCosTheta(const Vector3& vec) { return Math::Abs(vec.z); }
			inline float SinTheta2(const Vector3& vec) { return Math::Max(0.0f, 1.0f - CosTheta(vec) * CosTheta(vec)); }
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
			inline float TanTheta(const Vector3& vec)
			{
				float temp = 1 - vec.z * vec.z;
				if (temp <= 0.0f)
					return 0.0f;
				return Math::Sqrt(temp) / vec.z;
			}
			inline float TanTheta2(const Vector3& vec)
			{
				float temp = 1 - vec.z * vec.z;
				if (temp <= 0.0f)
					return 0.0f;
				return temp / (vec.z * vec.z);
			}

			inline bool SameHemisphere(const Vector3& vec1, const Vector3& vec2) { return vec1.z * vec2.z > 0.0f; }
		};
	}
}