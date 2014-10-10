#include "TriangleMesh.h"

#include "Graphics/ObjMesh.h"
#include "DifferentialGeom.h"
#include "Math/Ray.h"
#include "Memory/Memory.h"

#include "BSDF.h"

namespace EDX
{
	namespace RayTracer
	{
		void TriangleMesh::LoadMesh(const ObjMesh* pObjMesh, const BSDFType bsdfType)
		{
			mpObjMesh = pObjMesh;

			// Init vertex buffer data
			mVertexCount = pObjMesh->GetVertexCount();
			mpPositionBuffer = new Vector3[mVertexCount];
			mpNormalBuffer = new Vector3[mVertexCount];
			mpTexcoordBuffer = new Vector2[mVertexCount];
			for (auto i = 0; i < mVertexCount; i++)
			{
				const MeshVertex& vertex = pObjMesh->GetVertexAt(i);
				mpPositionBuffer[i] = vertex.position;
				mpNormalBuffer[i] = vertex.normal;
				mpTexcoordBuffer[i] = Vector2(vertex.fU, vertex.fV);
			}

			// Init index buffer data
			mTriangleCount = pObjMesh->GetTriangleCount();
			mpIndexBuffer = new uint[3 * mTriangleCount];
			memcpy(mpIndexBuffer, pObjMesh->GetIndexAt(0), 3 * mTriangleCount * sizeof(uint));

			// Initialize materials
			const auto& materialInfo = pObjMesh->GetMaterialInfo();
			for (auto i = 0; i < materialInfo.size(); i++)
			{
				if (materialInfo[i].strTexturePath[0])
					mpBSDFs.push_back(BSDF::CreateBSDF(bsdfType, materialInfo[i].strTexturePath));
				else
					mpBSDFs.push_back(BSDF::CreateBSDF(bsdfType, materialInfo[i].color));
			}

			mpMaterialIndices = new uint[mTriangleCount];
			for (auto i = 0; i < mTriangleCount; i++)
				mpMaterialIndices[i] = pObjMesh->GetMaterialIdx(i);
		}

		void TriangleMesh::LoadMesh(const char* path,
			const BSDFType bsdfType,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot)
		{
			ObjMesh* pMesh = new ObjMesh;
			pMesh->LoadFromObj(pos, scl, rot, path, true);

			LoadMesh(pMesh, bsdfType);
		}

		void TriangleMesh::LoadSphere(const float radius,
			const int slices,
			const int stacks,
			const BSDFType bsdfType,
			const Vector3& pos,
			const Vector3& scl,
			const Vector3& rot)
		{
			ObjMesh* pMesh = new ObjMesh;
			pMesh->LoadSphere(pos, scl, rot, radius, slices, stacks);

			LoadMesh(pMesh, bsdfType);
		}

		void TriangleMesh::PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const
		{
			assert(pDiffGeom);

			const Vector3& vt1 = GetPositionAt(3 * pDiffGeom->mTriId);
			const Vector3& vt2 = GetPositionAt(3 * pDiffGeom->mTriId + 1);
			const Vector3& vt3 = GetPositionAt(3 * pDiffGeom->mTriId + 2);

			const Vector3& normal1 = GetNormalAt(3 * pDiffGeom->mTriId);
			const Vector3& normal2 = GetNormalAt(3 * pDiffGeom->mTriId + 1);
			const Vector3& normal3 = GetNormalAt(3 * pDiffGeom->mTriId + 2);

			const Vector2& texcoord1 = GetTexCoordAt(3 * pDiffGeom->mTriId);
			const Vector2& texcoord2 = GetTexCoordAt(3 * pDiffGeom->mTriId + 1);
			const Vector2& texcoord3 = GetTexCoordAt(3 * pDiffGeom->mTriId + 2);

			const float u = pDiffGeom->mU;
			const float v = pDiffGeom->mV;
			const float w = 1.0f - u - v;

			pDiffGeom->mPosition = ray.CalcPoint(pDiffGeom->mDist);
			pDiffGeom->mNormal = Math::Normalize(w * normal1 + u * normal2 + v * normal3);
			pDiffGeom->mTexcoord = w * texcoord1 + u * texcoord2 + v * texcoord3;


			Vector3 e1 = vt1 - vt2;
			Vector3 e2 = vt3 - vt1;
			pDiffGeom->mGeomNormal = Math::Normalize(Math::Cross(e2, e1));

			Vector2 d1 = texcoord1 - texcoord2;
			Vector2 d2 = texcoord3 - texcoord1;
			float det = Math::Cross(d1, d2);
			Vector3 dpdu, dpdv;
			if (det == 0.0f)
			{
				Math::CoordinateSystem(pDiffGeom->mNormal, &dpdu, &dpdv);
			}
			else
			{
				float invDet = 1.f / det;
				dpdu = (d2.v * e1 - d1.v * e2) * invDet;
				dpdv = (-d2.u * e1 + d1.u * e2) * invDet;
			}
			pDiffGeom->mDpdu = dpdu;
			pDiffGeom->mDpdv = dpdv;

			pDiffGeom->mShadingFrame = Frame(pDiffGeom->mNormal);
			pDiffGeom->mGeomFrame = Frame(pDiffGeom->mGeomNormal);

			pDiffGeom->mpBSDF = mpBSDFs[mpMaterialIndices[pDiffGeom->mTriId]].Ptr();
		}

		const Vector3& TriangleMesh::GetPositionAt(uint idx) const
		{
			assert(idx < 3 * mTriangleCount);
			assert(mpIndexBuffer);
			assert(mpPositionBuffer);
			return mpPositionBuffer[mpIndexBuffer[idx]];
		}

		const Vector3& TriangleMesh::GetNormalAt(uint idx) const
		{
			assert(idx < 3 * mTriangleCount);
			assert(mpIndexBuffer);
			assert(mpPositionBuffer);
			return mpNormalBuffer[mpIndexBuffer[idx]];
		}

		const Vector2& TriangleMesh::GetTexCoordAt(uint idx) const
		{
			assert(idx < 3 * mTriangleCount);
			assert(mpIndexBuffer);
			assert(mpPositionBuffer);
			return mpTexcoordBuffer[mpIndexBuffer[idx]];
		}

		void TriangleMesh::Release()
		{
			SafeDeleteArray(mpPositionBuffer);
			SafeDeleteArray(mpNormalBuffer);
			SafeDeleteArray(mpTexcoordBuffer);
			SafeDeleteArray(mpIndexBuffer);
			SafeDeleteArray(mpMaterialIndices);
			mVertexCount = 0;
			mTriangleCount = 0;
		}
	}
}