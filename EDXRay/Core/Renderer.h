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

			RenderJobDesc	mJobDesc;

			// Tile-based multi-threading
			TaskSynchronizer mTaskSync;
			vector<RefPtr<RenderTask>> mTasks;

		public:
			Renderer()
			{
			}
			~Renderer();
			
			void Initialize(const RenderJobDesc& desc);

			void RenderFrame(RandomGen& random, MemoryArena& memory);
			void RenderImage(int threadId, RandomGen& random, MemoryArena& memory);

			void QueueRenderTasks();
			void StopRenderTasks();

			const Color* GetFrameBuffer() const;
			const RenderJobDesc GetJobDesc() const { return mJobDesc; }
			RefPtr<Scene> GetScene() { return mpScene; }
			void SetCameraParams(const CameraParameters& params);
		};
	}
}