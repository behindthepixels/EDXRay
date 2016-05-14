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

			const BSDF* mpBSDF;
			Vector3 mMeanFreePathLength;
			RefPtr<BSSRDFAdapter> mAdapter;
			float mEtai = 1.0f;
			float mEtat = 1.5f;

		public:
			BSSRDF(const BSDF* pBSDF, const Vector3& meanFreePath)
				: mpBSDF(pBSDF)
				, mMeanFreePathLength(meanFreePath)
			{
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

			Vector3 GetMeanFreePath() const
			{
				return mMeanFreePathLength;
			}

			void SetMeanFreePath(const Vector3& inMFP)
			{
				mMeanFreePathLength = inMFP;
			}

		private:
			inline Vector3 NormalizeDiffusion(const float r, const Vector3& D) const
			{
				if (r == 0.0f)
					return 0.0f;
				
				return (Math::Exp(Vector3(-r) / D) + Math::Exp(Vector3(-r) / (3.0f * D))) /
						(8.0f * float(Math::EDX_PI) * D * r);
			}
			inline float NormalizeDiffusion(const float r, const float d) const
			{
				if (r == 0.0f)
					return 0.0f;
				
				return (Math::Exp(-r / d) + Math::Exp(-r / (3.0f * d))) /
						(8.0f * float(Math::EDX_PI) * d * r);
			}

			float SampleRadius(const float u, const float d, float* pPdf = nullptr) const;
			float Pdf_Radius(const float radius, const float d) const;
			float Pdf_Sample(const float radius, const Vector3& D, const DifferentialGeom& diffGeomOut, const DifferentialGeom& diffGeomIn) const;
		};
	}
}