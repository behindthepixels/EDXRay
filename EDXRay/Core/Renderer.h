#pragma once

#include "EDXPrerequisites.h"

#include "Config.h"
#include "TaskSynchronizer.h"
#include "../ForwardDecl.h"

#include "Core/SmartPointer.h"
#include "Windows/Timer.h"

namespace EDX
{
	namespace RayTracer
	{
		class Renderer
		{
		protected:
			UniquePtr<Camera>	mpCamera;
			UniquePtr<Scene>	mpScene;
			UniquePtr<Integrator> mpIntegrator;
			UniquePtr<Sampler>	mpSampler;
			UniquePtr<Film>	mpFilm;

			RenderJobDesc	mJobDesc;

			// Tile-based multi-threading
			TaskSynchronizer mTaskSync;
			UniquePtr<QueuedRenderTask> mTask;

			// Timer
			Timer mTimer;
			float mFrameTime;

		public:
			Renderer();
			~Renderer();

			void InitComponent();
			void Resize(int width, int height);

			void BakeSamples();

			void QueueRenderTasks();
			void StopRenderTasks();

			RenderJobDesc* GetJobDesc()
			{
				return &mJobDesc; 
			}
			Scene* GetScene()
			{
				return mpScene.Get();
			}
			Camera* GetCamera()
			{
				return mpCamera.Get();
			}
			Sampler* GetSampler()
			{
				return mpSampler.Get();
			}
			Film* GetFilm()
			{
				return mpFilm.Get();
			}
			Integrator* GetIntegrator()
			{
				return mpIntegrator.Get();
			}

			float GetFrameTime() const
			{
				return mFrameTime;
			}

			void SetJobDesc(const RenderJobDesc& jobDesc);

		};
	}
}