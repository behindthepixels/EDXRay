#include "Primitive.h"
#include "TriangleMesh.h"

namespace EDX
{
	namespace RayTracer
	{
		Primitive::Primitive(TriangleMesh* pMesh)
			: mpMesh(pMesh)
		{
		}
	}
}