#pragma once

#include "Math/Vec2.h"
#include "Math/Vec3.h"
#include "Math/Vec4.h"
#include "RNG/Random.h"

namespace EDX
{
	namespace RayTracer
	{
		namespace Sampling
		{
			inline void ConcentricSampleDisk(float u1, float u2, float *dx, float *dy)
			{
				float r, theta;
				// Map uniform random numbers to $[-1,1]^2$
				float sx = 2 * u1 - 1;
				float sy = 2 * u2 - 1;

				// Map square to $(r,\theta)$

				// Handle degeneracy at the origin
				if (sx == 0.0 && sy == 0.0)
				{
					*dx = 0.0;
					*dy = 0.0;
					return;
				}
				if (sx >= -sy)
				{
					if (sx > sy)
					{
						// Handle first region of disk
						r = sx;
						if (sy > 0.0) theta = sy / r;
						else          theta = 8.0f + sy / r;
					}
					else
					{
						// Handle second region of disk
						r = sy;
						theta = 2.0f - sx / r;
					}
				}
				else
				{
					if (sx <= sy)
					{
						// Handle third region of disk
						r = -sx;
						theta = 4.0f - sy / r;
					}
					else
					{
						// Handle fourth region of disk
						r = -sy;
						theta = 6.0f + sx / r;
					}
				}
				theta *= float(Math::EDX_PI_4);
				*dx = r * Math::Cos(theta);
				*dy = r * Math::Sin(theta);
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
				return 1.0f / (4.0f * float(Math::EDX_PI));
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

			inline float PowerHeuristic(int nf, float fPdf, int ng, float gPdf)
			{
				float f = nf * fPdf, g = ng * gPdf;
				return (f * f) / (f * f + g * g);
			}
			inline float PdfAtoW(float fPdfA, float fDist, float fCos)
			{
				return fPdfA * fDist * fDist / Math::Abs(fCos);
			}
			inline float PdfWToA(float fPdfW, float fDist, float fCos)
			{
				return fPdfW * Math::Abs(fCos) / (fDist * fDist);
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