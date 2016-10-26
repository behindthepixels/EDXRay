#pragma once

#include "Graphics/Camera.h"
#include "../ForwardDecl.h"

#include "Core/SmartPointer.h"

namespace EDX
{
	namespace RayTracer
	{
		struct CameraParameters
		{
			Vector3	Pos;
			Vector3	Target;
			Vector3	Up;
			float	NearClip, FarClip;
			float	FocusPlaneDist;

			static const int FullFrameSensorSize = 24; // full frame sensor 36x24mm
			int FocalLengthMilliMeters;
			float FStop;
			float Vignette;

			float CalcFieldOfView() const;
			float CalcCircleOfConfusionRadius() const; // In millimeters
		};

		class Camera : public EDX::Camera
		{
		public:
			float mCoCRadius, mFocalPlaneDist;
			float mImagePlaneDist;
			float mVignetteFactor;

			// Differential
			Vector3 mDxCam;
			Vector3 mDyCam;

		private:
			UniquePtr<Sampling::Distribution2D>	mpApertureDistribution;

		public:
			Camera();

			void Init(const Vector3& pos,
				const Vector3& tar,
				const Vector3& up,
				const int resX,
				const int resY,
				const float FOV = 35.0f,
				const float nearClip = 1.0f,
				const float farClip = 1000.0f,
				const float blurRadius = 0.0f,
				const float focalDist = 0.0f,
				const float vignette = 3.0f);

			void Resize(int width, int height);
			bool GenerateRay(const CameraSample& sample, Ray* pRay, const bool forcePinHole = false) const;
			bool GenRayDifferential(const CameraSample& sample, RayDifferential* pRay) const;

			void SetApertureFunc(const char* path);

			float GetCircleOfConfusionRadius() const
			{
				return mCoCRadius;
			}
			float GetFocusDistance() const
			{
				return mFocalPlaneDist;
			}
			float GetImagePlaneDistance() const
			{
				return mImagePlaneDist;
			}
			const CameraParameters GetCameraParams() const
			{
				CameraParameters ret;
				ret.Pos = mPos;
				ret.Target = mTarget;
				ret.Up = mUp;
				ret.NearClip = mNearClip;
				ret.FarClip = mFarClip;
				ret.FocusPlaneDist = mFocalPlaneDist;
				ret.Vignette = 3.0f - mVignetteFactor;

				return ret;
			}
		};
	}
}