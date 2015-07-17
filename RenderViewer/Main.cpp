
#include "Windows/Window.h"
#include "Windows/Application.h"
#include "Memory/Memory.h"

#include "Graphics/OpenGL.h"

#include "Core/Renderer.h"
#include "Core/Film.h"
#include "Core/Scene.h"
#include "Core/Primitive.h"
#include "Core/TriangleMesh.h"
#include "Lights/PointLight.h"
#include "Lights/DirectionalLight.h"
#include "Lights/EnvironmentLight.h"
#include "Core/BSDF.h"
#include "BSDFs/RoughConductor.h"
#include "BSDFs/RoughDielectric.h"

#include "ScenePreviewer.h"

#include "Graphics/EDXGui.h"
#include "Windows/Bitmap.h"

using namespace EDX;
using namespace EDX::RayTracer;
using namespace EDX::GUI;

Renderer*	gpRenderer = nullptr;
Previewer*	gpPreview = nullptr;
bool gRendering = false;
Color gCursorColor;

void GUIEvent(Object* pObject, EventArgs args);

void OnInit(Object* pSender, EventArgs args)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	gpRenderer = new Renderer;

	Scene* pScene = gpRenderer->GetScene().Ptr();
	Primitive* pMesh = new Primitive;
	Primitive* pMesh2 = new Primitive;
	Primitive* pMesh3 = new Primitive;
	Primitive* pMesh4 = new Primitive;
	//pMesh->LoadMesh("../../Media/sponza/sponza.obj", Vector3(0, 0, 0), 0.01f * Vector3::UNIT_SCALE, Vector3(0, 90, 0));
	//pMesh->LoadMesh("../../Media/crytek-sponza/sponza.obj", Vector3(0, 0, 0), 0.01f * Vector3::UNIT_SCALE, Vector3(0, 90, 0));
	//pMesh->LoadMesh("../../Media/cornell-box/cornellbox.obj", Vector3(0, 0, 0), 3.0f * Vector3::UNIT_SCALE, Vector3(0, 180, 0));
	//pMesh->LoadMesh("../../Media/san-miguel/san-miguel.obj", Vector3(-5, 0, -10), Vector3::UNIT_SCALE, Vector3(0, 0, 0));
	//pMesh->LoadSphere(1.5f, BSDFType::Diffuse, Color::WHITE, 32, 16, Vector3(0.0f, 3.0f, 0.0f));
	//pMesh2->LoadMesh("../../Media/venusm.obj", BSDFType::RoughDielectric, Color(0.7f, 0.37f, 0.3f), Vector3(1.5f, 0.88f, 0.0f), 0.001f * Vector3::UNIT_SCALE, Vector3(0.0f, 0.0f, 0.0f));
	//pMesh2->LoadMesh("../../Media/splash.obj", BSDFType::Glass, Color(1.0f, 1.0f, 1.0f), Vector3(4.95f, 0.06f, -4.95f), 9.9f * Vector3::UNIT_SCALE, Vector3(0.0f, 45.0f, 0.0f));
	//pMesh3->LoadSphere(2.0f, BSDFType::Diffuse, Color::WHITE, 128, 64, Vector3(0.0f, 2.0f, 0.0f));
	//pMesh3->LoadMesh("../../Media/bunny.obj", BSDFType::RoughConductor, Color(0.99f, 0.79f, 0.39f), Vector3(-1.5f, -0.5f, 0.0f), 0.16f * Vector3::UNIT_SCALE, Vector3(0.0f, 0.0f, 0.0f));

	float offset = 0.4f;
	pMesh->LoadMesh("../../Media/backdrop.obj", BSDFType::Diffuse, Color(0.35f), Vector3(20, 0, -25), 2.2f * Vector3::UNIT_SCALE, Vector3(0, 0, 0));
	pMesh2->LoadMesh("../../Media/EDXMaterialPreviewScenesEnvMap.obj", Vector3(0, 0, 0), Vector3::UNIT_SCALE, Vector3(0, 0, 0));
	//pMesh3->LoadMesh("../../Media/teapot.obj", BSDFType::RoughConductor, Color(0.99f, 0.79f, 0.39f), Vector3(-2.3f + offset, 0.0f, 1.2f - offset), 0.3f * Vector3::UNIT_SCALE, Vector3(0.0f, -150.0f, 0.0f));
	//pMesh2->LoadMesh("../../Media/teapot.obj", BSDFType::Mirror, Color(0.9f, 0.9f, 0.9f), Vector3(0.7f - offset, 0.0f, -2.3f + offset), 0.3f * Vector3::UNIT_SCALE, Vector3(0.0f, -15.0f, 0.0f));
	//pMesh4->LoadSphere(1.33f, BSDFType::Glass, Color::WHITE, 256, 128, Vector3(2.02f - offset, 1.33f, 2.02f - offset));
	//pMesh4->LoadMesh("../../Media/OceanMesh2.obj", BSDFType::RoughDielectric, Color(1.0f), Vector3::ZERO, Vector3(1, 10, 1), Vector3(0.0f, 180.0f, 0.0f));
	//pMesh2->LoadMesh("../../Media/OceanMesh1.obj", BSDFType::RoughDielectric, Color(1.0f), Vector3(-13000, -5, 0), Vector3(50, 500, 50), Vector3(0.0f, 180.0f, 0.0f));

	//Primitive* pPlane1 = new Primitive;
	//Primitive* pPlane2 = new Primitive;
	//Primitive* pPlane3 = new Primitive;
	//Primitive* pPlane4 = new Primitive;
	//Primitive* pPlane5 = new Primitive;
	//pPlane1->LoadPlane(10.0f, BSDFType::Diffuse, Color(0.2f), Vector3(0.0f, 0.0f, 0.0f), Vector3::UNIT_SCALE, Vector3::ZERO);
	//pPlane2->LoadPlane(10.0f, BSDFType::Diffuse, Color(0.9f, 0.9f, 0.9f), Vector3(0.0f, 10.0f, 0.0f), Vector3::UNIT_SCALE, Vector3(180.0f, 0.0f, 0.0f));
	//pPlane3->LoadPlane(10.0f, BSDFType::Diffuse, Color(0.9f, 0.6f, 0.6f), Vector3(5.0f, 5.0f, 0.0f), Vector3::UNIT_SCALE, Vector3(0.0f, 0.0f, 90.0f));
	//pPlane4->LoadPlane(10.0f, BSDFType::Diffuse, Color(0.6f, 0.6f, 0.9f), Vector3(-5.0f, 5.0f, 0.0f), Vector3::UNIT_SCALE, Vector3(0.0f, 0.0f, -90.0f));
	//pPlane5->LoadPlane(10.0f, BSDFType::Diffuse, Color(0.9f, 0.9f, 0.9f), Vector3(0.0f, 5.0f, -5.0f), Vector3::UNIT_SCALE, Vector3(90.0f, 0.0f, 0.0f));

	//pScene->AddPrimitive(pPlane1);
	//pScene->AddPrimitive(pPlane2);
	//pScene->AddPrimitive(pPlane3);
	//pScene->AddPrimitive(pPlane4);
	//pScene->AddPrimitive(pPlane5);

	pScene->AddPrimitive(pMesh);
	pScene->AddPrimitive(pMesh2);
	//pScene->AddPrimitive(pMesh3);
	//pScene->AddPrimitive(pMesh4);
	pScene->AddLight(new EnvironmentLight("../../Media/uffizi-large.hdr", pScene, 1.0f));
	//pScene->AddLight(new EnvironmentLight(Color(3.0f), Color(0.2f), 40.0f, pScene, -60.0f));
	//pScene->AddLight(new DirectionalLight(Vector3(0.0f, 10.0f, 10.0f), Color(3.0f), pScene));
	//pScene->AddLight(new PointLight(Vector3(0.0f, 7.9f, 0.0f), Color(50.0f)));

	pScene->InitAccelerator();

	OpenGL::InitializeOpenGLExtensions();

	RenderJobDesc desc;
	desc.ImageWidth = Application::GetMainWindow()->GetWindowWidth();
	desc.ImageHeight = Application::GetMainWindow()->GetWindowHeight();
	desc.SamplesPerPixel = 4096;
	desc.CameraParams.FieldOfView = 40;
	desc.CameraParams.Pos = Vector3(-6.17641401f, 14.5548525f, 16.4850121f);
	desc.CameraParams.Target = Vector3(-5.86896896f, 14.0666752f, 15.6682129f);
	desc.CameraParams.LensRadius = 0.5f;
	gpPreview = new Previewer;
	gpPreview->Initialize(*pScene, desc);

	// Initialize UI
	EDXGui::Init();
}

