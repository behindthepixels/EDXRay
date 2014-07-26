#include "RenderThread.h"
#include "Renderer.h"

namespace EDX
{
	namespace RayTracer
	{
		void RenderThread::WorkLoop()
		{
			mpRenderer->RenderImage();

			SelfTermnate();
		}
	}
}