#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"

#include "../Core/BSDF.h"
#include "Medium.h"
#include "Math/Vector.h"
#include "Memory/RefPtr.h"

namespace EDX
{
	namespace RayTracer
	{
		class Primitive
		{
		private:
			friend class Scene;

			RefPtr<TriangleMesh>	mpMesh;

			// Materials
			vector<RefPtr<BSDF>>	mpBSDFs;
			vector<RefPtr<BSSRDF>>	mpBSSRDFs;
			const AreaLight*		mpAreaLight = nullptr;
			uint*					mpMaterialIndices = nullptr;
			// Medium
			vector<MediumInterface>	mMediumInterfaces;

		public:
			Primitive()
			{
			}
			~Primitive();

			void LoadMesh(const char* path,
				const Vector3& pos = Vector3::ZERO,
				const Vector3& scl = Vector3::UNIT_SCALE,
				const Vector3& rot = Vector3::ZERO,
				const bool forceComputeNormal = false,
				const MediumInterface& mediumInterface = MediumInterface(),
				const Vector3& meanFreePath = Vector3::ZERO);
			void LoadMesh(const char* path,
				const BSDFType bsdfType = BSDFType::Diffuse,
				const Color& reflectance = Color(0.8f, 0.8f, 0.8f),
				const Vector3& pos = Vector3::ZERO,
				const Vector3& scl = Vector3::UNIT_SCALE,
				const Vector3& rot = Vector3::ZERO,
				const bool forceComputeNormal = false,
				const MediumInterface& mediumInterface = MediumInterface(),
				const Vector3& meanFreePath = Vector3::ZERO);
			void LoadSphere(const float radius,
				const BSDFType bsdfType = BSDFType::Diffuse,
				const Color& reflectance = Color(0.8f, 0.8f, 0.8f),
				const int slices = 64,
				const int stacks = 64,
				const Vector3& pos = Vector3::ZERO,
				const Vector3& scl = Vector3::UNIT_SCALE,
				const Vector3& rot = Vector3::ZERO,
				const MediumInterface& mediumInterface = MediumInterface(),
				const Vector3& meanFreePath = Vector3::ZERO);
			void LoadPlane(const float length,
				const BSDFType bsdfType = BSDFType::Diffuse,
				const Color& reflectance = Color(0.8f, 0.8f, 0.8f),
				const Vector3& pos = Vector3::ZERO,
				const Vector3& scl = Vector3::UNIT_SCALE,
				const Vector3& rot = Vector3::ZERO,
				const MediumInterface& mediumInterface = MediumInterface(),
				const Vector3& meanFreePath = Vector3::ZERO);

			void PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const;

			BSDF* GetBSDF(const uint triId);
			BSSRDF* GetBSSRDF(const uint triId);
			MediumInterface* GetMediumInterface(const uint triId);
			BSDF* GetBSDF_FromIdx(const uint idx) const;
			void SetBSDF(const BSDFType type, const int triId);
			void SetBSSRDF(const int triId, const bool setNull = false);
			void SetMediumInterface(const int triId, const bool setNull = false);
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