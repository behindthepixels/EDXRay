#include "Camera.h"
#include "Sampler.h"
#include "Sampling.h"
#include "../Core/Ray.h"
#include "Windows/Bitmap.h"

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

		Camera::Camera()
		{
			const int ApertureSize = 128;
			Array2f apetureFunc(ApertureSize, ApertureSize);

			for (int i = 0; i < ApertureSize; i++)
			{
				float y = 2.0f * i / float(ApertureSize) - 1.0f;

				for (int j = 0; j < ApertureSize; j++)
				{
					float x = 2.0f * j / float(ApertureSize) - 1.0f;

					apetureFunc[Vector2i(i, j)] = Math::Length(Vector2(x, y)) < 1.0f ? 1.0f : 0.0f;
				}
			}

			mpApertureDistribution = MakeUnique<Sampling::Distribution2D>(apetureFunc.Data(), ApertureSize, ApertureSize);
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
			const float focalDist,
			const float vignette)
		{
			EDX::Camera::Init(pos, tar, up, resX, resY, FOV, nearClip, farClip);

			mCoCRadius = blurRadius;
			mFocalPlaneDist = focalDist;
			mVignetteFactor = 3.0f - vignette;

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

		bool Camera::GenerateRay(const CameraSample& sample, Ray* pRay, const bool forcePinHole) const
		{
			Vector3 camCoord = Matrix::TransformPoint(Vector3(sample.imageX, sample.imageY, 0.0f), mRasterToCamera);

			pRay->mOrg = Vector3::ZERO;
			pRay->mDir = Math::Normalize(camCoord);

			if (mCoCRadius > 0.0f && !forcePinHole)
			{
				float fFocalHit = mFocalPlaneDist / pRay->mDir.z;
				Vector3 ptFocal = pRay->CalcPoint(fFocalHit);

				Vector2 screenCoord = 2.0f * Vector2(sample.imageX, sample.imageY) / Vector2(mFilmResX, mFilmResY) - Vector2::UNIT_SCALE;
				screenCoord.x *= mRatio;
				screenCoord.y *= -1.0f;

				float fU, fV, pdf;
				mpApertureDistribution->SampleContinuous(sample.lensU, sample.lensV, &fU, &fV, &pdf);
				fU = 2.0f * fU - 1.0f;
				fV = 2.0f * fV - 1.0f;

				if (Math::Length(Vector2(screenCoord.x + fU, screenCoord.y + fV)) > mVignetteFactor)
					return false;

				fU *= mCoCRadius;
				fV *= mCoCRadius;

				pRay->mOrg = Vector3(fU, fV, 0.0f);
				pRay->mDir = Math::Normalize(ptFocal - pRay->mOrg);
			}

			*pRay = TransformRay(*pRay, mViewInv);
			pRay->mMin = float(Math::EDX_EPSILON);
			pRay->mMax = float(Math::EDX_INFINITY);

			return true;
		}

		bool Camera::GenRayDifferential(const CameraSample& sample, RayDifferential* pRay) const
		{
			//Vector2 ndcCoord = Vector2(
			//	2.0f * sample.imageX / float(mFilmResX) - 1.0f,
			//	2.0f * sample.imageY / float(mFilmResY) - 1.0f
			//);

			//ndcCoord.x *= mRatio;

			//float r = Math::Length(ndcCoord);
			//float theta = Math::ToRadians(r * mFOV);
			//float sinTheta = Math::Sin(theta);
			//float cosTheta = Math::Cos(theta);
			//float phi = Math::Atan2(-ndcCoord.y, ndcCoord.x);

			//Vector3 dir = Math::SphericalDirection(sinTheta, cosTheta, phi, -Math::Cross(mDir, mUp), mDir, mUp);

			Vector3 camCoord = Matrix::TransformPoint(Vector3(sample.imageX, sample.imageY, 0.0f), mRasterToCamera);

			pRay->mOrg = Vector3::ZERO;
			pRay->mDir = Math::Normalize(camCoord);

			if (mCoCRadius > 0.0f)
			{
				float fFocalHit = mFocalPlaneDist / pRay->mDir.z;
				Vector3 ptFocal = pRay->CalcPoint(fFocalHit);

				Vector2 screenCoord = 2.0f * Vector2(sample.imageX, sample.imageY) / Vector2(mFilmResX, mFilmResY) - Vector2::UNIT_SCALE;
				screenCoord.x *= mRatio;
				screenCoord.y *= -1.0f;

				float fU, fV, pdf;
				mpApertureDistribution->SampleContinuous(sample.lensU, sample.lensV, &fU, &fV, &pdf);
				fU = 2.0f * fU - 1.0f;
				fV = 2.0f * fV - 1.0f;

				if (Math::Length(Vector2(screenCoord.x + fU, screenCoord.y + fV)) > mVignetteFactor)
					return false;

				fU *= mCoCRadius;
				fV *= mCoCRadius;

				pRay->mOrg = Vector3(fU, fV, 0.0f);
				pRay->mDir = Math::Normalize(ptFocal - pRay->mOrg);
			}

			pRay->mDxOrg = pRay->mDyOrg = pRay->mOrg;
			pRay->mDxDir = Math::Normalize(camCoord + mDxCam);
			pRay->mDyDir = Math::Normalize(camCoord + mDyCam);
			pRay->mHasDifferential = true;

			*pRay = TransformRayDiff(*pRay, mViewInv);
			pRay->mMin = float(Math::EDX_EPSILON);
			pRay->mMax = float(Math::EDX_INFINITY);

			return true;
		}

		void Camera::SetApertureFunc(const char* path)
		{
			int ApertureWidth, AperturaHeight, Channel;
			float* pFunc = Bitmap::ReadFromFile<float>(path, &ApertureWidth, &AperturaHeight, &Channel);

			mpApertureDistribution = MakeUnique<Sampling::Distribution2D>(pFunc, ApertureWidth, AperturaHeight);
			Memory::SafeDeleteArray(pFunc);
		}
	}
}