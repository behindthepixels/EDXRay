#pragma once

#include "EDXPrerequisites.h"

namespace EDX
{
	namespace RayTracer
	{
		struct CameraSample
		{
			float imageX, imageY;
			float lensU, lensV;
			float time;
		};

		struct Sample : public CameraSample
		{

		};

		class Sampler
		{
		public:
			int 
		};
	}
}