#pragma once

#include "Math/Vector.h"
#include "RNG/Random.h"
#include "Memory/Array.h"

namespace EDX
{
	namespace RayTracer
	{
		namespace Sampling
		{
			class Distribution1D
			{
			private:
				Array1f mPDF;
				Array1f mCDF;
				int mSize;
				float mIntegralVal;

			public:
				Distribution1D(float* pFunc, int size)
				{
					mSize = size;
					mPDF.Init(size);
					mCDF.Init(size + 1);

					mPDF.SetData(pFunc);

					float invSize = 1.0f / float(size);
					mCDF[0] = 0.0f;
					for (auto i = 0; i < mCDF.LinearSize(); i++)
						mCDF[i] = mCDF[i - 1] + mPDF[i - 1] * invSize;

					mIntegralVal = mCDF[size];
					for (auto i = 0; i < mCDF.LinearSize(); i++)
						mCDF[i] /= mIntegralVal;
				}

				int SampleDiscrete(float u, float* pPdf)
				{
					float *ptr = std::upper_bound(mCDF.ModifiableData(), mCDF.ModifiableData() + mCDF.LinearSize(), u);
					int offset = Math::Max(0, int(ptr - mCDF.Data() - 1));
					if (pPdf)
						*pPdf = mPDF[offset] / (mIntegralVal * mSize);

					return offset;
				}
			};

			inline void ConcentricSampleDisk(float u1, float u2, float *dx, float *dy)
			{
				float r1 = 2.0f * u1 - 1.0f;
				float r2 = 2.0f * u2 - 1.0f;

				/* Modified concencric map code with less branching (by Dave Cline), see
				http://psgraphics.blogspot.ch/2011/01/improved-code-for-concentric-map.html */

				float phi, r;
				if (r1 == 0 && r2 == 0)
					r = phi = 0;

				if (r1*r1 > r2*r2)
				{
					r = r1;
					phi = float(Math::EDX_PI_4) * (r2 / r1);
				}
				else {
					r = r2;
					phi = float(Math::EDX_PI_2) - (r1 / r2) * float(Math::EDX_PI_4);
				}

				*dx = r * Math::Cos(phi);
				*dy = r * Math::Sin(phi);
			}
			inline void UniformSampleTriangle(float u1, float u2, float* u, float* v)
			{
				float su1 = Math::Sqrt(u1);
				*u = 1.f - su1;
				*v = u2 * su1;
			}
			inline Vector3 CosineSampleHemisphere(float u1, float u2)
			{
				Vector3 ret;
				ConcentricSampleDisk(u1, u2, &ret.x, &ret.y);
				ret.z = Math::Sqrt(Math::Max(0.f, 1.f - ret.x * ret.x - ret.y * ret.y));
				return ret;
			}
			inline Vector3 UniformSampleSphere(float u1, float u2)
			{
				float z = 1.f - 2.f * u1;
				float r = sqrtf(max(0.f, 1.f - z*z));
				float phi = 2.0f * float(Math::EDX_PI) * u2;
				float x = r * cosf(phi);
				float y = r * sinf(phi);
				return Vector3(x, y, z);
			}
			inline Vector3 UniformSampleCone(float u1, float u2, float costhetamax,
				const Vector3 &x, const Vector3 &y, const Vector3 &z)
			{
				float costheta = Math::Lerp(costhetamax, 1.0f, u1);
				float sintheta = sqrtf(1.f - costheta * costheta);
				float phi = 2.0f * float(Math::EDX_PI) * u2;
				return cosf(phi) * sintheta * x + sinf(phi) * sintheta * y +
					costheta * z;
			}
			inline float UniformSpherePDF()
			{
				return Math::EDX_INV_4PI;
			}
			inline float UniformHemispherePDF()
			{
				return Math::EDX_INV_2PI;
			}
			inline float CosineHemispherePDF(float cos)
			{
				return Math::Max(0.0f, cos) * float(Math::EDX_INV_PI);
			}
			inline float CosineHemispherePDF(const Vector3 norm, const Vector3& vec)
			{
				return Math::Max(0.0f, Math::Dot(norm, vec)) * float(Math::EDX_INV_PI);
			}
			inline float UniformConePDF(float cosThetaMax)
			{
				return 1.0f / (2.0f * float(Math::EDX_PI) * (1.0f - cosThetaMax));
			}

			inline float PowerHeuristic(int nf, float pdf, int ng, float gPdf)
			{
				float f = nf * pdf, g = ng * gPdf;
				return (f * f) / (f * f + g * g);
			}
			inline float PdfAtoW(float pdfA, float dist, float cos)
			{
				return pdfA * dist * dist / Math::Abs(cos);
			}
			inline float PdfWToA(float pdfW, float dist, float cos)
			{
				return pdfW * Math::Abs(cos) / (dist * dist);
			}

			inline float VanDerCorput(uint n, uint scramble)
			{
				// Reverse the bits of n
				n = (n << 16) | (n >> 16);
				n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
				n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
				n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
				n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);

				n ^= scramble;

				return Math::Min(((n >> 8) & 0xffffff) / float(1 << 24), 0.9999999403953552f);
			}

			inline float Sobol(uint n, uint uiScramble)
			{
				for (uint v = 1 << 31; n != 0; n >>= 1, v ^= v >> 1)
				{
					if (n & 0x1)
					{
						uiScramble ^= v;
					}
				}
				return Math::Min(((uiScramble >> 8) & 0xffffff) / float(1 << 24), 0.9999999403953552f);
			}
		}
	}
}