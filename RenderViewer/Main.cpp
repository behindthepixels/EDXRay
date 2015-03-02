
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
#include "Lights/EnvironmentalLight.h"
#include "Core/BSDF.h"
#include "BSDFs/RoughConductor.h"
#include "BSDFs/RoughDielectric.h"

#include "ScenePreviewer.h"

#include "Graphics/EDXGui.h"
#include "Windows/Bitmap.h"

using namespace EDX;
using namespace EDX::RayTracer;
using namespace EDX::GUI;

int gImageWidth = 800;
int gImageHeight = 600;

Renderer*	gpRenderer = nullptr;
Previewer*	gpPreview = nullptr;
bool gRendering = false;
Color gCursorColor;

void GUIEvent(Object* pObject, EventArgs args);

void OnInit(Object* pSender, EventArgs args)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	gpRenderer = new Renderer;

	RenderJobDesc desc;
	desc.ImageWidth = gImageWidth;
	desc.ImageHeight = gImageHeight;
	desc.SamplesPerPixel = 4096;
	desc.CameraParams.FieldOfView = 45;
	desc.CameraParams.Pos = Vector3(0, 3, 5);
	desc.CameraParams.Target = Vector3(0, 3, 0);
	gpRenderer->Initialize(desc);

	Scene* pScene = gpRenderer->GetScene().Ptr();
	//Primitive* pMesh = new Primitive;
	Primitive* pMesh2 = new Primitive;
	Primitive* pMesh3 = new Primitive;
	Primitive* pMesh4 = new Primitive;
	//pMesh->LoadMesh("../../Media/sponza/sponza.obj", Vector3(0, 0, 0), 0.01f * Vector3::UNIT_SCALE, Vector3(0, 90, 0));
	//pMesh->LoadMesh("../../Media/crytek-sponza/sponza.obj", Vector3(0, 0, 0), 0.01f * Vector3::UNIT_SCALE, Vector3(0, 90, 0));
	//pMesh->LoadMesh("../../Media/cornell-box/cornellbox.obj", Vector3(0, 0, 0), 3.0f * Vector3::UNIT_SCALE, Vector3(0, 180, 0));
	//pMesh->LoadMesh("../../Media/san-miguel/san-miguel.obj", Vector3(-5, 0, -10), Vector3::UNIT_SCALE, Vector3(0, 0, 0));
	//pMesh->LoadSphere(1.0f, BSDFType::RoughConductor, Color::WHITE, 128, 128, Vector3(0.0f, 3.0f, 0.0f));
	pMesh2->LoadMesh("../../Media/venusm.obj", BSDFType::RoughDielectric, Color(0.6f, 0.27f, 0.27f), Vector3(1.5f, 2.88f, 0.0f), 0.001f * Vector3::UNIT_SCALE, Vector3(0.0f, 0.0f, 0.0f));
	//pMesh3->LoadSphere(1.0f, BSDFType::RoughDielectric, Color::WHITE, 128, 128, Vector3(-2.5f, 3.0f, 0.0f));
	pMesh3->LoadMesh("../../Media/bunny.obj", BSDFType::Diffuse, Color(0.27f, 0.27f, 0.6f), Vector3(-1.5f, 1.5f, 0.0f), 0.16f * Vector3::UNIT_SCALE, Vector3(0.0f, 0.0f, 0.0f));
	pMesh4->LoadPlane(10.0f, BSDFType::Diffuse, Color(0.25f), Color(0.9f, 0.9f, 0.9f), Vector3(0.0f, 2.0f, 0.0f));

	//pScene->AddPrimitive(pMesh);
	pScene->AddPrimitive(pMesh2);
	pScene->AddPrimitive(pMesh3);
	pScene->AddPrimitive(pMesh4);
	//pScene->AddLight(new DirectionalLight(Vector3(2.5f, 10.0f, 1.0f), Color(18.2f)));
	pScene->SetEnvironmentMap(new EnvironmentalLight("../../Media/uffizi-large.hdr"));
	//pScene->SetEnvironmentMap(new EnvironmentalLight(Color(3.0f), Color(0.2f), Math::ToRadians(70.0f)));
	//pScene->AddLight(new PointLight(Vector3(0.0f, 5.5f, 0.0f), Color(20.0f)));

	pScene->InitAccelerator();
	gpRenderer->BakeSamples();

	gpPreview = new Previewer;
	gpPreview->Initialize(*pScene, desc);

	// Initialize UI
	EDXGui::Init();
}

