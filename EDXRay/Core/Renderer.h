#pragma once

#include "../ForwardDecl.h"
#include "Memory/RefPtr.h"

namespace EDX
{
	namespace RayTracer
	{
		class Renderer
		{
		public:
			RefPtr<Camera>			mpCamera;
			RefPtr<SobolSampler>	mpSampler;
			RefPtr<Film>			mpFilm;
		};
	}
}