#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Primitive.h"
#include "TriangleMesh.h"
#include "Light.h"
#include "Ray.h"
#include "../Integrators/DirectLighting.h"
#include "../Integrators/PathTracing.h"
#include "../Integrators/BidirectionalPathTracing.h"
#include "../Integrators/MultiplexedMLT.h"
#include "../Sampler/RandomSampler.h"
#include "../Sampler/SobolSampler.h"
#include "../Tracer/BVH.h"
#include "../Tracer/BVHBuildTask.h"
#include "Film.h"
#include "DifferentialGeom.h"
#include "Graphics/Color.h"
#include "RenderTask.h"
#include "Config.h"

#include "Graphics/ObjMesh.h"

#include <ppl.h>
using namespace concurrency;

namespace EDX
{
	namespace RayTracer
	{
		Renderer::Renderer()
		{
			// Initialize scene
			mpCamera.Reset(new Camera());
			mpScene.Reset(new Scene);
			QueuedThreadPool::Instance()->Create(GetNumberOfCores());
		}

		Renderer::~Renderer()
		{
			QueuedThreadPool::DeleteInstance();
		}

		void Renderer::InitComponent()
		{
			mpCamera->Init(mJobDesc.CameraParams.Pos,
				mJobDesc.CameraParams.Target,
				mJobDesc.CameraParams.Up,
				mJobDesc.ImageWidth,
				mJobDesc.ImageHeight,
				mJobDesc.CameraParams.CalcFieldOfView(),
				mJobDesc.CameraParams.NearClip,
				mJobDesc.CameraParams.FarClip,
				mJobDesc.CameraParams.CalcCircleOfConfusionRadius(),
				mJobDesc.CameraParams.FocusPlaneDist,
				mJobDesc.CameraParams.Vignette);

			Filter* pFilter;
			switch (mJobDesc.FilterType)
			{
			case EFilterType::Box:
				pFilter = new BoxFilter;
				break;
			case EFilterType::Gaussian:
				pFilter = new GaussianFilter;
				break;
			case EFilterType::MitchellNetravali:
				pFilter = new MitchellNetravaliFilter;
				break;
			default:
				pFilter = new GaussianFilter;
				break;
			}

			mpFilm.Reset(new Film);
			mpFilm->Init(mJobDesc.ImageWidth, mJobDesc.ImageHeight, pFilter);

			switch (mJobDesc.SamplerType)
			{
			case ESamplerType::Random:
				mpSampler.Reset(new RandomSampler);
				break;
			case ESamplerType::Sobol:
				mpSampler.Reset(new SobolSampler(mJobDesc.ImageWidth, mJobDesc.ImageHeight));
				break;
			case ESamplerType::Metropolis:
				mpSampler.Reset(new RandomSampler);
				break;
			}

			switch (mJobDesc.IntegratorType)
			{
			case EIntegratorType::DirectLighting:
				mpIntegrator.Reset(new DirectLightingIntegrator(mJobDesc.MaxPathLength, mJobDesc, mTaskSync));
				break;
			case EIntegratorType::PathTracing:
				mpIntegrator.Reset(new PathTracingIntegrator(mJobDesc.MaxPathLength, mJobDesc, mTaskSync));
				break;
			case EIntegratorType::BidirectionalPathTracing:
				mpIntegrator.Reset(new BidirPathTracingIntegrator(mJobDesc.MaxPathLength, mpCamera.Get(), mpFilm.Get(), mJobDesc, mTaskSync));
				break;
			case EIntegratorType::MultiplexedMLT:
				mpIntegrator.Reset(new MultiplexedMLTIntegrator(mJobDesc.MaxPathLength, mpCamera.Get(), mpFilm.Get(), mJobDesc, mTaskSync));
				break;
			case EIntegratorType::StochasticPPM:
				mpIntegrator.Reset(new BidirPathTracingIntegrator(mJobDesc.MaxPathLength, mpCamera.Get(), mpFilm.Get(), mJobDesc, mTaskSync));
				break;
			}

			//BakeSamples();
			//mpScene->InitAccelerator();

			mTaskSync.Init(mJobDesc.ImageWidth, mJobDesc.ImageHeight);
			mTaskSync.SetAbort(false);
		}

		void Renderer::Resize(int width, int height)
		{
			mJobDesc.ImageWidth = width;
			mJobDesc.ImageHeight = height;

			if (mpCamera)
				mpCamera->Resize(width, height);
			if (mpFilm)
				mpFilm->Resize(width, height);

			mTaskSync.Init(width, height);
		}

		void Renderer::BakeSamples()
		{
		}

		void Renderer::QueueRenderTasks()
		{
			mpFilm->Clear();
			mTaskSync.SetAbort(false);

			mTask = MakeUnique<QueuedRenderTask>(this, 0);
			QueuedThreadPool::Instance()->AddQueuedWork(mTask.Get());
		}

		void Renderer::StopRenderTasks()
		{
			mTaskSync.SetAbort(true);
			QueuedThreadPool::Instance()->JoinAllThreads();
			mTask.Reset();
		}

		void Renderer::SetJobDesc(const RenderJobDesc& jobDesc)
		{
			mJobDesc = jobDesc;

			mpCamera->Init(mJobDesc.CameraParams.Pos,
				mJobDesc.CameraParams.Target,
				mJobDesc.CameraParams.Up,
				mJobDesc.ImageWidth,
				mJobDesc.ImageHeight,
				mJobDesc.CameraParams.CalcFieldOfView(),
				mJobDesc.CameraParams.NearClip,
				mJobDesc.CameraParams.FarClip,
				mJobDesc.CameraParams.CalcCircleOfConfusionRadius(),
				mJobDesc.CameraParams.FocusPlaneDist);
		}
	}
}