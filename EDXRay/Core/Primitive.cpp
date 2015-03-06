#include "Primitive.h"
#include "TriangleMesh.h"
#include "DifferentialGeom.h"
#include "Graphics/ObjMesh.h"
#include "Memory/Memory.h"

namespace EDX
{
	namespace RayTracer
	{
		Primitive::~Primitive()
		{
			SafeDeleteArray(mpMaterialIndices);
		}

		void Primitive::LoadMesh(const char* path,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot)
		{
			ObjMesh* pObjMesh = new ObjMesh;
			pObjMesh->LoadFromObj(pos, scl, rot, path, true);

			mpMesh = new TriangleMesh;
			mpMesh->LoadMesh(pObjMesh);

			// Initialize materials
			const auto& materialInfo = pObjMesh->GetMaterialInfo();
			for (auto i = 0; i < materialInfo.size(); i++)
			{
				if (materialInfo[i].strTexturePath[0])
				{
					mpBSDFs.push_back(BSDF::CreateBSDF(BSDFType::Diffuse, materialInfo[i].strTexturePath));
				}
				else
				{
					if (materialInfo[i].transColor != Color::BLACK)
						mpBSDFs.push_back(BSDF::CreateBSDF(BSDFType::Glass, materialInfo[i].transColor));
					else if (materialInfo[i].specColor != Color::BLACK)
						mpBSDFs.push_back(BSDF::CreateBSDF(BSDFType::Mirror, materialInfo[i].specColor));
					else
						mpBSDFs.push_back(BSDF::CreateBSDF(BSDFType::Diffuse, materialInfo[i].color));
				}
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
			const Vector3& rot)
		{
			ObjMesh* pObjMesh = new ObjMesh;
			pObjMesh->LoadFromObj(pos, scl, rot, path, true);

			mpMesh = new TriangleMesh;
			mpMesh->LoadMesh(pObjMesh);

			// Initialize materials
			const auto& materialInfo = pObjMesh->GetMaterialInfo();
			for (auto i = 0; i < materialInfo.size(); i++)
				mpBSDFs.push_back(BSDF::CreateBSDF(bsdfType, reflectance));

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
			const Vector3& rot)
		{
			ObjMesh* pObjMesh = new ObjMesh;
			pObjMesh->LoadSphere(pos, scl, rot, radius, slices, stacks);

			mpMesh = new TriangleMesh;
			mpMesh->LoadMesh(pObjMesh);

			// Initialize materials
			const auto& materialInfo = pObjMesh->GetMaterialInfo();
			for (auto i = 0; i < materialInfo.size(); i++)
			{
				if (materialInfo[i].strTexturePath[0])
					mpBSDFs.push_back(BSDF::CreateBSDF(bsdfType, materialInfo[i].strTexturePath));
				else
					mpBSDFs.push_back(BSDF::CreateBSDF(bsdfType, reflectance));
			}

			mpMaterialIndices = new uint[mpMesh->GetTriangleCount()];
			for (auto i = 0; i < mpMesh->GetTriangleCount(); i++)
				mpMaterialIndices[i] = pObjMesh->GetMaterialIdx(i);
		}

		void Primitive::LoadPlane(const float length,
			const BSDFType bsdfType,
			const Color& reflectance,
			const Color& color,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot)
		{
			ObjMesh* pObjMesh = new ObjMesh;
			pObjMesh->LoadPlane(pos, scl, rot, length);

			mpMesh = new TriangleMesh;
			mpMesh->LoadMesh(pObjMesh);

			// Initialize materials
			const auto& materialInfo = pObjMesh->GetMaterialInfo();
			for (auto i = 0; i < materialInfo.size(); i++)
			{
				mpBSDFs.push_back(BSDF::CreateBSDF(bsdfType, reflectance));
			}

			mpMaterialIndices = new uint[mpMesh->GetTriangleCount()];
			for (auto i = 0; i < mpMesh->GetTriangleCount(); i++)
				mpMaterialIndices[i] = pObjMesh->GetMaterialIdx(i);
		}

		void Primitive::PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const
		{
			pDiffGeom->mpBSDF = mpBSDFs[mpMaterialIndices[pDiffGeom->mTriId]].Ptr();
			mpMesh->PostIntersect(ray, pDiffGeom);
		}

		BSDF* Primitive::GetBSDF(const uint triId) const
		{
			return mpBSDFs[mpMaterialIndices[triId]].Ptr();
		}

		BSDF* Primitive::GetBSDF_FromIdx(const uint idx) const
		{
			return mpBSDFs[idx].Ptr();
		}

		void Primitive::SetBSDF(const BSDFType type, const int triId)
		{
			auto& bsdf = mpBSDFs[mpMaterialIndices[triId]];
			auto newBsdf = BSDF::CreateBSDF(type, bsdf->GetTexture(), bsdf->IsTextured());
			bsdf = newBsdf;
		}
	}
}