void OnRender(Object* pSender, EventArgs args)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	auto jobDesc = gpRenderer->GetJobDesc();
	if (gRendering)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, jobDesc->ImageWidth, 0, jobDesc->ImageHeight, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glRasterPos3f(0.0f, 0.0f, 0.0f);
		glDrawPixels(jobDesc->ImageWidth, jobDesc->ImageHeight, GL_RGBA, GL_FLOAT, (float*)gpRenderer->GetFilm()->GetPixelBuffer());
	}
	else
		gpPreview->OnRender();

	EDXGui::BeginFrame();
	EDXGui::BeginDialog(LayoutStrategy::DockRight);
	{
		EDXGui::Text("Image Res: %i, %i", jobDesc->ImageWidth, jobDesc->ImageHeight);
		EDXGui::Text("Samples per Pixel: %i", gpRenderer->GetFilm() ? gpRenderer->GetFilm()->GetSampleCount() : 0);
		EDXGui::Text("(%.2f, %.2f, %.2f)", gCursorColor.r, gCursorColor.g, gCursorColor.b);
		if (EDXGui::Button(!gRendering ? "Render" : "Stop Rendering"))
		{
			gRendering = !gRendering;
			if (gRendering)
			{
				gpRenderer->InitComponent();
				gpRenderer->SetCameraParams(gpPreview->GetCamera().GetCameraParams());
				gpRenderer->QueueRenderTasks();
			}
			else
			{
				gpRenderer->StopRenderTasks();
			}
		}
		if (EDXGui::Button("Save Image"))
		{
			char name[256];
			char directory[MAX_PATH];
			sprintf_s(directory, MAX_PATH, "%s../../Media", Application::GetBaseDirectory());
			sprintf_s(name, "%sEDXRay_%i.bmp", directory, time(0));
			Bitmap::SaveBitmapFile(name, (float*)gpRenderer->GetFilm()->GetPixelBuffer(), jobDesc->ImageWidth, jobDesc->ImageHeight);
		}

		static bool showRenderSettings = true;
		if (EDXGui::CollapsingHeader("Render Settings", showRenderSettings))
		{
			ComboBoxItem integratoriItems[] = {
					{ 0, "Direct Lighting" },
					{ 1, "Path Tracing" },
					{ 2, "BD Path Tracing" },
					{ 3, "Multiplexed MLT" },
					{ 4, "Stochastic PPM" }
			};
			EDXGui::ComboBox("Integrator", integratoriItems, 5, (int&)jobDesc->IntegratorType);

			static int sampler = 0;
			ComboBoxItem samplerItems[] = {
					{ 0, "Random" },
					{ 1, "Sobol" },
					{ 2, "Metroplis" }
			};
			EDXGui::ComboBox("Sampler", samplerItems, 3, (int&)jobDesc->SamplerType);

			static int filter = 0;
			ComboBoxItem filteriItems[] = {
					{ 0, "Box" },
					{ 1, "Gaussion" },
					{ 2, "Mitchell Netravali" }
			};
			EDXGui::ComboBox("Filter", filteriItems, 3, (int&)jobDesc->FilterType);

			EDXGui::InputDigit((int&)jobDesc->MaxPathLength, "Max Length");
			EDXGui::InputDigit((int&)jobDesc->SamplesPerPixel, "Max Samples");
			EDXGui::CheckBox("Adaptive Sampling", jobDesc->AdaptiveSample);
			EDXGui::CheckBox("Use RHF", jobDesc->UseRHF);

			EDXGui::CloseHeaderSection();
		}

		static bool showCameraSettings = true;
		if (EDXGui::CollapsingHeader("Camera Settings", showCameraSettings))
		{
			EDXGui::Slider<float>("Lens Radius", &gpPreview->GetCamera().mLensRadius, 0.0f, 0.5f);
			EDXGui::CheckBox("Set Focus Distance", gpPreview->mSetFocusDistance);
			EDXGui::CheckBox("Lock Camera", gpPreview->mLockCameraMovement);

			EDXGui::CloseHeaderSection();
		}

		static bool showSceneSettings = true;
		static bool useSkyLight = false;
		static float turbidity = 3.0f;
		static Color groundAlbedo = Color(0.2f);
		static float envLightRotation = 0.0f;
		static float envLightScale = 1.0f;
		if (EDXGui::CollapsingHeader("Scene", showSceneSettings))
		{
			if (EDXGui::Button("Environment Light"))
			{
				if (!useSkyLight)
				{
					char filePath[MAX_PATH];
					char directory[MAX_PATH];
					sprintf_s(directory, MAX_PATH, "%s../../Media", Application::GetBaseDirectory());
					if (Application::GetMainWindow()->OpenFileDialog(directory, "", "", filePath))
					{
						gpRenderer->GetScene()->AddLight(new EnvironmentLight(filePath, gpRenderer->GetScene().Ptr(), 1.0f, envLightRotation));
					}
				}
				else
				{
					gpRenderer->GetScene()->AddLight(new EnvironmentLight(Color(turbidity), groundAlbedo, 40.0f,
						gpRenderer->GetScene().Ptr(), envLightRotation));
				}
			}

			EDXGui::CheckBox("Use Sky Light", useSkyLight);
			if (useSkyLight)
			{
				EDXGui::Slider<float>("Turbidity", &turbidity, 1.0f, 10.0f);
				EDXGui::ColorSlider(&groundAlbedo);
			}

			if (EDXGui::Slider<float>("Env Light Rotation", &envLightRotation, -float(Math::EDX_PI), float(Math::EDX_PI)))
			{
				dynamic_cast<const EnvironmentLight*>(gpRenderer->GetScene()->GetEnvironmentMap())->SetRotation(envLightRotation);
			}
			if (EDXGui::Slider<float>("Env Light Scaling", &envLightScale, 0.0f, 5.0f))
			{
				dynamic_cast<const EnvironmentLight*>(gpRenderer->GetScene()->GetEnvironmentMap())->SetScaling(envLightScale);
			}

			EDXGui::CloseHeaderSection();
		}

		static bool showRHF = true;
		if (jobDesc->UseRHF && EDXGui::CollapsingHeader("RHF Denoise", showRHF))
		{
			if (EDXGui::Button("Denoise"))
			{
				gpRenderer->StopRenderTasks();
				gpRenderer->GetFilm()->Denoise();
			}
			EDXGui::CloseHeaderSection();
		}
	}
	EDXGui::EndDialog();

	if (gpPreview->GetPickedPrimId() != -1 && !gRendering)
	{
		EDXGui::BeginDialog(LayoutStrategy::Floating);
		{
			EDXGui::Text("Material Editor");
			ComboBoxItem items[] = {
				0, "Lambertian",
				1, "Smooth Conductor",
				2, "Smooth Dielectric",
				3, "Rough Conductor",
				4, "Rough Dielectric",
				5, "Disney",
			};

			const auto primId = gpPreview->GetPickedPrimId();
			const auto triId = gpPreview->GetPickedTriangleId();
			auto prim = gpRenderer->GetScene()->GetPrimitives()[primId].Ptr();
			auto previewMesh = gpPreview->GetMesh(primId);

			static BSDFType bsdfType;
			bsdfType = prim->GetBSDF(triId)->GetBSDFType();
			EDXGui::ComboBox("Materials:", items, 6, (int&)bsdfType);

			if (bsdfType != prim->GetBSDF(triId)->GetBSDFType())
				prim->SetBSDF(bsdfType, triId);

			auto pBsdf = prim->GetBSDF(triId);
			for (auto i = 0; i < pBsdf->GetParameterCount(); i++)
			{
				string name = pBsdf->GetParameterName(i);
				Parameter param = pBsdf->GetParameter(name);
				switch (param.Type)
				{
				case Parameter::Float:
				{
					EDXGui::Slider(name.c_str(), &param.Value, param.Min, param.Max);
					pBsdf->SetParameter(name, param);
					if (name == "Roughness" && EDXGui::Button("Roughness Map"))
					{
						char filePath[MAX_PATH];
						char directory[MAX_PATH];
						sprintf_s(directory, MAX_PATH, "%s../../Media", Application::GetBaseDirectory());
						if (Application::GetMainWindow()->OpenFileDialog(directory, "", "", filePath))
						{
							strcpy_s(param.TexPath, MAX_PATH, filePath);
							param.Type = Parameter::TextureMap;
							pBsdf->SetParameter("Roughness", param);
						}
					}

					break;
				}
				case Parameter::Color:
				{
					if (EDXGui::Button("Texture"))
					{
						char filePath[MAX_PATH];
						char directory[MAX_PATH];
						sprintf_s(directory, MAX_PATH, "%s../../Media", Application::GetBaseDirectory());
						if (Application::GetMainWindow()->OpenFileDialog(directory, "", "", filePath))
						{
							strcpy_s(param.TexPath, MAX_PATH, filePath);
							pBsdf->SetParameter("TextureMap", param);
							previewMesh->SetTexture(triId, param.TexPath);
						}
					}
					else
					{
						Color color = Color(param.R, param.G, param.B);
						EDXGui::ColorSlider(&color);
						param.R = color.r;
						param.G = color.g;
						param.B = color.b;
						pBsdf->SetParameter(name, param);
						previewMesh->SetTexture(triId, nullptr);
					}
					break;
				}
				case Parameter::TextureMap:
					if (EDXGui::Button("Texture"))
					{
						char filePath[MAX_PATH];
						char directory[MAX_PATH];
						sprintf_s(directory, MAX_PATH, "%s../../Media", Application::GetBaseDirectory());
						if (Application::GetMainWindow()->OpenFileDialog(directory, "", "", filePath))
						{
							strcpy_s(param.TexPath, MAX_PATH, filePath);
							pBsdf->SetParameter("TextureMap", param);
							previewMesh->SetTexture(triId, param.TexPath);
						}
					}
					else if (EDXGui::Button("Constant Color"))
					{
						param.R = 0.6f; param.G = 0.6f; param.B = 0.6f;
						pBsdf->SetParameter("Color", param);
						previewMesh->SetTexture(triId, nullptr);
					}
					break;
				case Parameter::NormalMap:
					if (EDXGui::Button("Normal Map"))
					{
						char filePath[MAX_PATH];
						char directory[MAX_PATH];
						sprintf_s(directory, MAX_PATH, "%s../../Media", Application::GetBaseDirectory());
						if (Application::GetMainWindow()->OpenFileDialog(directory, "", "", filePath))
						{
							strcpy_s(param.TexPath, MAX_PATH, filePath);
							pBsdf->SetParameter("NormalMap", param);
						}
					}
					break;
				}
			}
		}
		EDXGui::EndDialog();
	}
	EDXGui::EndFrame();
}

