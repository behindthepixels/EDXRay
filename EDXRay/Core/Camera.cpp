#include "Camera.h"

namespace EDX
{
	namespace RayTracer
	{
		void Camera::Init(const Vector3& pos,
			const Vector3& tar,
			const Vector3& up,
			const int resX,
			const int resY,
			const float FOV,
			const float nearClip,
			const float farClip,
			const float lensR,
			const float focalDist)
		{
			EDX::Camera::Init(pos, tar, up, resX, resY, FOV, nearClip, farClip);

			mLensRadius = lensR;
			mFocalPlaneDist = focalDist;
		}

		void Camera::GenerateRay(const CameraSample& sample, Ray* pRay) const
		{
			*pRay = Ray();
		}
	}
}