#pragma once

#include "EDXPrerequisites.h"
#include "Math/Vector.h"
#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		class Frame
		{
		public:
			Vector3 mX, mY, mZ;

		public:
			Frame()
			{
				mX = Vector3::UNIT_X;
				mY = Vector3::UNIT_Y;
				mZ = Vector3::UNIT_Z;
			};

			Frame(const Vector3& x,
				const Vector3& y,
				const Vector3& z
				)
				: mX(x)
				, mY(y)
				, mZ(z)
			{}

			Frame(const Vector3& vNormal)
			{
				mZ = Math::Normalize(vNormal);
				Math::CoordinateSystem(mZ, &mX, &mY);
			}

			Vector3 LocalToWorld(const Vector3& vVec) const
			{
				return mX * vVec.x + mY * vVec.y + mZ * vVec.z;
			}

			Vector3 WorldToLocal(const Vector3& vVec) const
			{
				return Vector3(Math::Dot(vVec, mX), Math::Dot(vVec, mY), Math::Dot(vVec, mZ));
			}

			const Vector3& Binormal() const { return mX; }
			const Vector3& Tangent() const { return mY; }
			const Vector3& Normal() const { return mZ; }

		};

		class Intersection
		{
		public:
			int		mPrimId, mTriId;
			float	mU, mV;
			float	mDist;

			Intersection()
				: mDist(float(Math::EDX_INFINITY))
			{
			}
		};

		class DifferentialGeom : public Intersection
		{
		public:
			Vector3 mPosition, mNormal;
			Vector2 mTexcoord;
			Vector3 mGeomNormal;
			Vector3 mDpdu, mDpdv;

			Frame mShadingFrame, mGeomFrame;

			BSDF* mpBSDF;

			inline Vector3 WorldToLocal(const Vector3& vVec) const
			{
				return mShadingFrame.WorldToLocal(vVec);
			}
			inline Vector3 LocalToWorld(const Vector3& vVec) const
			{
				return mShadingFrame.LocalToWorld(vVec);
			}
		};

	}
}