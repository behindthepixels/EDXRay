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
			void GenerateRay(const CameraSample& sample, Ray* pRay) const;
		};
	}
}