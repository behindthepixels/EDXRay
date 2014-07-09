#pragma once

#include "EDXPrerequisites.h"

#include "../ForwardDecl.h"
#include "Memory/RefPtr.h"

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

		public:
			Renderer()
			{
			}
			
			void Initialize(const RenderJobParams& params);

			void RenderFrame();
			void RenderImage();
		};
	}
}