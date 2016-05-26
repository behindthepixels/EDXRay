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
			Vector3 mSigmaS, mSigmaA, mSigmaT;
			Vector3 mMeanFreePathLength;
			Color mDiffuseReflectance;

		public:
			HomogeneousMedium(const Vector3& sigmaS, const Vector3& sigmaA, const float phaseG)
				: Medium(phaseG)
				, mSigmaS(sigmaS)
				, mSigmaA(sigmaA)
			{
				mSigmaT = mSigmaS + mSigmaA;
			}

			HomogeneousMedium(const Color& diffuseReflectance, const Vector3& meanFreePath, const float eta)
				: Medium(0.0f)
			{
				mDiffuseReflectance = diffuseReflectance;
				mMeanFreePathLength = meanFreePath;
				Reparameterizer::Eval(mDiffuseReflectance, mMeanFreePathLength, eta, &mSigmaS, &mSigmaA);
				mSigmaT = mSigmaS + mSigmaA;
			}


			Color Transmittance(const Ray& ray, Sampler*pSampler) const override;
			Color Sample(const Ray& ray, Sampler* pSampler, MediumScatter* pMS) const override;

			Color GetDiffuseReflectance() const
			{
				return mDiffuseReflectance;
			}

			Vector3 GetMeanFreePath() const
			{
				return mMeanFreePathLength;
			}

			void SetDiffReflectanceAndMeanFreePath(const Color& diffuseReflectance, const Vector3& meanFreePath, const float eta, const float scale = 1.0f)
			{
				mDiffuseReflectance = diffuseReflectance;
				mMeanFreePathLength = meanFreePath;
				Reparameterizer::Eval(mDiffuseReflectance, mMeanFreePathLength * scale, eta, &mSigmaS, &mSigmaA);
				mSigmaT = mSigmaS + mSigmaA;
			}
		};
	}
}