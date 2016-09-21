#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/Primitive.h"
#include "../Core/TriangleMesh.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "../Tracer/BVH.h"
#include "Graphics/Color.h"
#include "Math/EDXMath.h"

namespace EDX
{
	namespace RayTracer
	{
		class AreaLight : public Light
		{
		private:
			Primitive*			mpPrim;
			Color				mIntensity;
			uint				mTriangleCount;
			float				mArea, mInvArea;
			UniquePtr<BVH2>	mLightBVH;

		public:
			AreaLight(
				Primitive* pPrim,
				const Color& intens,
				const uint sampCount = 1)
				: Light(sampCount)
				, mpPrim(pPrim)
				, mIntensity(intens)
			{
				mTriangleCount = mpPrim->GetMesh()->GetTriangleCount();

				for (auto i = 0; i < mTriangleCount; i++)
				{
					float area = TriangleArea(i);
					mArea += area;
				}

				mInvArea = 1.0f / mArea;

				mpPrim->SetAreaLight(this);

				Array<Primitive*> primitives;
				primitives.Add(mpPrim);
				mLightBVH = MakeUnique<BVH2>();
				mLightBVH->Construct(primitives);
			}

			Color Illuminate(const Scatter& scatter,
				const RayTracer::Sample& lightSample,
				Vector3* pDir,
				VisibilityTester* pVisTest,
				float* pPdf,
				float* pCosAtLight = nullptr,
				float* pEmitPdfW = nullptr) const override
			{
				const Vector3& pos = scatter.mPosition;
				uint triId = lightSample.w * mTriangleCount;
				triId = Math::Clamp(triId, 0, mTriangleCount - 1);

				float b0, b1;
				Sampling::UniformSampleTriangle(lightSample.u, lightSample.v, &b0, &b1);

				Vector3 lightPoint, lightNormal;
				SampleTriangle(triId, b0, b1, lightPoint, lightNormal);

				const Vector3 lightVec = lightPoint - pos;
				*pDir = Math::Normalize(lightVec);
				pVisTest->SetSegment(pos, lightPoint);
				pVisTest->SetMedium(scatter.mMediumInterface.GetMedium(*pDir, scatter.mNormal));

				const float dist = Math::Length(lightVec);
				*pPdf = (dist * dist) /
					Math::AbsDot(lightNormal, -*pDir) * mInvArea;

				const float cosNormalDir = Math::Dot(lightNormal, -*pDir);
				if (cosNormalDir < 1e-6f)
					return Color::BLACK;

				if (pCosAtLight != NULL)
				{
					*pCosAtLight = cosNormalDir;
				}
				if (pEmitPdfW != NULL)
				{
					*pEmitPdfW = mInvArea * Sampling::CosineHemispherePDF(cosNormalDir);
				}

				return mIntensity;
			}

			Color Sample(const RayTracer::Sample& lightSample1,
				const RayTracer::Sample& lightSample2,
				Ray* pRay,
				Vector3* pNormal,
				float* pPdf,
				float* pDirectPdf = nullptr) const override
			{
				uint triId = lightSample1.w * mTriangleCount;
				triId = Math::Clamp(triId, 0, mTriangleCount - 1);

				float b0, b1;
				Sampling::UniformSampleTriangle(lightSample1.u, lightSample1.v, &b0, &b1);

				Vector3 lightPoint, lightNormal;
				SampleTriangle(triId, b0, b1, lightPoint, lightNormal);

				Vector3 localDirOut = Sampling::CosineSampleHemisphere(lightSample2.u, lightSample2.v);
				localDirOut.z = Math::Max(localDirOut.z, 1e-6f);

				Frame lightFrame = Frame(lightNormal);
				Vector3 worldDirOut = lightFrame.LocalToWorld(localDirOut);

				*pRay = Ray(lightPoint, worldDirOut);
				*pPdf = mInvArea * Sampling::CosineHemispherePDF(BSDFCoordinate::CosTheta(localDirOut)); // Pdf of position times direction

				if (pDirectPdf)
					*pDirectPdf = mInvArea;

				return mIntensity * BSDFCoordinate::CosTheta(localDirOut);
			}

			Color Emit(const Vector3& dir,
				const Vector3& normal = Vector3::ZERO,
				float* pPdf = nullptr,
				float* pDirectPdf = nullptr) const override
			{
				if (Math::Dot(normal, dir) <= 0.0f)
					return Color::BLACK;


				if (pDirectPdf)
					*pDirectPdf = mInvArea;

				if (pPdf)
				{
					*pPdf = Sampling::CosineHemispherePDF(normal, dir);
					*pPdf *= mInvArea;
				}

				return mIntensity;
			}

			float Pdf(const Vector3& pos, const Vector3& dir) const override
			{
				DifferentialGeom isect;
				Ray ray = Ray(pos, dir);
				if (mLightBVH->Intersect(ray, &isect))
				{
					mpPrim->PostIntersect(ray, &isect);

					const float dist = isect.mDist;

					return (dist * dist) /
						Math::AbsDot(isect.mGeomNormal, -dir) * mInvArea;
				}
				else
					return 0.0f;
			}

			bool IsAreaLight() const override
			{
				return true;
			}

			bool IsDelta() const override
			{
				return false;
			}

			bool IsFinite() const override
			{
				return true;
			}

			Primitive* GetPrimitive()
			{
				return mpPrim;
			}

		private:
			inline void SampleTriangle(const uint triId,
				const float b0,
				const float b1,
				Vector3& outPos,
				Vector3& outNormal) const
			{
				auto pMesh = mpPrim->GetMesh();
				const Vector3& p0 = pMesh->GetPositionAt(3 * triId + 0);
				const Vector3& p1 = pMesh->GetPositionAt(3 * triId + 1);
				const Vector3& p2 = pMesh->GetPositionAt(3 * triId + 2);
				outPos = b0 * p0 + b1 * p1 + (1.0f - b0 - b1) * p2;

				const Vector3& n0 = pMesh->GetNormalAt(3 * triId + 0);
				const Vector3& n1 = pMesh->GetNormalAt(3 * triId + 1);
				const Vector3& n2 = pMesh->GetNormalAt(3 * triId + 2);
				outNormal = b0 * n0 + b1 * n1 + (1.0f - b0 - b1) * n2;
			}

			inline float TriangleArea(const uint triId)
			{
				auto pMesh = mpPrim->GetMesh();
				const Vector3& p0 = pMesh->GetPositionAt(3 * triId + 0);
				const Vector3& p1 = pMesh->GetPositionAt(3 * triId + 1);
				const Vector3& p2 = pMesh->GetPositionAt(3 * triId + 2);

				return 0.5f * Math::Length(Math::Cross(p1 - p0, p2 - p0));
			}
		};

	}
}