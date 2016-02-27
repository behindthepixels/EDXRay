#pragma once

#include "EDXPrerequisites.h"

#include "../ForwardDecl.h"

namespace EDX
{
	namespace RayTracer
	{
		class PhaseFunction
		{

		};

		class Medium
		{
		public:
			virtual Color Transmittance(const Ray& ray, Sampler*pSampler) const = 0;
		};

		class MediumInterface
		{
		private:
			const Medium* mpInside;
			const Medium* mpOutside;

		public:
			MediumInterface()
				: mpInside(nullptr)
				, mpOutside(nullptr)
			{
			}

			// MediumInterface Public Methods
			MediumInterface(const Medium* pMedium)
				: mpInside(pMedium)
				, mpOutside(pMedium)
			{
			}

			MediumInterface(const Medium* pInside, const Medium* pOutside)
				: mpInside(pInside)
				, mpOutside(pOutside)
			{
			}

			bool IsMediumTransition() const { return mpInside != mpOutside; }

		};
	}
}