#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Light.h"
#include "../Core/Scene.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/Sampler.h"
#include "../Core/Sampling.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		class DirectionalLight : public Light
		{
		private:
			Color	mIntensity;
			Vector3 mDirection;
			Frame	mDirFrame;
			float	mConeCosMax;
			const Scene* mpScene;

		public:
			DirectionalLight(const Vector3& dir,
				const Color& intens,
				const Scene* pScene,
				const float coneDegrees = 0.0f,
				const uint sampCount = 1)
				: Light(sampCount)
				, mDirection(Math::Normalize(dir))
				, mDirFrame(mDirection)
				, mIntensity(intens)
				, mpScene(pScene)
				, mConeCosMax(Math::Cos(Math::ToRadians(coneDegrees)))
			{
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
				*pDir = Sampling::UniformSampleCone(lightSample.u, lightSample.v,
					mConeCosMax,
					mDirFrame.mX,
					mDirFrame.mY,
					mDirFrame.mZ);

				*pPdf = Sampling::UniformConePDF(mConeCosMax);

				Vector3 center;
				float radius;
				mpScene->WorldBounds().BoundingSphere(&center, &radius);
				pVisTest->SetSegment(pos, pos + 2.0f * radius * *pDir);
				pVisTest->SetMedium(scatter.mMediumInterface.GetMedium(*pDir, scatter.mNormal));

				if (pCosAtLight)
					*pCosAtLight = Math::Dot(*pDir, mDirection);

				if (pEmitPdfW)
				{
					*pEmitPdfW = Sampling::ConcentricDiscPdf() / (radius * radius) * (*pPdf);
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
				float f1, f2;
				Sampling::ConcentricSampleDisk(lightSample1.u, lightSample2.v, &f1, &f2);

				Vector3 center;
				float radius;
				mpScene->WorldBounds().BoundingSphere(&center, &radius);

				Vector3 origin = center + radius * (f1 * mDirFrame.mX + f2 * mDirFrame.mY);
				Vector3 direction = -Sampling::UniformSampleCone(lightSample2.u, lightSample2.v,
					mConeCosMax,
					mDirFrame.mX,
					mDirFrame.mY,
					mDirFrame.mZ);
				*pRay = Ray(origin + radius * mDirection, direction);
				*pNormal = pRay->mDir;

				float dirPdf = Sampling::UniformConePDF(mConeCosMax);
				*pPdf = Sampling::ConcentricDiscPdf() / (radius * radius) * dirPdf;
				if (pDirectPdf)
					*pDirectPdf = dirPdf;

				return mIntensity;
			}

			Color Emit(const Vector3& dir,
				const Vector3& normal = Vector3::ZERO,
				float* pPdf = nullptr,
				float* pDirectPdf = nullptr) const override
			{
				if (Sampling::DirectionInCone(dir, -mDirection, mConeCosMax))
				{
					float dirPdf = Sampling::UniformConePDF(mConeCosMax);
					if (pPdf)
					{
						Vector3 center;
						float radius;
						mpScene->WorldBounds().BoundingSphere(&center, &radius);

						*pPdf = Sampling::ConcentricDiscPdf() / (radius * radius) * dirPdf;
					}

					if (pDirectPdf)
						*pDirectPdf = dirPdf;

					return mIntensity;
				}

				return Color::BLACK;
			}

			float Pdf(const Vector3& pos, const Vector3& dir) const override
			{
				if (Sampling::DirectionInCone(dir, -mDirection, mConeCosMax))
				{
					return Sampling::UniformConePDF(mConeCosMax);
				}
				else
					return 0.0f;
			}

			bool IsEnvironmentLight() const override
			{
				return true;
			}

			bool IsDelta() const override
			{
				return mConeCosMax == 1.0f;
			}

			bool IsFinite() const override
			{
				return false;
			}
		};

	}
}