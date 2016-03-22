#include "Light.h"
#include "Scene.h"
#include "Sampler.h"
#include "Medium.h"
#include "Graphics/Color.h"

namespace EDX
{
	namespace RayTracer
	{
		bool VisibilityTester::Unoccluded(const Scene* pScene) const
		{
			return !pScene->Occluded(mRay);
		}

		Color VisibilityTester::Transmittance(const Scene* pScene, Sampler* pSampler) const
		{
			if (mRay.mpMedium)
				return mRay.mpMedium->Transmittance(mRay, pSampler);
			else
				return Color::WHITE;
		}
	}
}