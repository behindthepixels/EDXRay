#include "DifferentialGeom.h"
#include "Math/Ray.h"

namespace EDX
{
	namespace RayTracer
	{
		void DifferentialGeom::ComputeDifferentials(const RayDifferential& ray) const
		{
			if (ray.mHasDifferential && mTextured)
			{
				float fD = -Math::Dot(mNormal, mPosition);

				float fTx = -(Math::Dot(mNormal, ray.mDxOrg) + fD) / Math::Dot(mNormal, ray.mDxDir);
				if (_isnan(fTx))
					goto Fail;

				float fTy = -(Math::Dot(mNormal, ray.mDyOrg) + fD) / Math::Dot(mNormal, ray.mDyDir);
				if (_isnan(fTy))
					goto Fail;

				Vector3 vPx = ray.mDxOrg + fTx * ray.mDxDir;
				Vector3 vPy = ray.mDyOrg + fTy * ray.mDyDir;

				mDpdx = vPx - mPosition;
				mDpdy = vPy - mPosition;

				auto SolveLinearSystem2x2 = [](const float A[2][2], const float B[2], float *x0, float *x1) -> bool
				{
					float fDet = A[0][0] * A[1][1] - A[0][1] * A[1][0];
					if (Math::Abs(fDet) < 1e-10f)
						return false;
					float fInvDet = 1.0f / fDet;
					*x0 = (A[1][1] * B[0] - A[0][1] * B[1]) * fInvDet;
					*x1 = (A[0][0] * B[1] - A[1][0] * B[0]) * fInvDet;
					if (_isnan(*x0) || _isnan(*x1))
						return false;
					return true;
				};

				float A[2][2], Bx[2], By[2];
				int axes[2];
				if (Math::Abs(mNormal.x) > Math::Abs(mNormal.y) && Math::Abs(mNormal.x) > Math::Abs(mNormal.z))
				{
					axes[0] = 1;
					axes[1] = 2;
				}
				else if (Math::Abs(mNormal.y) > Math::Abs(mNormal.z))
				{
					axes[0] = 0;
					axes[1] = 2;
				}
				else
				{
					axes[0] = 0;
					axes[1] = 1;
				}

				// Initialize matrices for chosen projection plane
				A[0][0] = mDpdu[axes[0]];
				A[0][1] = mDpdv[axes[0]];
				A[1][0] = mDpdu[axes[1]];
				A[1][1] = mDpdv[axes[1]];
				Bx[0] = vPx[axes[0]] - mPosition[axes[0]];
				Bx[1] = vPx[axes[1]] - mPosition[axes[1]];
				By[0] = vPy[axes[0]] - mPosition[axes[0]];
				By[1] = vPy[axes[1]] - mPosition[axes[1]];
				if (!SolveLinearSystem2x2(A, Bx, &mDudx, &mDvdx))
				{
					mDudx = 0.0f;
					mDvdx = 0.0f;
				}
				if (!SolveLinearSystem2x2(A, By, &mDudy, &mDvdy))
				{
					mDudy = 0.0f;
					mDvdy = 0.0f;
				}
			}
			else
			{
			Fail:
				mDpdx = mDpdy = Vector3::ZERO;
				mDudx = mDudy = mDvdx = mDvdy = 0.0f;
			}
		}
	}
}