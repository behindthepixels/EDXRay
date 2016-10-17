#include "Camera.h"
#include "Sampler.h"
#include "Sampling.h"
#include "../Core/Ray.h"

namespace EDX
{
	namespace RayTracer
	{
		// Convert focal length to FOV,
		// 
		// fov = 2 * atan(d/(2*f))
		// where,
		//   d = sensor dimension (full frame sensor 36x24mm)
		//   f = focal length
		float CameraParameters::CalcFieldOfView() const
		{
			return Math::ToDegrees(
				2.0f * Math::Atan2(0.5f * FullFrameSensorSize, FocalLengthMilliMeters)
			);
		}


		// Convert f-stop, focal length, and focal distance to
		// projected circle of confusion size at infinity in mm.
		float CameraParameters::CalcCircleOfConfusionRadius() const
		{
			if (FStop == 22.0f)
				return 0.0f;

			const float Infinity = 1e8f;
			float blurRadius = 0.5f * Math::Abs(FocalLengthMilliMeters /
				FStop * (1.0f - (FocusPlaneDist * (1000.0f * Infinity - FocalLengthMilliMeters)) / (Infinity * (1000.0f * FocusPlaneDist - FocalLengthMilliMeters))));

			float blurFOV = blurRadius / float(24.0f) * CalcFieldOfView();

			return FocusPlaneDist * Math::Tan(Math::ToRadians(blurFOV));
		}

		void Camera::Init(const Vector3& pos,
			const Vector3& tar,
			const Vector3& up,
			const int resX,
			const int resY,
			const float FOV,
			const float nearClip,
			const float farClip,
			const float blurRadius,
			const float focalDist)
		{
			EDX::Camera::Init(pos, tar, up, resX, resY, FOV, nearClip, farClip);

			mCoCRadius = blurRadius;
			mFocalPlaneDist = focalDist;

			float tanHalfAngle = Math::Tan(Math::ToRadians(mFOV * 0.5f));
			mImagePlaneDist = mFilmResY * 0.5f / tanHalfAngle;
		}

		void Camera::Resize(int width, int height)
		{
			EDX::Camera::Resize(width, height);

			mDxCam = Matrix::TransformPoint(Vector3::UNIT_X, mRasterToCamera) - Matrix::TransformPoint(Vector3::ZERO, mRasterToCamera);
			mDyCam = Matrix::TransformPoint(Vector3::UNIT_Y, mRasterToCamera) - Matrix::TransformPoint(Vector3::ZERO, mRasterToCamera);

			float tanHalfAngle = Math::Tan(Math::ToRadians(mFOV * 0.5f));
			mImagePlaneDist = mFilmResY * 0.5f / tanHalfAngle;
		}

		void Camera::GenerateRay(const CameraSample& sample, Ray* pRay, const bool forcePinHole) const
		{
			Vector3 camCoord = Matrix::TransformPoint(Vector3(sample.imageX, sample.imageY, 0.0f), mRasterToCamera);

			pRay->mOrg = Vector3::ZERO;
			pRay->mDir = Math::Normalize(camCoord);

			if (mCoCRadius > 0.0f && !forcePinHole)
			{
				float fFocalHit = mFocalPlaneDist / pRay->mDir.z;
				Vector3 ptFocal = pRay->CalcPoint(fFocalHit);

				float fU, fV;
				Sampling::ConcentricSampleDisk(sample.lensU, sample.lensV, &fU, &fV);
				fU *= mCoCRadius;
				fV *= mCoCRadius;

				pRay->mOrg = Vector3(fU, fV, 0.0f);
				pRay->mDir = Math::Normalize(ptFocal - pRay->mOrg);
			}

			*pRay = TransformRay(*pRay, mViewInv);
			pRay->mMin = float(Math::EDX_EPSILON);
			pRay->mMax = float(Math::EDX_INFINITY);
		}

		void Camera::GenRayDifferential(const CameraSample& sample, RayDifferential* pRay) const
		{
			Vector3 camCoord = Matrix::TransformPoint(Vector3(sample.imageX, sample.imageY, 0.0f), mRasterToCamera);

			pRay->mOrg = Vector3::ZERO;
			pRay->mDir = Math::Normalize(camCoord);

			if (mCoCRadius > 0.0f)
			{
				float fFocalHit = mFocalPlaneDist / pRay->mDir.z;
				Vector3 ptFocal = pRay->CalcPoint(fFocalHit);

				float u, v;
				Sampling::ConcentricSampleDisk(sample.lensU, sample.lensV, &u, &v);
				u *= mCoCRadius;
				v *= mCoCRadius;

				pRay->mOrg = Vector3(u, v, 0.0f);
				pRay->mDir = Math::Normalize(ptFocal - pRay->mOrg);
			}

			pRay->mDxOrg = pRay->mDyOrg = pRay->mOrg;
			pRay->mDxDir = Math::Normalize(camCoord + mDxCam);
			pRay->mDyDir = Math::Normalize(camCoord + mDyCam);
			pRay->mHasDifferential = true;

			*pRay = TransformRayDiff(*pRay, mViewInv);
			pRay->mMin = float(Math::EDX_EPSILON);
			pRay->mMax = float(Math::EDX_INFINITY);
		}
	}
}