void OnResize(Object* pSender, ResizeEventArgs args)
{
	// Set opengl params
	glViewport(0, 0, args.Width, args.Height);

	if (gRendering)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, args.Width, 0, args.Height, -1, 1);

		//gpRenderer->StopRenderTasks();
		//gpRenderer->Resize(args.Width, args.Height);
		//gpRenderer->QueueRenderTasks();
	}
	else
	{
		gpPreview->OnResize(args.Width, args.Height);
		gpRenderer->Resize(args.Width, args.Height);
	}

	EDXGui::Resize(args.Width, args.Height);
}

void OnMouseEvent(Object* pSender, MouseEventArgs args)
{
	if (EDXGui::HandleMouseEvent(args))
		return;

	if (args.Action == MouseAction::Move)
	{
		auto jobDesc = gpRenderer->GetJobDesc();
		gCursorColor = gpRenderer->GetFilm() ?
			gpRenderer->GetFilm()->GetPixelBuffer()[args.x + (jobDesc->ImageHeight - args.y - 1) * jobDesc->ImageWidth] :
			Color::BLACK;

	}

	if (!gRendering)
		gpPreview->HandleMouseMsg(args);
}

void OnKeyboardEvent(Object* pSender, KeyboardEventArgs args)
{
	if (EDXGui::HandleKeyboardEvent(args))
		return;

	if (!gRendering)
		gpPreview->HandleKeyboardMsg(args);
}

void OnRelease(Object* pSender, EventArgs args)
{
	gpRenderer->StopRenderTasks();

	SafeDelete(gpRenderer);
	SafeDelete(gpPreview);
	EDXGui::Release();
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdArgs, int cmdShow)
{
	Application::Init(hInst);
	Window* mainWindow = new GLWindow;
	mainWindow->SetMainLoop(NotifyEvent(OnRender));
	mainWindow->SetInit(NotifyEvent(OnInit));
	mainWindow->SetResize(ResizeEvent(OnResize));
	mainWindow->SetRelease(NotifyEvent(OnRelease));
	mainWindow->SetMouseHandler(MouseEvent(OnMouseEvent));
	mainWindow->SetkeyboardHandler(KeyboardEvent(OnKeyboardEvent));

	mainWindow->Create(L"EDXRay", 1280, 800);

	Application::Run(mainWindow);

	return 0;
}