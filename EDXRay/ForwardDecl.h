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
		class Integrator;
		class Light;
		class EnvironmentLight;
		class PointLight;
		class AreaLight;
		class BSDF;
		class BSSRDF;
		class Ray;
		class RayDifferential;
		struct CameraSample;
		struct SampleBuffer;
		struct Sample;
		struct SampleOffsets;
		class Sampler;
		class RayDifferential;
		class Scatter;
		class Intersection;
		class DifferentialGeom;
		class Medium;
		class MediumInterface;
		class MediumScatter;
		class PhaseFunctionHG;
		class Frame;
		class Primitive;
		class TriangleMesh;
		struct BuildTriangle;
		struct BuildVertex;
		class BVH2;
		class BVH4;
		class Triangle4;
		class RenderTask;
		class QueuedRenderTask;
		class BuildTask;
		class QueuedBuildTask;
		enum class BSDFType;

		namespace Sampling
		{
			class Distribution1D;
			class Distribution2D;
		}
	}

	class RandomGen;
	class MemoryPool;
	class Color;
	class Ray;
	class ObjMesh;
}