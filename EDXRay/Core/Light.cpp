#include "Light.h"
#include "Scene.h"

namespace EDX
{
	namespace RayTracer
	{
		bool VisibilityTester::Unoccluded(const Scene* pScene) const
		{
			return !pScene->Occluded(mRay);
		}
	}
}