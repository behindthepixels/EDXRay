#pragma once

#include "EDXPrerequisites.h"
#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		// Sobol Matrix Declarations
		static const int NumSobolDimensions = 1024;
		static const int SobolMatrixSize = 52;
		extern const uint32 SobolMatrices32[NumSobolDimensions * SobolMatrixSize];
		extern const uint64 SobolMatrices64[NumSobolDimensions * SobolMatrixSize];
		extern const uint64 VdCSobolMatrices[][SobolMatrixSize];
		extern const uint64 VdCSobolMatricesInv[][SobolMatrixSize];
	}
}