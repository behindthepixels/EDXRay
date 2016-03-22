#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"

#include "../Core/Medium.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		class HomogeneousMedium : public Medium
		{
		private:
			Color mSigmaS, mSigmaA, mSigmaT;

		public:
			HomogeneousMedium(const Color& sigmaS, const Color& sigmaA, const float phaseG)
				: Medium(phaseG)
				, mSigmaS(sigmaS)
				, mSigmaA(sigmaA)
			{
				mSigmaT = mSigmaS + mSigmaA;
			}

			Color Transmittance(const Ray& ray, Sampler*pSampler) const override;
			Color Sample(const Ray& ray, Sampler* pSampler, MediumScatter* pMS) const override;
		};
	}
}