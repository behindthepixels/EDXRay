
#include "Windows/Window.h"
#include "Windows/Application.h"

#include <gl/GL.h>

#include "Core/Renderer.h"

using namespace EDX;
using namespace EDX::RayTracer;

Renderer gRenderer;

void OnInit(Object* pSender, EventArgs args)
{
	glClearColor(0.4f, 0.5f, 0.65f, 1.0f);

	RenderJobDesc desc;
	desc.ImageWidth = 1280;
	desc.ImageHeight = 800;
	desc.SamplesPerPixel = 1;
	desc.CameraParams.FieldOfView = 90;
	gRenderer.Initialize(desc);

	gRenderer.RenderImage();
}

void OnRender(Object* pSender, EventArgs args)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glRasterPos3f(0.0f, 0.0f, 0.0f);
	glDrawPixels(gRenderer.GetJobDesc().ImageWidth, gRenderer.GetJobDesc().ImageHeight, GL_RGBA, GL_FLOAT, (float*)gRenderer.GetFrameBuffer());
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
}

void OnRelease(Object* pSender, EventArgs args)
{
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdArgs, int cmdShow)
{
	Application::Init(hInst);

	Window* mainWindow = new GLWindow;
	mainWindow->SetMainLoop(NotifyEvent(OnRender));
	mainWindow->SetInit(NotifyEvent(OnInit));
	mainWindow->SetResize(ResizeEvent(OnResize));
	mainWindow->SetRelease(NotifyEvent(OnRelease));

	mainWindow->Create(L"EDXRay", 1280, 800);

	Application::Run(mainWindow);

	return 0;
}