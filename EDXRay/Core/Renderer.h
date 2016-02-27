#pragma once

#include "EDXPrerequisites.h"

#include "Config.h"
#include "../ForwardDecl.h"
#include "Memory/RefPtr.h"
#include "TaskSynchronizer.h"

namespace EDX
{
	namespace RayTracer
	{
		class Renderer
		{
		protected:
			RefPtr<Camera>	mpCamera;
			RefPtr<Scene>	mpScene;
			RefPtr<Integrator> mpIntegrator;
			RefPtr<Sampler>	mpSampler;
			RefPtr<Film>	mpFilm;
			RefPtr<SampleBuffer> mpSampleBuf;

			RenderJobDesc	mJobDesc;

			// Tile-based multi-threading
			TaskSynchronizer mTaskSync;
			vector<RefPtr<RenderTask>> mTasks;

		public:
			Renderer();
			~Renderer();

			void Initialize();
			void InitComponent();

			void Resize(int width, int height);

			void RenderFrame(Sampler* pSampler, RandomGen& random, MemoryArena& memory);
			void RenderImage(int threadId, RandomGen& random, MemoryArena& memory);

			void BakeSamples();

			void QueueRenderTasks();
			void StopRenderTasks();

			Film* GetFilm();
			RenderJobDesc* GetJobDesc() { return &mJobDesc; }
			RefPtr<Scene> GetScene() { return mpScene; }
			RefPtr<Camera> GetCamera() { return mpCamera; }
			void SetJobDesc(const RenderJobDesc& jobDesc);
		};
	}
}