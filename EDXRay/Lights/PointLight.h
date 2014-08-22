#pragma once

#include "EDXPrerequisites.h"
#include "../Core/Light.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		class PointLight : public Light
		{
		private:
			Vector3	mPosition;
			Color	mIntensity;

		public:
			PointLight(const Vector3& pos,
				const Color& intens)
				: mPosition(pos)
				, mIntensity(intens)
			{
			}

			Color Illuminate(const Vector3& pos, const Sample& lightSample, Vector3* pDir, VisibilityTester* pVisTest, float* pPdf) const
			{
				*pDir = Math::Normalize(mPosition - pos);
				*pPdf = 1.0f;
				pVisTest->SetSegment(pos, mPosition);

				//if (pfCosAtLight != NULL)
				//{
				//	*pfCosAtLight = 1.0f;
				//}
				//if (pfEmitPDFW != NULL)
				//{
				//	*pfEmitPDFW = Sampling::UniformSpherePDF();
				//}

				return mIntensity / Math::DistanceSquared(mPosition, pos);
			}
		};

	}
}