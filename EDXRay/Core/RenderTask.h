#pragma once

#include "EDXPrerequisites.h"
#include "Renderer.h"
#include "Integrator.h"

#include "Windows/Threading.h"

#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		class QueuedRenderTask : public QueuedWork
		{
		public:
			QueuedRenderTask(Renderer* pRenderer, const int idx)
				: mpRenderer(pRenderer)
				, mIndex(idx)
			{
			}

			void DoThreadedWork()
			{
				mpRenderer->GetIntegrator()->Render(mpRenderer->GetScene(), mpRenderer->GetCamera(), mpRenderer->GetSampler(), mpRenderer->GetFilm());
			}

			void Abandon()
			{

			}

		private:
			Renderer*	mpRenderer;
			int			mIndex;
		};
	}
}
