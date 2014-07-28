#pragma once

namespace EDX
{
	namespace RayTracer
	{
		class Renderer;
		struct RenderJobDesc;
		class Camera;
		class Scene;
		class Film;

		struct CameraSample;
		struct Sample;
		class Sampler;
		class RayDifferential;
		class Intersection;
		class DifferentialGeom;
		class Primitive;
		class BVH4;
	}

	class RandomGen;
	class Color;
	class Ray;
}