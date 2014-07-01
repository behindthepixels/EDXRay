#include "Camera.h"

namespace EDX
{
	namespace RayTracer
	{
		void Camera::GenerateRay(const CameraSample& sample, Ray* pRay) const
		{
			*pRay = Ray();
		}
	}
}