#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"
#include "BSDF.h"
#include "Sampling.h"
#include "Math/Vector.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		class BSSRDFAdapter : public BSDF
		{
		private:
			const BSSRDF* mpBSSRDF;

		public:
			BSSRDFAdapter(const BSSRDF* pBSSRDF)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_DIFFUSE), BSDFType::Diffuse, Color::WHITE)
				, mpBSSRDF(pBSSRDF)
			{
			}

			Color SampleScattered(const Vector3& vOut, const Sample& sample, const DifferentialGeom& diffGeom, Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL, ScatterType* pSampledTypes = NULL) const;

		private:
			float PdfInner(const Vector3& vIn, const Vector3& vOut, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const override;
			float EvalInner(const Vector3& vOut, const Vector3& vIn, const DifferentialGeom& diffGeom, ScatterType types = BSDF_ALL) const override;
		};


		class BSSRDF
		{
		private:
			static const int LUTSize = 1024;
			static float ScatteredDistLUT[LUTSize];

			Color mDiffuseReflectance;
			Vector3 mMeanFreePathLength;
			Vector3 mD;
			RefPtr<BSSRDFAdapter> mAdapter;
			float mEtai = 1.0f;
			float mEtat = 1.5f;

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

				mAdapter = new BSSRDFAdapter(this);
			}

			Color SampleSubsurfaceScattered(
				const Vector3&			wo,
				const Sample&			sample,
				const DifferentialGeom&	diffGeom,
				const Scene*			pScene,
				DifferentialGeom*		pSampledDiffGeom,
				float*					pPdf,
				MemoryArena&			memory) const;

			float EvalWi(const Vector3& wi) const;

		private:
			inline Color NormalizeDiffusion(const float r) const
			{
				if (r == 0.0f)
					return 0.0f;
				else
					return Color(Math::Exp(Vector3(-r) / mD) + Math::Exp(Vector3(-r) / (3.0f * mD))) /
						Color(8.0f * float(Math::EDX_PI) * mD * r);
			}
			inline float NormalizeDiffusion(const float r, const float d, const float A) const
			{
				if (r == 0.0f)
					return 0.0f;
				else
					return (Math::Exp(-r / d) + Math::Exp(-r / (3.0f * d))) /
						(8.0f * float(Math::EDX_PI) * d * r);
			}

			float SampleRadius(const float u, const float d, const float A, float* pPdf = nullptr) const;
			float Pdf_Radius(const float radius, const float d, const float A) const;
			float Pdf_Sample(const float radius, const DifferentialGeom& diffGeomOut, const DifferentialGeom& diffGeomOutIn) const;
		};
	}
}