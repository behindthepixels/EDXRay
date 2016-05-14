#pragma once

#include "EDXPrerequisites.h"
#include "Math/Vector.h"
#include "Memory/RefPtr.h"
#include "Memory/Memory.h"
#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		class PhaseFunctionHG
		{
		private:
			const float mG;

		public:
			PhaseFunctionHG(float g)
				: mG(g)
			{
			}

			float Eval(const Vector3& wo, const Vector3& wi) const;
			float Sample(const Vector3& wo, Vector3* pvIn, const Vector2& sample) const;

			inline float HenyeyGreenstein(const float cosTheta, const float g) const
			{
				float denom = 1 + g * g + 2 * g * cosTheta;
				return float(Math::EDX_INV_4PI) * (1 - g * g) / (denom * Math::Sqrt(denom));
			}
		};

		class Medium
		{
		protected:
			RefPtr<PhaseFunctionHG> mpPhaseFunc;

		public:
			Medium(const float g)
			{
				mpPhaseFunc = new PhaseFunctionHG(g);
			}

			virtual Color Transmittance(const Ray& ray, Sampler*pSampler) const = 0;
			virtual Color Sample(const Ray& ray, Sampler* pSampler, MediumScatter* pMS) const = 0;
		};

		class MediumInterface
		{
		private:
			const Medium* mpInside;
			const Medium* mpOutside;

		public:
			MediumInterface()
				: mpInside(nullptr)
				, mpOutside(nullptr)
			{
			}

			// MediumInterface Public Methods
			MediumInterface(const Medium* pMedium)
				: mpInside(pMedium)
				, mpOutside(pMedium)
			{
			}

			MediumInterface(const Medium* pInside, const Medium* pOutside)
				: mpInside(pInside)
				, mpOutside(pOutside)
			{
			}

			const Medium* GetOutside() const
			{
				return mpOutside;
			}

			const Medium* GetInside() const
			{
				return mpInside;
			}

			void SetOutside(const Medium* pMedium)
			{
				SafeDelete(mpOutside);
				mpOutside = pMedium;
			}

			void SetInside(const Medium* pMedium)
			{
				SafeDelete(mpInside);
				mpInside = pMedium;
			}

			const Medium* GetMedium(const Vector3& dir, const Vector3& normal) const
			{
				return Math::Dot(dir, normal) > 0.0f ? mpOutside : mpInside;
			}

			bool IsMediumTransition() const { return mpInside != mpOutside; }

		};

		struct Reparameterizer
		{
		public:
			static void Eval(const Color& diffuseReflectance, const Vector3& diffuseMeanFreePath, const float eta, Vector3* pSigmaS, Vector3* pSigmaA);

		private:
			const static int LUTSize = 1000;
			static float ReducedAlbedoLUT[LUTSize];
			static Vector3 DiffuseReflectance(const Color& diffuseReflectance, const Vector3& alpha, const float A);
			static float InternalReflectParam(const float F_dr);
			static float Fresnel_dr(const float eta);
		};
	}
}