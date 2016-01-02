#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"

#include "BSDF.h"
#include "Math/Vector.h"
#include "Memory/RefPtr.h"

namespace EDX
{
	namespace RayTracer
	{
		class Primitive
		{
		private:
			RefPtr<TriangleMesh>	mpMesh;

			// Materials
			vector<RefPtr<BSDF>>	mpBSDFs;
			const AreaLight*		mpAreaLight;
			uint*					mpMaterialIndices;

		public:
			Primitive()
				: mpAreaLight(nullptr)
				, mpMaterialIndices(nullptr)
			{
			}
			~Primitive();

			void LoadMesh(const char* path,
				const Vector3& pos = Vector3::ZERO,
				const Vector3& scl = Vector3::UNIT_SCALE,
				const Vector3& rot = Vector3::ZERO,
				const bool forceComputeNormal = false);
			void LoadMesh(const char* path,
				const BSDFType bsdfType = BSDFType::Diffuse,
				const Color& reflectance = Color(0.8f, 0.8f, 0.8f),
				const Vector3& pos = Vector3::ZERO,
				const Vector3& scl = Vector3::UNIT_SCALE,
				const Vector3& rot = Vector3::ZERO,
				const bool forceComputeNormal = false);
			void LoadSphere(const float radius,
				const BSDFType bsdfType = BSDFType::Diffuse,
				const Color& reflectance = Color(0.8f, 0.8f, 0.8f),
				const int slices = 64,
				const int stacks = 64,
				const Vector3& pos = Vector3::ZERO,
				const Vector3& scl = Vector3::UNIT_SCALE,
				const Vector3& rot = Vector3::ZERO);
			void LoadPlane(const float length,
				const BSDFType bsdfType = BSDFType::Diffuse,
				const Color& reflectance = Color(0.8f, 0.8f, 0.8f),
				const Vector3& pos = Vector3::ZERO,
				const Vector3& scl = Vector3::UNIT_SCALE,
				const Vector3& rot = Vector3::ZERO);

			void PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const;

			BSDF* GetBSDF(const uint triId) const;
			BSDF* GetBSDF_FromIdx(const uint idx) const;
			void SetBSDF(const BSDFType type, const int triId);
			void SetAreaLight(const AreaLight* pAreaLt)
			{
				mpAreaLight = pAreaLt;
			}

			const TriangleMesh* GetMesh() const
			{
				return mpMesh.Ptr();
			}
		};
	}
}