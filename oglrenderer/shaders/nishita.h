#ifndef NISHITA_H
#define NISHITA_H

#ifndef GLSL_SHADER
# include "glm/glm.hpp"
# include "GL/glew.h"
# define INOUT_VEC3 vec3 &
# define mat4       glm::mat4
# define min        glm::min
# define max        glm::max
# define vec4       glm::vec4
# define vec3       glm::vec3
# define vec2       glm::vec2
# define uint       uint32_t
# define ivec4      glm::ivec4
#else 
# define INOUT_VEC3 inout vec3
#endif

struct linesample
{
	bool mHasSolutions;
	int  mType;
	vec3 mMin;
	vec3 mMax;
};

bool has_solutions(
	vec3 StartPoint,
	vec3 UnitVector,
	float Radius)
{
	float b = 2 * dot(UnitVector, StartPoint);
	float bsquared = pow(b, 2);
	float c = dot(StartPoint, StartPoint) - pow(Radius, 2);
	float fourac = 4 * c;

	if (bsquared > fourac)
	{
		return true;
	}
	else
	{
		return false;
	}
}


int linetype(
	vec3 StartPoint,
	vec3 UnitVector,
	float Radius)
{
	float b = 2 * dot(UnitVector, StartPoint);
	float bsquared = pow(b, 2);
	float c = dot(StartPoint, StartPoint) - pow(Radius, 2);
	float fourac = 4 * c;

	if (bsquared > fourac)
	{
		float d1 = (-b + pow(bsquared - fourac, 0.5)) / 2;
		float d2 = (-b - pow(bsquared - fourac, 0.5)) / 2;
		if (d1 < 0 && d2 < 0)
		{
			return 2;
		}
		else if (d1 < 0 || d2 < 0)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else {
		return 3;
	}
}


vec3 lmin(
	vec3 StartPoint,
	vec3 UnitVector,
	float Radius)
{
	float b = 2 * dot(UnitVector, StartPoint);
	float bsquared = pow(b, 2);
	float c = dot(StartPoint, StartPoint) - pow(Radius, 2);
	float fourac = 4 * c;

	if (bsquared > fourac)
	{
		float d1 = (-b + pow(bsquared - fourac, 0.5)) / 2;
		float d2 = (-b - pow(bsquared - fourac, 0.5)) / 2;
		return StartPoint + UnitVector * min(d1, d2);
	}
	else
	{
		return vec3(0.0f);
	}
}


vec3 lmax(
	vec3 StartPoint,
	vec3 UnitVector,
	float Radius)
{
	float b = 2 * dot(UnitVector, StartPoint);
	float bsquared = pow(b, 2);
	float c = dot(StartPoint, StartPoint) - pow(Radius, 2);
	float fourac = 4 * c;

	if (bsquared > fourac)
	{
		float d1 = (-b + pow(bsquared - fourac, 0.5)) / 2;
		float d2 = (-b - pow(bsquared - fourac, 0.5)) / 2;
		return StartPoint + UnitVector * max(d1, d2);
	}
	else
	{
		return vec3(0.0f);
	}
}


void nishitaSky(
	float      altitude         /*= 1*/,
	float      rayleighIntensity/*= 20*/,
	float      mieIntensity     /*= 20*/,
	vec3       sunDirFromOrigin /*= vec3(0.0f, 0.0f,1.0f)*/,
	vec3       rayDir           /*= vec3(0.0f, 1.0f, 0.0f)*/,
	INOUT_VEC3 rayleigh,
	INOUT_VEC3 mie,
	INOUT_VEC3 sky)
{
	//Variables:
	float Re = 6360e3f; // Radius of the earth
	float Ra = 6420e3f; // Radius of the atmosphere
	float Hr = 7994.0f; // Rayleigh scale height
	float Hm = 1200.0f; // Mie scale height
	float g = 0.76f;    // Anisotropy term for Mie scattering.

    const vec3 BetaR = vec3(3.8e-6, 13.5e-6, 33.1e-6);
	vec3 BetaM_max = vec3(9.73e-6);
	vec3 BetaM_min = vec3(13.28e-6);
	vec3 BetaM = vec3(21e-6);

	//Clean up inputs:
	vec3 SunDirection = normalize(sunDirFromOrigin);
	vec3 Direction = normalize(vec3(rayDir.x, max(rayDir.y, 0.0f), rayDir.z));

	//Calcualte Rayleigh and mie phases.
	float mu = dot(Direction, SunDirection);
	float phaseR = (3.0f / (16.0f * PI)) * (1.0f + mu * mu);
	float phaseM = (3.0f / (8.0f * PI)) * ((1.0f - g * g) * (1.0f + mu * mu) / ((2.0f + g * g) * pow(1.0f + g * g - 2.0f * g * mu, 1.5f)));

	//Stuff that will eventually help form my output.
	vec3 SumR = vec3(0);
	vec3 SumM = vec3(0);

	//Set up start and end points for my integrals.
	vec3 Pu = vec3(0, 0, 0);
	vec3 Pv = vec3(0, 0, 0);

	//Get start and end point for View Line.
	if (altitude > 0)
	{
		vec3 Pc = vec3(0, Re + altitude, 0);
		Pu = Pc;

		linesample groundline;
		groundline.mHasSolutions = has_solutions(Pc, Direction, Re);
		groundline.mType = linetype(Pc, Direction, Re);
		groundline.mMin = lmin(Pc, Direction, Re);
		groundline.mMax = lmax(Pc, Direction, Re);

		linesample atmosphereline;
		atmosphereline.mHasSolutions = has_solutions(Pc, Direction, Ra);
		atmosphereline.mType = linetype(Pc, Direction, Ra);
		atmosphereline.mMin = lmin(Pc, Direction, Ra);
		atmosphereline.mMax = lmax(Pc, Direction, Ra);

		Pv = atmosphereline.mMax;
		if (atmosphereline.mHasSolutions && atmosphereline.mType == 0)
		{
			//Line starts outside atmoshere, so Pu becomes Line.min
			Pu = atmosphereline.mMin;
		}
		if (groundline.mHasSolutions && groundline.mType == 0)
		{
			//Line hits ground, Pv becomes point at which line hits ground, which will be Line.min
			Pv = groundline.mMin;
		}
	}

	// Create samples along view ray.
	int numSamples = 32;
	int numSamplesL = 8;

	float segmentLength = length(Pu - Pv) / numSamples;
	float opticalDepthR = 0;
	float opticalDepthM = 0;

	for (int i = 0; i < numSamples; i++)
	{
		vec3 Px = Pu + segmentLength * Direction * float(i + 0.5f);
		float sampleHeight = length(Px) - Re;

		/* Get optical depth for light at this sample: */
		float Hr_sample = exp(-sampleHeight / Hr) * segmentLength;
		float Hm_sample = exp(-sampleHeight / Hm) * segmentLength;

		opticalDepthR += Hr_sample;
		opticalDepthM += Hm_sample;

		//Get light depth to sun at this sample.
		float opticalDepthLR = 0;
		float opticalDepthLM = 0;

		linesample atmosphereline_sample;
		atmosphereline_sample.mHasSolutions = has_solutions(Px, SunDirection, Ra);
		atmosphereline_sample.mType = linetype(Px, SunDirection, Ra);
		atmosphereline_sample.mMin = lmin(Px, SunDirection, Ra);
		atmosphereline_sample.mMax = lmax(Px, SunDirection, Ra);

		linesample groundline_sample;
		groundline_sample.mHasSolutions = has_solutions(Px, SunDirection, Re);
		groundline_sample.mType = linetype(Px, SunDirection, Re);
		groundline_sample.mMin = lmin(Px, SunDirection, Re);
		groundline_sample.mMax = lmax(Px, SunDirection, Re);

		vec3 Ps = atmosphereline_sample.mMax;
		int j = 0;
		if (groundline_sample.mHasSolutions && groundline_sample.mType == 0)
		{
			//Light is below horizon from this point. No sample needed.
			opticalDepthLR += 0;
			opticalDepthLM += 0;
		}
		else
		{
			float segmentLengthL = length(Px - Ps) / numSamplesL;

			for (j = 0; j < numSamplesL; j++)
			{
				vec3 Pl = Px + segmentLengthL * SunDirection * float(j + 0.5);
				float sampleHeightL = length(Pl) - Re;
				if (sampleHeightL < 0) break;
				// ignore sampleheights < 0 they're inside the planet...
				float Hlr_sample = exp(-sampleHeightL / Hr) * segmentLengthL;
				float Hlm_sample = exp(-sampleHeightL / Hm) * segmentLengthL;

				opticalDepthLR += Hlr_sample;
				opticalDepthLM += Hlm_sample;
			}
		}

		// With our light samples done we can calculate the attenuation.
		// Only include a sample if we reached j (so not if we hit the break clause for rays that hit the ground in finding the sun)
		if (j == numSamplesL)
		{
			vec3 tauR = BetaR * (opticalDepthR + opticalDepthLR);
			vec3 tauM = BetaM * 1.1f * (opticalDepthM + opticalDepthLM);
			vec3 tau = tauR + tauM;
			vec3 attnR = vec3(exp(-tauR.x), exp(-tauR.y), exp(-tauR.z));
			vec3 attnM = exp(-tauM);
			vec3 attenuation = vec3(exp(-tau.x), exp(-tau.y), exp(-tau.z));

			SumR += Hr_sample * attenuation;
			SumM += Hm_sample * attenuation;
		}
	}

	rayleigh = (SumR * phaseR * BetaR) * rayleighIntensity;
	mie = (SumM * phaseM * BetaM) * mieIntensity;
	sky = rayleigh + mie;
}


#ifndef GLSL_SHADER
# undef INOUT_VEC3 inout vec3
# undef mat4
# undef min
# undef max
# undef vec4
# undef vec3
# undef vec2
# undef uint
# undef ivec4
#endif


#endif