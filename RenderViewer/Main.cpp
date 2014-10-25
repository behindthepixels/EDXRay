
#include "Windows/Window.h"
#include "Windows/Application.h"
#include "Memory/Memory.h"

#include <gl/GL.h>

#include "Core/Renderer.h"
#include "Core/Scene.h"
#include "Core/Primitive.h"
#include "Core/TriangleMesh.h"
#include "Lights/PointLight.h"
#include "Lights/DirectionalLight.h"
#include "Lights/EnvironmentalLight.h"
#include "Core/BSDF.h"

#include "ScenePreviewer.h"

using namespace EDX;
using namespace EDX::RayTracer;

int gImageWidth = 1280;
int gImageHeight = 800;

RefPtr<Renderer> gpRenderer = nullptr;
Previewer gPreview;
bool gRendering = false;

void OnInit(Object* pSender, EventArgs args)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	gpRenderer = new Renderer;

	RenderJobDesc desc;
	desc.ImageWidth = gImageWidth;
	desc.ImageHeight = gImageHeight;
	desc.SamplesPerPixel = 8192 * 4;
	desc.CameraParams.FieldOfView = 65;
	gpRenderer->Initialize(desc);

	Scene* pScene = gpRenderer->GetScene().Ptr();
	Primitive* pMesh = new Primitive;
	//pMesh->LoadMesh("../../Media/sponza/sponza.obj", BSDFType::Diffuse, Vector3(0, 0, 0), 0.01f * Vector3::UNIT_SCALE, Vector3(0, 90, 0));
	//pMesh->LoadMesh("../../Media/crytek-sponza/sponza.obj", BSDFType::Diffuse, Vector3(0, 0, 0), 0.01f * Vector3::UNIT_SCALE, Vector3(0, 90, 0));
	//pMesh->LoadMesh("../../Media/cornell-box/cornellbox.obj", BSDFType::Diffuse, Vector3(0, 0, 0), 3.0f * Vector3::UNIT_SCALE, Vector3(0, 180, 0));
	pMesh->LoadMesh("../../Media/san-miguel/san-miguel.obj", BSDFType::Diffuse, Vector3(-5, 0, -10), Vector3::UNIT_SCALE, Vector3(0, 0, 0));
	//pMesh->LoadSphere(1.0f, BSDFType::Diffuse, 128, 128, Vector3(0.0f, 1.0f, 10.5f));

	pScene->AddPrimitive(pMesh);
	pScene->AddLight(new DirectionalLight(Vector3(2.5f, 10.0f, 1.0f), Color(13.0f)));
	pScene->AddLight(new EnvironmentalLight(100 * Color(0.66, 0.66, 0.7)));
	pScene->InitAccelerator();
	gpRenderer->BakeSamples();

	gPreview.Initialize(*pScene, desc.ImageWidth, desc.ImageHeight, desc.CameraParams.FieldOfView);
}

void OnRender(Object* pSender, EventArgs args)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (gRendering)
	{
		glRasterPos3f(0.0f, 0.0f, 0.0f);
		glDrawPixels(gpRenderer->GetJobDesc().ImageWidth, gpRenderer->GetJobDesc().ImageHeight, GL_RGBA, GL_FLOAT, (float*)gpRenderer->GetFrameBuffer());
	}
	else
		gPreview.OnRender();
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
		gPreview.OnResize(args.Width, args.Height);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void OnMouseEvent(Object* pSender, MouseEventArgs args)
{
	if (!gRendering)
		gPreview.GetCamera().HandleMouseMsg(args);
}

void OnKeyboardEvent(Object* pSender, KeyboardEventArgs args)
{
	switch (args.key)
	{
	case 'R':
		gRendering = !gRendering;
		if (gRendering)
		{
			gpRenderer->SetCameraParams(gPreview.GetCamera().GetCameraParams());

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, Application::GetMainWindow()->GetWindowWidth(), 0, Application::GetMainWindow()->GetWindowHeight(), -1, 1);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			gpRenderer->QueueRenderTasks();
		}
		else
		{
			gpRenderer->StopRenderTasks();
			gPreview.OnResize(Application::GetMainWindow()->GetWindowWidth(), Application::GetMainWindow()->GetWindowHeight());
		}
		break;
	}

	if (!gRendering)
		gPreview.GetCamera().HandleKeyboardMsg(args);
}

void OnRelease(Object* pSender, EventArgs args)
{
	gpRenderer->StopRenderTasks();
	gpRenderer.Dereference();
	gPreview.~Previewer();
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