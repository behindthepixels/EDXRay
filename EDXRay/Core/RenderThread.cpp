#include "RenderThread.h"
#include "Renderer.h"

namespace EDX
{
	namespace RayTracer
	{
		void RenderThread::Init(Renderer* pRenderer, uint id)
		{
			mpRenderer = pRenderer;
			mId = id;
		}

		void RenderThread::WorkLoop()
		{
			mpRenderer->RenderImage(mId);

			SelfTermnate();
		}
	}
}