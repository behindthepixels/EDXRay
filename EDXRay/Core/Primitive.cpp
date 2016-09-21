#include "Primitive.h"
#include "BSDF.h"
#include "BSSRDF.h"
#include "../Media/Homogeneous.h"
#include "TriangleMesh.h"
#include "DifferentialGeom.h"
#include "Graphics/ObjMesh.h"
#include "Core/Memory.h"

namespace EDX
{
	namespace RayTracer
	{
		Primitive::~Primitive()
		{
			Memory::SafeDeleteArray(mpMaterialIndices);
		}

		void Primitive::LoadMesh(const char* path,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot,
			const bool forceComputeNormal,
			const MediumInterface& mediumInterface,
			const Vector3& meanFreePath)
		{
			ObjMesh* pObjMesh = new ObjMesh;
			pObjMesh->LoadFromObj(pos, scl, rot, path, forceComputeNormal, true);

			mpMesh = MakeUnique<TriangleMesh>();
			mpMesh->LoadMesh(pObjMesh);

			// Initialize materials
			const auto& materialInfo = pObjMesh->GetMaterialInfo();
			for (auto i = 0; i < materialInfo.Size(); i++)
			{
				if (materialInfo[i].strTexturePath[0])
				{
					mpBSDFs.Add(BSDF::CreateBSDF(BSDFType::Diffuse, materialInfo[i].strTexturePath));
				}
				else
				{
					if (materialInfo[i].transColor != Color::BLACK)
						mpBSDFs.Add(BSDF::CreateBSDF(BSDFType::Glass, materialInfo[i].transColor));
					else if (materialInfo[i].specColor != Color::BLACK)
						mpBSDFs.Add(BSDF::CreateBSDF(BSDFType::Mirror, materialInfo[i].specColor));
					else
						mpBSDFs.Add(BSDF::CreateBSDF(BSDFType::Diffuse, materialInfo[i].color));
				}

				mMediumInterfaces.Add(mediumInterface);
				if (meanFreePath != Vector3::ZERO)
					mpBSSRDFs.Add(MakeUnique<BSSRDF>(mpBSDFs.Top().Get(), meanFreePath));
				else
					mpBSSRDFs.Add(nullptr);
			}

			mpMaterialIndices = new uint[mpMesh->GetTriangleCount()];
			for (auto i = 0; i < mpMesh->GetTriangleCount(); i++)
				mpMaterialIndices[i] = pObjMesh->GetMaterialIdx(i);
		}

		void Primitive::LoadMesh(const char* path,
			const BSDFType bsdfType,
			const Color& reflectance,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot,
			const bool forceComputeNormal,
			const MediumInterface& mediumInterface,
			const Vector3& meanFreePath)
		{
			ObjMesh* pObjMesh = new ObjMesh;
			pObjMesh->LoadFromObj(pos, scl, rot, path, forceComputeNormal, true);

			mpMesh = MakeUnique<TriangleMesh>();
			mpMesh->LoadMesh(pObjMesh);

			// Initialize materials
			const auto& materialInfo = pObjMesh->GetMaterialInfo();
			for (auto i = 0; i < materialInfo.Size(); i++)
			{
				mpBSDFs.Add(BSDF::CreateBSDF(bsdfType, reflectance));
				mMediumInterfaces.Add(mediumInterface);
				if (meanFreePath != Vector3::ZERO)
					mpBSSRDFs.Add(MakeUnique<BSSRDF>(mpBSDFs.Top().Get(), meanFreePath));
				else
					mpBSSRDFs.Add(nullptr);
			}

			mpMaterialIndices = new uint[mpMesh->GetTriangleCount()];
			for (auto i = 0; i < mpMesh->GetTriangleCount(); i++)
				mpMaterialIndices[i] = pObjMesh->GetMaterialIdx(i);
		}

