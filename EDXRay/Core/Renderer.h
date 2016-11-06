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
			UniquePtr<SampleBuffer> mpSampleBuf;

			RenderJobDesc	mJobDesc;

			// Tile-based multi-threading
			TaskSynchronizer mTaskSync;
			Array<UniquePtr<QueuedRenderTask>> mTasks;

			// Timer
			Timer mTimer;
			float mFrameTime;

		public:
			Renderer();
			~Renderer();

			void InitComponent();

			void Resize(int width, int height);

			void RenderFrame(Sampler* pSampler, RandomGen& random, MemoryPool& memory);
			void RenderImage(int threadId, RandomGen& random, MemoryPool& memory);

			void BakeSamples();

			void QueueRenderTasks();
			void StopRenderTasks();

			Film* GetFilm();
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
			float GetFrameTime() const
			{
				return mFrameTime;
			}

			void SetJobDesc(const RenderJobDesc& jobDesc);

		};
	}
}