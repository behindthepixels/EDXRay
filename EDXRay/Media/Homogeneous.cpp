#include "Homogeneous.h"
#include "../Core/Sampler.h"
#include "../Core/DifferentialGeom.h"
#include "../Core/Ray.h"
#include "Math/EDXMath.h"

namespace EDX
{
	namespace RayTracer
	{
		Color HomogeneousMedium::Transmittance(const Ray& ray, Sampler*pSampler) const
		{
			return Color(Math::Exp((-1.0f * mSigmaT) * ray.mMax));
		}

		Color HomogeneousMedium::Sample(const Ray& ray, Sampler* pSampler, MediumScatter* pMS) const
		{
			// Sample the color channel
			int channel = Math::Min(pSampler->Get1D() * 3, 2);

			// Sample distance along the ray
			float sampledDist = -Math::Log(1 - pSampler->Get1D()) / mSigmaT[channel];
			float dist = Math::Min(sampledDist, ray.mMax);
			bool sampledMedium = sampledDist < ray.mMax;

			if (sampledMedium)
				*pMS = MediumScatter(ray.CalcPoint(dist), mpPhaseFunc.Get(), MediumInterface(ray.mpMedium));

			Vector3 transmittance = Math::Exp((-1.0f * mSigmaT) * dist);
			Vector3 density = sampledMedium ? (mSigmaT * transmittance) : transmittance;

			float pdf = 0.0f;
			for (auto i = 0; i < 3; i++)
			{
				pdf += density[i];
			}
			pdf /= 3.0f;

			return sampledMedium ? Color(transmittance * mSigmaS / pdf) : Color(transmittance / pdf);
		}
	}
}