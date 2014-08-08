#pragma once

#include "../ForwardDecl.h"

#include "Memory/RefPtr.h"

namespace EDX
{
	namespace RayTracer
	{
		class Primitive
		{
		private:
			RefPtr<TriangleMesh> mpMesh;

		public:
			Primitive(TriangleMesh* pMesh);

		public:
			const TriangleMesh* GetMesh() const
			{
				return mpMesh.Ptr();
			}
		};
	}
}