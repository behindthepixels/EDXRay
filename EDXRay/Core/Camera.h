#pragma once

#include "Graphics/Camera.h"
#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		class Camera : public EDX::Camera
		{
		private:
			float mLensRadius, mFocalPlaneDist;
			float mImagePlaneDist;

		public:
			void Init(const Vector3& pos,
				const Vector3& tar,
				const Vector3& up,
				const int resX,
				const int resY,
				const float FOV = 35.0f,
				const float nearClip = 1.0f,
				const float farClip = 1000.0f,
				const float lensR = 0.0f,
				const float focalDist = 0.0f);
			void GenerateRay(const CameraSample& sample, Ray* pRay) const;
		};
	}
}