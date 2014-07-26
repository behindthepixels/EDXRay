#pragma once

#include "EDXPrerequisites.h"

#include "Config.h"
#include "../ForwardDecl.h"
#include "Memory/RefPtr.h"
#include "TaskScheduler.h"
#include "RenderThread.h"

namespace EDX
{
	namespace RayTracer
	{
		class Renderer
		{
		protected:
			RefPtr<Camera>	mpCamera;
			RefPtr<Sampler>	mpSampler;
			RefPtr<Film>	mpFilm;

			RenderJobDesc	mJobDesc;

			TaskScheduler	mTaskScheduler;
			vector<RenderThread> mThreads;

		public:
			Renderer()
			{
			}
			
			void Initialize(const RenderJobDesc& desc);

			void RenderFrame();
			void RenderImage();

			void LaunchRenderThreads();

			const Color* GetFrameBuffer() const;
			const RenderJobDesc GetJobDesc() const { return mJobDesc; }
		};
	}
}