#pragma once

#include "EDXPrerequisites.h"
#include "Math/Vector.h"
#include "Memory/RefPtr.h"
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

			const Medium* GetMedium(const Vector3& dir, const Vector3& normal) const
			{
				return Math::Dot(dir, normal) > 0.0f ? mpOutside : mpInside;
			}

			bool IsMediumTransition() const { return mpInside != mpOutside; }

		};
	}
}