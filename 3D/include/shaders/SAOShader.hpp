#pragma once
#include "Shader.hpp"
#include <map>
#include <math.h>

class SAOShader : public ShaderProgram
{
private:
	//Locations
	int projScaleLoc = -1;
	int depthBufferLoc = -1;
	int radiusLoc = -1;
	int biasLoc = -1;
	int intensityDivR6Loc = -1;
	int gNormalLoc = -1;
	int viewLoc = -1;
	int projInfoLoc = -1;
	int samplesLoc = -1;
	int spiralTurnsLoc = -1;

	//Variables
	float projScale;
	s32 depthBuffer = 0;
	float radius;
	float bias;
	float intensity;
	s32 gNormal;
	glm::fmat4 view;
	glm::fmat4 proj;
	glm::ivec2 viewport = glm::ivec2(1, 1);
	glm::fvec4 projInfo = glm::fvec4(1.f, 1.f, 1.f, 1.f);
	int samples;
	int spiralTurns;

public:
	SAOShader() 
	{
		name = "ssao";
		type = VertFrag;
	}

	int initialise()
	{
		use();
		projScaleLoc = glGetUniformLocation(GLID, "projScale");
		depthBufferLoc = glGetUniformLocation(GLID, "depthBuffer");
		radiusLoc = glGetUniformLocation(GLID, "radius");
		biasLoc = glGetUniformLocation(GLID, "bias");
		intensityDivR6Loc = glGetUniformLocation(GLID, "intensityDivR6");
		gNormalLoc = glGetUniformLocation(GLID, "gNormal");
		viewLoc = glGetUniformLocation(GLID, "view");
		projInfoLoc = glGetUniformLocation(GLID, "projInfo");
		samplesLoc = glGetUniformLocation(GLID, "samples");
		spiralTurnsLoc = glGetUniformLocation(GLID, "spiralTurns");

		setRadius(1.5f);
		setIntensity(2.75f);
		setProjScale(500.f);
		setSamples(50);
		setSpiralTurns(9);
		setBias(0.01);

		stop();

		return 1;
	}

	void setProjScale(float pProjScale)
	{
		projScale = pProjScale;
	}

	void setDepthBuffer(u32 pDepthBuffer)
	{
		depthBuffer = pDepthBuffer;
	}

	void setRadius(float pRadius)
	{
		radius = pRadius;
	}

	float getRadius()
	{
		return radius;
	}
	
	void setSamples(int pSamples)
	{
		samples = pSamples;
	}

	int getSamples()
	{
		return samples;
	}

	void setSpiralTurns(int pSpiralTurns)
	{
		spiralTurns = pSpiralTurns;
	}

	int getSpiralTurns()
	{
		return spiralTurns;
	}

	void setBias(float pBias)
	{
		bias = pBias;
	}

	void setIntensity(float pIntensity)
	{
		intensity = pIntensity;
	}

	float getIntensity()
	{
		return intensity;
	}

	void setNormal(u32 pNormal)
	{
		gNormal = pNormal;
	}
	
	void setProj(glm::fmat4 &pProj)
	{
		proj = pProj;
		projInfo = glm::fvec4(-2.f / (viewport.x * proj[0][0]), -2.f / (viewport.y*proj[1][1]), (1.f - proj[0][2]) / proj[0][0], (1.f + proj[1][2]) / proj[1][1]);
	}

	void setView(glm::fmat4& pView)
	{
		view = pView;
	}

	void setViewport(glm::ivec2 pViewport)
	{
		viewport = pViewport;
		projInfo = glm::fvec4(-2.f / (viewport.x * proj[0][0]), -2.f / (viewport.y*proj[1][1]), (1.f - proj[0][2]) / proj[0][0], (1.f + proj[1][2]) / proj[1][1]);
	}

	void sendUniforms()
	{
		glUniform1f(projScaleLoc, projScale);
		glUniform1i(depthBufferLoc, depthBuffer);
		glUniform1f(radiusLoc, radius);
		glUniform1f(biasLoc, bias);
		glUniform1f(intensityDivR6Loc, intensity / std::pow(radius, 6.f));
		glUniform1i(gNormalLoc, gNormal);
		glUniformMatrix2fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniform1f(projScaleLoc, projScale);
		glUniform4fv(projInfoLoc, 1, glm::value_ptr(projInfo));
		glUniform1i(samplesLoc, samples);
		glUniform1i(spiralTurnsLoc, spiralTurns);
	}
};