		void Primitive::LoadSphere(const float radius,
			const BSDFType bsdfType,
			const Color& reflectance,
			const int slices,
			const int stacks,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot,
			const MediumInterface& mediumInterface,
			const Vector3& meanFreePath)
		{
			ObjMesh* pObjMesh = new ObjMesh;
			pObjMesh->LoadSphere(pos, scl, rot, radius, slices, stacks);

			mpMesh = MakeUnique<TriangleMesh>();
			mpMesh->LoadMesh(pObjMesh);

			// Initialize materials
			mpBSDFs.Add(BSDF::CreateBSDF(bsdfType, reflectance));
			mMediumInterfaces.Add(mediumInterface);
			if (meanFreePath != Vector3::ZERO)
				mpBSSRDFs.Add(MakeUnique<BSSRDF>(mpBSDFs.Top().Get(), meanFreePath));
			else
				mpBSSRDFs.Add(nullptr);

			mpMaterialIndices = new uint[mpMesh->GetTriangleCount()];
			for (auto i = 0; i < mpMesh->GetTriangleCount(); i++)
				mpMaterialIndices[i] = pObjMesh->GetMaterialIdx(i);
		}

		void Primitive::LoadPlane(const float length,
			const BSDFType bsdfType,
			const Color& reflectance,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot,
			const MediumInterface& mediumInterface,
			const Vector3& meanFreePath)
		{
			ObjMesh* pObjMesh = new ObjMesh;
			pObjMesh->LoadPlane(pos, scl, rot, length);

			mpMesh = MakeUnique<TriangleMesh>();
			mpMesh->LoadMesh(pObjMesh);

			// Initialize materials
			mpBSDFs.Add(BSDF::CreateBSDF(bsdfType, reflectance));
			mMediumInterfaces.Add(mediumInterface);
			if (meanFreePath != Vector3::ZERO)
				mpBSSRDFs.Add(MakeUnique<BSSRDF>(mpBSDFs.Top().Get(), meanFreePath));
			else
				mpBSSRDFs.Add(nullptr);

			mpMaterialIndices = new uint[mpMesh->GetTriangleCount()];
			for (auto i = 0; i < mpMesh->GetTriangleCount(); i++)
				mpMaterialIndices[i] = pObjMesh->GetMaterialIdx(i);
		}

		void Primitive::PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const
		{
			pDiffGeom->mpBSDF = mpBSDFs[mpMaterialIndices[pDiffGeom->mTriId]].Get();
			pDiffGeom->mpBSSRDF = mpBSSRDFs[mpMaterialIndices[pDiffGeom->mTriId]].Get();
			pDiffGeom->mpAreaLight = this->mpAreaLight;
			pDiffGeom->mMediumInterface = this->mMediumInterfaces[mpMaterialIndices[pDiffGeom->mTriId]];
			mpMesh->PostIntersect(ray, pDiffGeom);
		}

		BSDF* Primitive::GetBSDF(const uint triId)
		{
			return mpBSDFs[mpMaterialIndices[triId]].Get();
		}

		BSSRDF* Primitive::GetBSSRDF(const uint triId)
		{
			return mpBSSRDFs[mpMaterialIndices[triId]].Get();
		}

		MediumInterface* Primitive::GetMediumInterface(const uint triId)
		{
			return &mMediumInterfaces[mpMaterialIndices[triId]];
		}

		BSDF* Primitive::GetBSDF_FromIdx(const uint idx) const
		{
			return mpBSDFs[idx].Get();
		}

		void Primitive::SetBSDF(const BSDFType type, const int triId)
		{
			auto& bsdf = mpBSDFs[mpMaterialIndices[triId]];
			auto newBsdf = BSDF::CreateBSDF(type, bsdf.Get());
			bsdf = Move(newBsdf);
		}

		void Primitive::SetBSSRDF(const int triId, const bool setNull)
		{
			auto& bsdf = mpBSDFs[mpMaterialIndices[triId]];
			auto& bssrdf = mpBSSRDFs[mpMaterialIndices[triId]];
			auto newBssrdf = !setNull ? MakeUnique<BSSRDF>(bsdf.Get(), Vector3(0.05f)) : nullptr;
			bssrdf = Move(newBssrdf);
		}

		void Primitive::SetMediumInterface(const int triId, const bool setNull)
		{
			auto& materialInterface = mMediumInterfaces[mpMaterialIndices[triId]];
			materialInterface.SetInside(!setNull ? new HomogeneousMedium(Vector3(0.1f), Vector3(0.02f), 0.0f) : nullptr);
		}
	}
}