#pragma once

#include "EDXPrerequisites.h"
#include "Graphics/OpenGL.h"
#include "Graphics/ObjMesh.h"

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
			UniquePtr<OpenGL::VertexBuffer> mpVBO;
			UniquePtr<OpenGL::IndexBuffer> mpIBO;
			Array<UniquePtr<OpenGL::Texture2D>> mTextures;

			void SetTexture(int triId, const char* filePath)
			{
				if (filePath)
					mTextures[mpMesh->GetMaterialIdx(triId)].Reset(OpenGL::Texture2D::Create(filePath));
				else
					mTextures[mpMesh->GetMaterialIdx(triId)] = nullptr;
			}
		};

		const char* ScreenQuadVertShaderSource = R"(
		    varying vec2 screenPos;
			void main()
			{
				gl_Position = gl_Vertex;
				screenPos = gl_Vertex;
			})";
		const char* EnvMapFragShaderSource = R"(
			uniform sampler2D EnvTexSampler;
			uniform mat4 ScreenToWorld;
			uniform float rotation;
		    varying vec2 screenPos;
			void main()
			{
				vec4 worldPos = mul(vec4(screenPos, 1, 1), ScreenToWorld);
				vec3 worldDir = normalize(worldPos.xyz);

				float theta = acos(clamp(worldDir.y, -1.0f, 1.0f));
				float p = atan(worldDir.z, worldDir.x);
				float twoPi = 2.0f * 3.1415926f;
				float phi = (p < 0.0f) ? p + twoPi : p;
				phi += rotation;
				if(phi > twoPi)
					phi -= twoPi;
				vec2 envTexCoord = vec2(phi / twoPi, theta / 3.1415926f);
				vec4 sample = texture2D(EnvTexSampler, envTexCoord);
				sample.x = pow(sample.x, 0.454545f);
				sample.y = pow(sample.y, 0.454545f);
				sample.z = pow(sample.z, 0.454545f);
				gl_FragColor = sample;
			})";

		class Previewer
		{
		private:
			Camera* mpCamera;
			Array<UniquePtr<GLMesh>> mMeshes;
			OpenGL::Texture2D mEnvMap;
			const Scene* mpScene;
			const EnvironmentLight* mpCachedEnvLight;
			int mPickedPrimIdx;
			int mPickedTriIdx;

			Shader mScreenQuadVertexShader;
			Shader mEnvMapFragmentShader;
			Program mProgram;

		public:
			bool mSetFocusDistance;
			bool mLockCameraMovement;

		public:
			void Initialize(const Scene& scene, const Camera* pCamera)
			{
				mpScene = &scene;
				mpCamera = const_cast<Camera*>(pCamera);
				mPickedPrimIdx = -1;
				mSetFocusDistance = false;
				mLockCameraMovement = false;

				auto& prims = mpScene->GetPrimitives();
				for (auto& it : prims)
				{
					const ObjMesh* pMesh = it->GetMesh()->GetObjMeshHandle();
					GLMesh* glMesh = new GLMesh;
					glMesh->mpMesh = const_cast<ObjMesh*>(pMesh);

					glMesh->mpVBO = MakeUnique<OpenGL::VertexBuffer>();
					glMesh->mpVBO->SetData(pMesh->GetVertexCount() * sizeof(MeshVertex), (void*)&pMesh->GetVertexAt(0));
					glMesh->mpIBO = MakeUnique<OpenGL::IndexBuffer>();
					glMesh->mpIBO->SetData(3 * pMesh->GetTriangleCount() * sizeof(uint), (void*)pMesh->GetIndexAt(0));

					const auto& materialInfo = pMesh->GetMaterialInfo();
					glMesh->mTextures.Resize(materialInfo.Size());
					for (auto i = 0; i < materialInfo.Size(); i++)
					{
						if (materialInfo[i].strTexturePath[0])
						{
							glMesh->mTextures[i].Reset(OpenGL::Texture2D::Create(materialInfo[i].strTexturePath));
						}
						else
						{
							glMesh->mTextures[i] = nullptr;
						}
					}

					mMeshes.Add(UniquePtr<GLMesh>(glMesh));
				}

				mScreenQuadVertexShader.Load(ShaderType::VertexShader, ScreenQuadVertShaderSource);
				mEnvMapFragmentShader.Load(ShaderType::FragmentShader, EnvMapFragShaderSource);
				mProgram.AttachShader(&mScreenQuadVertexShader);
				mProgram.AttachShader(&mEnvMapFragmentShader);
				mProgram.Link();
			}

			void OnRender()
			{
				mpCamera->Transform();

				RenderEnvMap();

				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();

				glPushAttrib(GL_ALL_ATTRIB_BITS);

				glEnable(GL_LIGHTING);
				glEnable(GL_LIGHT0);
				GLfloat mat_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0 };
				GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
				GLfloat mat_shininess[] = { 50.0 };
				GLfloat light_position[] = { 1.0, 1.0, -1.0, 0.0 };
				glShadeModel(GL_SMOOTH);
				glEnable(GL_NORMALIZE);

				glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
				glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
				glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
				glLightfv(GL_LIGHT0, GL_POSITION, light_position);
				glLightfv(GL_LIGHT0, GL_AMBIENT, mat_specular);

				glMatrixMode(GL_MODELVIEW);
				const Matrix mModelView = mpCamera->GetViewMatrix() * mpScene->GetScaleMatrix();
				glLoadTransposeMatrixf((float*)&mModelView);

				glMatrixMode(GL_PROJECTION);
				const Matrix& mProj = mpCamera->GetProjMatrix();
				glLoadTransposeMatrixf((float*)&mProj);

				glEnable(GL_DEPTH_TEST);
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER, 0.0f);
				//glDisable(GL_LIGHTING);

				auto idx = 0;
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
						auto mtlIdx = pObjMesh->GetSubsetMtlIndex(i);
						auto pBsdf = mpScene->GetPrimitives()[idx]->GetBSDF_FromIdx(mtlIdx);
						if (it->mTextures[mtlIdx])
						{
							glEnable(GL_TEXTURE_2D);
							glMaterialfv(GL_FRONT, GL_DIFFUSE, (float*)&Color::WHITE);

							it->mTextures[mtlIdx]->Bind();
							it->mTextures[mtlIdx]->SetFilter(TextureFilter::Anisotropic16x);
						}
						else
						{
							glDisable(GL_TEXTURE_2D);
							auto pBsdf = mpScene->GetPrimitives()[idx]->GetBSDF_FromIdx(pObjMesh->GetSubsetMtlIndex(i));
							Parameter param;
							param = pBsdf->GetParameter("Color");
							Color color = Color(param.R, param.G, param.B);
							glMaterialfv(GL_FRONT, GL_DIFFUSE, (float*)&color);

							if (it->mTextures[mtlIdx] != nullptr)
								it->mTextures[mtlIdx] = nullptr;
						}

						auto setTriangleCount = pObjMesh->GetSubsetStartIdx(i + 1) - pObjMesh->GetSubsetStartIdx(i);
						glDrawRangeElements(GL_TRIANGLES, 0, setTriangleCount, setTriangleCount, GL_UNSIGNED_INT, (void*)(pObjMesh->GetSubsetStartIdx(i) * sizeof(uint)));

						if (it->mTextures[mtlIdx])
							it->mTextures[mtlIdx]->UnBind();

						if (idx == mPickedPrimIdx && pObjMesh->GetSubsetMtlIndex(i) == pObjMesh->GetMaterialIdx(mPickedTriIdx))
						{
							glDisable(GL_LIGHTING);
							glColor3f(1.0f, 1.0f, 0.0f);
							glPolygonMode(GL_FRONT, GL_LINE);
							glPolygonMode(GL_BACK, GL_FILL);
							glLineWidth(3.0f);

							auto setTriangleCount = pObjMesh->GetSubsetStartIdx(i + 1) - pObjMesh->GetSubsetStartIdx(i);
							glDrawRangeElements(GL_TRIANGLES, 0, setTriangleCount, setTriangleCount, GL_UNSIGNED_INT, (void*)(pObjMesh->GetSubsetStartIdx(i) * sizeof(uint)));

							glLineWidth(1.0f);
							glEnable(GL_LIGHTING);
							glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
						}
					}

					glDisableClientState(GL_VERTEX_ARRAY);
					glDisableClientState(GL_NORMAL_ARRAY);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					it->mpIBO->UnBind();
					it->mpVBO->UnBind();

					idx++;
				}

				glPopAttrib();
			}

			void RenderEnvMap()
			{
				if (auto* pMap = dynamic_cast<const EnvironmentLight*>(mpScene->GetEnvironmentLight()))
				{
					if (mpCachedEnvLight != pMap)
					{
						mpCachedEnvLight = pMap;
						if (mpCachedEnvLight->IsTexture())
						{
							mEnvMap.Load(ImageFormat::RGBA, ImageFormat::RGBA, ImageDataType::Float,
								(void*)ImageTexture<Color, Color>::GetLevelMemoryPtr(*dynamic_cast<ImageTexture<Color, Color>*>(mpCachedEnvLight->GetTexture())),
								mpCachedEnvLight->GetTexture()->Width(),
								mpCachedEnvLight->GetTexture()->Height());
						}
						else
						{
							Color color = mpCachedEnvLight->GetTexture()->GetValue();
							mEnvMap.Load(ImageFormat::RGBA, ImageFormat::RGBA, ImageDataType::Float,
								(float*)&color, 1, 1);
						}
					}
				}
				if (mpCachedEnvLight)
				{
					mProgram.Use();

					Matrix mViewProj = Matrix::Mul(mpCamera->GetProjMatrix(), mpCamera->GetViewMatrix());
					mProgram.SetUniform("ScreenToWorld", Matrix::Inverse(mViewProj), false);
					mProgram.SetUniform("EnvTexSampler", 0);
					mProgram.SetUniform("rotation", mpCachedEnvLight->GetRotation());
					mEnvMap.Bind();
					mEnvMap.SetFilter(TextureFilter::TriLinear);

					glBegin(GL_QUADS);

					glVertex2f(-1.0f, -1.0f);
					glVertex2f(1.0f, -1.0f);
					glVertex2f(1.0f, 1.0f);
					glVertex2f(-1.0f, 1.0f);

					glEnd();

					mProgram.Unuse();
					mEnvMap.UnBind();
				}
			}

			void OnResize(int width, int height)
			{
				mpCamera->Resize(width, height);
			}

			void Pick(const int x, const int y)
			{
				CameraSample camSample = { x, y, 0.0f, 0.0f, 0.0f };

				Ray ray;
				mpCamera->GenerateRay(camSample, &ray, true);

				Intersection isect;
				if (mpScene->Intersect(ray, &isect))
				{
					if (!mSetFocusDistance)
					{
						mPickedPrimIdx = isect.mPrimId;
						mPickedTriIdx = isect.mTriId;
					}
					else
					{
						mpCamera->mFocalPlaneDist = isect.mDist;
					}
				}
				else
				{
					mPickedPrimIdx = -1;
					mPickedTriIdx = -1;
				}
			}

			void HandleMouseMsg(const MouseEventArgs& args)
			{
				if (!mLockCameraMovement)
					mpCamera->HandleMouseMsg(args);

				if (args.Action == MouseAction::LButtonDown && (GetAsyncKeyState(VK_CONTROL) & (1 << 15)))
					Pick(args.x, args.y);
			}

			void HandleKeyboardMsg(const KeyboardEventArgs& args)
			{
				if (!mLockCameraMovement)
					mpCamera->HandleKeyboardMsg(args);
			}

			Camera& GetCamera()
			{
				return *mpCamera;
			}

			GLMesh* GetMesh(int meshId) const
			{
				return mMeshes[meshId].Get();
			}

			int GetPickedPrimId() const
			{
				return mPickedPrimIdx;
			}

			int GetPickedTriangleId() const
			{
				return mPickedTriIdx;
			}
		};
	}
}