void OnRender(Object* pSender, EventArgs args)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (gRendering)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, Application::GetMainWindow()->GetWindowWidth(), 0, Application::GetMainWindow()->GetWindowHeight(), -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glRasterPos3f(0.0f, 0.0f, 0.0f);
		glDrawPixels(gpRenderer->GetJobDesc().ImageWidth, gpRenderer->GetJobDesc().ImageHeight, GL_RGBA, GL_FLOAT, (float*)gpRenderer->GetFilm()->GetPixelBuffer());
	}
	else
		gpPreview->OnRender();

	EDXGui::BeginFrame();
	EDXGui::BeginDialog(LayoutStrategy::DockRight);
	{
		EDXGui::Text("Image Res: %i, %i", gImageWidth, gImageHeight);
		EDXGui::Text("Samples per Pixel: %i", gpRenderer->GetFilm()->GetSampleCount());
		EDXGui::Text("(%.2f, %.2f, %.2f)", gCursorColor.r, gCursorColor.g, gCursorColor.b);
		if (EDXGui::Button(!gRendering ? "Render" : "Stop Rendering"))
		{
			gRendering = !gRendering;
			if (gRendering)
			{
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
			sprintf_s(name, "EDXRay_%i.bmp", time(0));
			Bitmap::SaveBitmapFile(name, (_byte*)gpRenderer->GetFilm()->GetPixelBuffer(), gImageWidth, gImageHeight);
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
				5, "Principled",
			};

			const auto primId = gpPreview->GetPickedPrimId();
			const auto triId = gpPreview->GetPickedTriangleId();
			auto prim = gpRenderer->GetScene()->GetPrimitives()[primId].Ptr();

			static BSDFType bsdfType;
			bsdfType = prim->GetBSDF(triId)->GetBSDFType();
			EDXGui::ComboBox("Materials:", items, 6, (int&)bsdfType);

			if (bsdfType != prim->GetBSDF(triId)->GetBSDFType())
				prim->SetBSDF(bsdfType, triId);

			auto pBsdf = prim->GetBSDF(triId);
			for (auto i = 0; i < pBsdf->GetParameterCount(); i++)
			{
				float value, min, max;
				string name = pBsdf->GetParameterName(i);
				if (pBsdf->GetParameter(name, &value, &min, &max))
				{
					EDXGui::Slider(name.c_str(), &value, min, max);
					pBsdf->SetParameter(name, value);
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
	}
	else
	{
		gpPreview->OnResize(args.Width, args.Height);
	}

	EDXGui::Resize(args.Width, args.Height);
}

void OnMouseEvent(Object* pSender, MouseEventArgs args)
{
	if (EDXGui::HandleMouseEvent(args))
		return;

	if (args.Action == MouseAction::Move)
	{
		gCursorColor = gpRenderer->GetFilm()->GetPixelBuffer()[args.x + (gImageHeight - args.y - 1) * gImageWidth];
	}

	if (!gRendering)
		gpPreview->HandleMouseMsg(args);
}

void OnKeyboardEvent(Object* pSender, KeyboardEventArgs args)
{
	if (EDXGui::HandleKeyboardEvent(args))
		return;

	if (!gRendering)
		gpPreview->GetCamera().HandleKeyboardMsg(args);
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

	mainWindow->Create(L"EDXRay", gImageWidth, gImageHeight);

	Application::Run(mainWindow);

	return 0;
}