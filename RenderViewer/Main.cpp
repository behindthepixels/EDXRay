
#include "Windows/Window.h"
#include "Windows/Application.h"
#include "Memory/Memory.h"

#include <gl/GL.h>

#include "Core/Renderer.h"
#include "Core/Scene.h"
#include "Core/Primitive.h"
#include "Core/TriangleMesh.h"
#include "Lights/PointLight.h"

#include "ScenePreviewer.h"

using namespace EDX;
using namespace EDX::RayTracer;

RefPtr<Renderer> gpRenderer = nullptr;
Previewer gPreview;
bool gRendering = false;

void OnInit(Object* pSender, EventArgs args)
{
	glClearColor(0.4f, 0.5f, 0.65f, 1.0f);

	gpRenderer = new Renderer;

	RenderJobDesc desc;
	desc.ImageWidth = 1280;
	desc.ImageHeight = 800;
	desc.SamplesPerPixel = 16;
	desc.CameraParams.FieldOfView = 65;
	gpRenderer->Initialize(desc);

	Scene* pScene = gpRenderer->GetScene().Ptr();
	TriangleMesh* pMesh = new TriangleMesh;
	pMesh->LoadSphere(1.0f, 128, 128, Vector3(-2.0f, 1.0f, 10.5f));
	TriangleMesh* pMesh2 = new TriangleMesh;
	pMesh2->LoadSphere(1.0f, 128, 128, Vector3(2.0f, 1.0f, 10.5f));

	pScene->AddPrimitive(new Primitive(pMesh));
	pScene->AddPrimitive(new Primitive(pMesh2));
	pScene->AddLight(new PointLight(Vector3(0.0f, 10.0f, 10.5f), Color(300.0f)));
	pScene->InitAccelerator();

	gPreview.Initialize(*pScene, 1280, 800, 65);
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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, args.Width, 0, args.Height, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	gPreview.OnResize(args.Width, args.Height);
}

void OnMouseEvent(Object* pSender, MouseEventArgs args)
{
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
			gPreview.OnResize(Application::GetMainWindow()->GetWindowWidth(), Application::GetMainWindow()->GetWindowHeight());
		}
		break;
	}

	gPreview.GetCamera().HandleKeyboardMsg(args);
}

void OnRelease(Object* pSender, EventArgs args)
{
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

	mainWindow->Create(L"EDXRay", 1280, 800);

	Application::Run(mainWindow);

	return 0;
}