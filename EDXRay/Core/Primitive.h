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
			bool Intersect(const Ray& ray, Intersection* pIsect) const;
			bool Occluded(const Ray& ray) const
			{

			}

			const TriangleMesh* GetMesh() const
			{
				return mpMesh.Ptr();
			}
		};
	}
}