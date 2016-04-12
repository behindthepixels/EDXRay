#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"
#include "Sampling.h"
#include "Math/Vector.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		class BSSRDF
		{
		private:
			static const int LUTSize = 1024;
			static float ScatteredDistLUT[LUTSize];

			Color mDiffuseReflectance;
			Vector3 mMeanFreePathLength;
			Vector3 mD;

		public:
			BSSRDF(const Color& diffReflectance, const Vector3& meanFreePath)
				: mDiffuseReflectance(diffReflectance)
				, mMeanFreePathLength(meanFreePath)
			{
				auto ComputeScaling = [](const float A) -> float
				{
					const float tempTerm = Math::Abs(A - 0.8f);
					const float tempTerm2 = tempTerm * tempTerm * tempTerm;
					return 1.85f - A + 7 * tempTerm2;
				};

				Vector3 scaling;
				for (auto ch = 0; ch < 3; ch++)
					scaling[ch] = ComputeScaling(mDiffuseReflectance[ch]);

				mD = mMeanFreePathLength / scaling;
			}

			Color SampleSubsurfaceScattered(
				const Sample&			sample,
				const DifferentialGeom&	diffGeom,
				const Scene*			pScene,
				DifferentialGeom*		pSampledDiffGeom,
				float*					pPdf) const;

		private:
			inline float NormalizeDiffusion(const float r, const float d, const float A) const
			{
				return A * (Math::Exp(-r / d) + Math::Exp(-r / (3.0f * d))) /
					(8.0f * float(Math::EDX_PI) * d * r + 1e-6f);
			}

			float SampleRadius(const float u, const float d, const float A, float* pPdf = nullptr) const;
			float Pdf_Radius(const float radius, const float d, const float A) const;
			float Pdf_Sample(const float radius, const DifferentialGeom& diffGeomOut, const DifferentialGeom& diffGeomOutIn) const;
		};
	}
}