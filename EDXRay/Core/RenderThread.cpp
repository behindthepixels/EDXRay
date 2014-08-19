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
			mpRenderer->RenderImage(mId, mRandom, mMemory);

			SelfTermnate();
		}

		void RenderTask::Run(RenderTaskParams* pArgs, int idx)
		{
			pArgs->mpRenderer->RenderImage(pArgs->mId, pArgs->mRandom, pArgs->mMemory);
		}
	}
}