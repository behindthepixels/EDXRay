#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Light.h"
#include "../Core/DifferentialGeom.h"
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

		public:
			DirectionalLight(const Vector3& dir,
				const Color& intens,
				const uint sampCount = 1)
				: Light(sampCount)
				, mDirection(Math::Normalize(dir))
				, mIntensity(intens)
			{
			}

			Color Illuminate(const Vector3& pos, const Sample& lightSample, Vector3* pDir, VisibilityTester* pVisTest, float* pPdf) const
			{
				*pDir = mDirection;
				*pPdf = 1.0f;
				pVisTest->SetRay(pos, mDirection);

				return mIntensity;
			}

			Color Emit(const Vector3& dir) const
			{
				return Color::BLACK;
			}

			float Pdf(const Vector3& pos, const Vector3& dir) const
			{
				return 0.0f;
			}

			bool IsDelta() const
			{
				return true;
			}
		};

	}
}