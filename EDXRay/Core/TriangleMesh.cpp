#include "TriangleMesh.h"

#include "../Core/Ray.h"
#include "Graphics/ObjMesh.h"
#include "DifferentialGeom.h"
#include "Core/Memory.h"

#include "BSDF.h"

namespace EDX
{
	namespace RayTracer
	{
		void TriangleMesh::LoadMesh(const ObjMesh* pObjMesh)
		{
			mpObjMesh.Reset(pObjMesh);

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
			Memory::Memcpy(mpIndexBuffer, pObjMesh->GetIndexAt(0), 3 * mTriangleCount * sizeof(uint));

			mTextured = mpObjMesh->IsTextured();
		}

		void TriangleMesh::PostIntersect(const Ray& ray, DifferentialGeom* pDiffGeom) const
		{
			Assert(pDiffGeom);

			const uint vId1 = 3 * pDiffGeom->mTriId;
			const uint vId2 = 3 * pDiffGeom->mTriId + 1;
			const uint vId3 = 3 * pDiffGeom->mTriId + 2;

			const Vector3& vt1 = GetPositionAt(vId1);
			const Vector3& vt2 = GetPositionAt(vId2);
			const Vector3& vt3 = GetPositionAt(vId3);

			const Vector3& normal1 = GetNormalAt(vId1);
			const Vector3& normal2 = GetNormalAt(vId2);
			const Vector3& normal3 = GetNormalAt(vId3);

			const float u = pDiffGeom->mU;
			const float v = pDiffGeom->mV;
			const float w = 1.0f - u - v;

			pDiffGeom->mPosition = w * vt1 + u * vt2 + v * vt3;
			pDiffGeom->mNormal = w * normal1 + u * normal2 + v * normal3;

			Vector3 e1 = vt1 - vt2;
			Vector3 e2 = vt3 - vt1;
			pDiffGeom->mGeomNormal = Math::Normalize(Math::Cross(e2, e1));

			if (pDiffGeom->mpBSDF->GetTexture() || pDiffGeom->mpBSDF->GetNormalMap())
			{
				const Vector2& texcoord1 = GetTexCoordAt(vId1);
				const Vector2& texcoord2 = GetTexCoordAt(vId2);
				const Vector2& texcoord3 = GetTexCoordAt(vId3);

				pDiffGeom->mTexcoord = w * texcoord1 + u * texcoord2 + v * texcoord3;

				Vector2 d1 = texcoord1 - texcoord2;
				Vector2 d2 = texcoord3 - texcoord1;
				float det = Math::Cross(d1, d2);

				Vector3 dn1 = normal1 - normal2;
				Vector3 dn2 = normal3 - normal1;

				if (det != 0.0f)
				{
					float invDet = 1.f / det;
					pDiffGeom->mDpdu = (d2.v * e1 - d1.v * e2) * invDet;
					pDiffGeom->mDpdv = (-d2.u * e1 + d1.u * e2) * invDet;
					pDiffGeom->mDndu = (d2.v * dn1 - d1.v * dn2) * invDet;
					pDiffGeom->mDndv = (-d2.u * dn1 + d1.u * dn2) * invDet;
				}
				else
				{
					Math::CoordinateSystem(pDiffGeom->mNormal, &pDiffGeom->mDpdu, &pDiffGeom->mDpdv);
					pDiffGeom->mDndu = pDiffGeom->mDndv = Vector3::ZERO;
				}

				pDiffGeom->ComputeDifferentials(ray);

				auto pNormalMap = pDiffGeom->mpBSDF->GetNormalMap();
				if (det != 0.0f && pNormalMap)
				{
					pDiffGeom->mShadingFrame = Frame(Math::Normalize(pDiffGeom->mDpdu),
						Math::Normalize(pDiffGeom->mDpdv),
						pDiffGeom->mNormal);

					Vector2 differential[2] = {
						(pDiffGeom->mDudx, pDiffGeom->mDvdx),
						(pDiffGeom->mDudy, pDiffGeom->mDvdy)
					};
					Color normalMapSample = pNormalMap->Sample(pDiffGeom->mTexcoord, differential, TextureFilter::TriLinear);

					Vector3 tangentNormal = 2.0f * (Vector3(normalMapSample.r, normalMapSample.g, normalMapSample.b) - Vector3(0.5f));
					Vector3 worldNormal = Math::Normalize(pDiffGeom->mShadingFrame.LocalToWorld(tangentNormal));
					pDiffGeom->mNormal = worldNormal;
					pDiffGeom->mShadingFrame = Frame(worldNormal);
				}
				else
					pDiffGeom->mShadingFrame = Frame(pDiffGeom->mNormal);
			}
			else
				pDiffGeom->mShadingFrame = Frame(pDiffGeom->mNormal);
		}

		const Vector3& TriangleMesh::GetPositionAt(uint idx) const
		{
			Assert(idx < 3 * mTriangleCount);
			Assert(mpIndexBuffer);
			Assert(mpPositionBuffer);
			return mpPositionBuffer[mpIndexBuffer[idx]];
		}

		const Vector3& TriangleMesh::GetNormalAt(uint idx) const
		{
			Assert(idx < 3 * mTriangleCount);
			Assert(mpIndexBuffer);
			Assert(mpPositionBuffer);
			return mpNormalBuffer[mpIndexBuffer[idx]];
		}

		const Vector2& TriangleMesh::GetTexCoordAt(uint idx) const
		{
			Assert(idx < 3 * mTriangleCount);
			Assert(mpIndexBuffer);
			Assert(mpPositionBuffer);
			return mpTexcoordBuffer[mpIndexBuffer[idx]];
		}

		void TriangleMesh::Release()
		{
			Memory::SafeDeleteArray(mpPositionBuffer);
			Memory::SafeDeleteArray(mpNormalBuffer);
			Memory::SafeDeleteArray(mpTexcoordBuffer);
			Memory::SafeDeleteArray(mpIndexBuffer);
			mVertexCount = 0;
			mTriangleCount = 0;
		}
	}
}