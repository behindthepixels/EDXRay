#pragma once

#include "EDXPrerequisites.h"
#include "Graphics/OpenGL.h"
#include "Graphics/ObjMesh.h"
#include "Memory/Memory.h"

#include "Core/Camera.h"
#include "Core/Scene.h"
#include "Core/Primitive.h"
#include "Core/TriangleMesh.h"

namespace EDX
{
	namespace RayTracer
	{
		using namespace OpenGL;

		struct GLMesh
		{
			ObjMesh* mpMesh;
			RefPtr<OpenGL::VertexBuffer> mpVBO;
			RefPtr<OpenGL::IndexBuffer> mpIBO;
			vector<RefPtr<OpenGL::Texture2D>> mTextures;
		};

		class Previewer
		{
		private:
			Camera mCamera;
			vector<RefPtr<GLMesh>> mMeshes;

		public:
			void Initialize(const Scene& scene, int windowWidth, int windowHeight, float fov)
			{
				OpenGL::InitializeOpenGLExtensions();

				mCamera.Init(-5.0f * Vector3::UNIT_Z, Vector3::ZERO, Vector3::UNIT_Y, windowWidth, windowHeight, fov, 0.01f);

				auto& prims = scene.GetPrimitives();
				for (auto& it : prims)
				{
					const ObjMesh* pMesh = it->GetMesh()->GetObjMeshHandle();
					GLMesh* glMesh = new GLMesh;
					glMesh->mpMesh = const_cast<ObjMesh*>(pMesh);

					glMesh->mpVBO = new OpenGL::VertexBuffer;
					glMesh->mpVBO->SetData(pMesh->GetVertexCount() * sizeof(MeshVertex), (void*)&pMesh->GetVertexAt(0));
					glMesh->mpIBO = new OpenGL::IndexBuffer;
					glMesh->mpIBO->SetData(3 * pMesh->GetTriangleCount() * sizeof(uint), (void*)pMesh->GetIndexAt(0));

					const auto& materialInfo = pMesh->GetMaterialInfo();
					for (auto i = 0; i < materialInfo.size(); i++)
					{
						if (materialInfo[i].strTexturePath[0])
							glMesh->mTextures.push_back(OpenGL::Texture2D::Create(materialInfo[i].strTexturePath));
						else
						{
							Color4b c = materialInfo[i].color;
							OpenGL::Texture2D* pTex = new OpenGL::Texture2D;
							pTex->Load(ImageFormat::RGBA, ImageFormat::RGBA, ImageDataType::Byte, &c, 1, 1);
							glMesh->mTextures.push_back(pTex);
						}
					}

					mMeshes.push_back(glMesh);
				}
			}

			void OnRender()
			{
				glMatrixMode(GL_MODELVIEW);

				const Matrix& mView = mCamera.GetViewMatrix();
				glLoadTransposeMatrixf((float*)&mView);

				glPushAttrib(GL_ALL_ATTRIB_BITS);
				glEnable(GL_TEXTURE_2D);
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER, 0.0f);
				glDisable(GL_LIGHTING);

				for (auto& it : mMeshes)
				{
					auto pObjMesh = it->mpMesh;

					it->mpVBO->Bind();
					glEnableClientState(GL_VERTEX_ARRAY);
					glVertexPointer(3, GL_FLOAT, sizeof(MeshVertex), 0);
					glEnableClientState(GL_NORMAL_ARRAY);
					glNormalPointer(GL_FLOAT, sizeof(MeshVertex), reinterpret_cast<void*>(offsetof(MeshVertex, normal)));
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, sizeof(MeshVertex), reinterpret_cast<void*>(offsetof(MeshVertex, fU)));
					it->mpIBO->Bind();

					for (auto i = 0; i < pObjMesh->GetSubsetCount(); i++)
					{
						it->mTextures[pObjMesh->GetSubsetMtlIndex(i)]->Bind();
						it->mTextures[pObjMesh->GetSubsetMtlIndex(i)]->SetFilter(TextureFilter::Anisotropic16x);
						auto setTriangleCount = pObjMesh->GetSubsetStartIdx(i + 1) - pObjMesh->GetSubsetStartIdx(i);
						glDrawRangeElements(GL_TRIANGLES, 0, setTriangleCount, setTriangleCount, GL_UNSIGNED_INT, (void*)(pObjMesh->GetSubsetStartIdx(i) * sizeof(uint)));
						it->mTextures[pObjMesh->GetSubsetMtlIndex(i)]->UnBind();
					}

					glDisableClientState(GL_VERTEX_ARRAY);
					glDisableClientState(GL_NORMAL_ARRAY);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					it->mpIBO->UnBind();
					it->mpVBO->UnBind();
				}

				glPopAttrib();
			}

			void OnResize(int width, int height)
			{
				mCamera.Resize(width, height);

				glViewport(0, 0, width, height);

				glMatrixMode(GL_PROJECTION);
				const Matrix& mProj = mCamera.GetProjMatrix();
				glLoadTransposeMatrixf((float*)&mProj);
			}

			Camera& GetCamera()
			{
				return mCamera;
			}
		};
	}
}