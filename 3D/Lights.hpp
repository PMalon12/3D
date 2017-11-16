#pragma once
#include "Include.hpp"
#include "Texture.hpp"
#include "Framebuffer.hpp"
#include "BufferObjects.hpp"
#include "Sampler.hpp"
#include "Billboard.hpp"



struct DirectLightData
{
	DirectLightData() {}
	DirectLightData(glm::fvec3 dir, glm::fvec3 col) : direction(dir), colour(col) {}
	~DirectLightData() {}

	glm::fvec3 direction, colour;
};

class PointLight
{
public:
	PointLight();
	struct GPUData
	{
	public:
		GPUData() : positionRad(glm::fvec4(0.f,0.f,0.f,50.f)), colourQuad(glm::fvec4(1.f,1.f,1.f,0.001f)), linear(0.001f), textureHandle(0), fadeStart(100), fadeLength(15) {}
		~GPUData() {}

		union
		{
			struct
			{
				glm::fvec3 position;
				float radius;
			};
			glm::fvec4 positionRad;
		};

		union
		{
			struct
			{
				glm::fvec3 colour;
				float quadratic;
			};
			glm::fvec4 colourQuad;
		};

		union
		{
			struct
			{
				float linear;
				unsigned short fadeStart;
				unsigned short fadeLength;
				GLuint64 textureHandle;
			};
			glm::fvec4 linearTexHandle;
		};

		struct
		{
			glm::fmat4 projView[6];
		};
	};

	void setPosition(glm::fvec3 pPos)
	{
		gpuData->position = pPos;
	}

	void setColour(glm::fvec3 pColour)
	{
		gpuData->colour = pColour;
	}

	void setQuadratic(float pQuad)
	{
		gpuData->quadratic = pQuad;
		updateRadius();
	}

	void setLinear(float pLinear)
	{
		gpuData->linear = pLinear;
		updateRadius();
	}

	void updateRadius();

	inline static float calculateRadius(float linear, float quad);

	void updateView()
	{ ///TODO: Dont think lookAt() is needed, since only the position is changing, the directions are always the same, so the matrix should be constructed with only changing a few of its values
		view[0] = (glm::lookAt(gpuData->position, gpuData->position + glm::fvec3(1.0, 0.0, 0.0), glm::fvec3(0.0, -1.0, 0.0)));
		view[1] = (glm::lookAt(gpuData->position, gpuData->position + glm::fvec3(-1.0, 0.0, 0.0), glm::fvec3(0.0, -1.0, 0.0)));
		view[2] = (glm::lookAt(gpuData->position, gpuData->position + glm::fvec3(0.0, 1.0, 0.0), glm::fvec3(0.0, 0.0, 1.0)));
		view[3] = (glm::lookAt(gpuData->position, gpuData->position + glm::fvec3(0.0, -1.0, 0.0), glm::fvec3(0.0, 0.0, -1.0)));
		view[4] = (glm::lookAt(gpuData->position, gpuData->position + glm::fvec3(0.0, 0.0, 1.0), glm::fvec3(0.0, -1.0, 0.0)));
		view[5] = (glm::lookAt(gpuData->position, gpuData->position + glm::fvec3(0.0, 0.0, -1.0), glm::fvec3(0.0, -1.0, 0.0)));
	}

	void updateProj()
	{
		proj = glm::perspective<float>(glm::radians(90.f), 1.f, 0.01f, gpuData->radius);
	}

	void updateProjView()
	{
		for (int i = 0; i < 6; ++i)
		{
			gpuData->projView[i] = proj * view[i];
		}
	}

	void initTexture(Sampler& sampler)
	{
		shadowTex.createFromStream(GL_DEPTH_COMPONENT32F_NV, 1024, 1024, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		gpuData->textureHandle = shadowTex.makeResident(sampler.getGLID());
	}

	GPUData* gpuData;
	GLTextureCube shadowTex;
	Framebuffer* fbo;

	float shadowRenderDistance;

	glm::fmat4 proj;
	glm::fmat4 view[6];
};

class SpotLight
{
public:
	SpotLight() {}
	~SpotLight() {}

	struct GPUData
	{
		GPUData() : fadeStart(100), fadeLength(15) {}
		GPUData(glm::fvec3 pos, glm::fvec3 dir, glm::fvec3 col, float inner, float outer = 0.f, float lin = 0.0001f, float quad = 0.0003f) : textureHandle(0), position(pos), direction(dir), colour(col), linear(lin), quadratic(quad), innerSpread(inner), outerSpread(outer) {
			if (outer < inner) { outer = inner; }
		}
		~GPUData() {}

		union
		{
			struct
			{
				glm::fvec3 position;
				float radius;
			};
			glm::fvec4 posRad;
		};

		union
		{
			struct
			{
				glm::fvec3 direction;
				float innerSpread;
			};
			glm::fvec4 dirInner;
		};

		union
		{
			struct
			{
				glm::fvec3 colour;
				float outerSpread;
			};
			glm::fvec4 colOuter;
		};

		union
		{
			struct
			{
				float linear;
				float quadratic;
				GLuint64 textureHandle;
			};
			glm::fvec4 linQuadTex;
		};

		union
		{
			struct
			{
				unsigned short fadeStart;
				unsigned short fadeLength;
				unsigned int na;
				unsigned int na2;
				unsigned int na3;
			};
		};

		glm::fmat4 projView;
	};

	void updateRadius();

	inline static float calculateRadius(float linear, float quad);

	void setPosition(glm::fvec3 pPos)
	{
		gpuData->position = pPos;
		updateView();
	}

	void setInnerSpread(float pInner)
	{
		gpuData->innerSpread = glm::cos(pInner);
	}

	void setOuterSpread(float pOuter)
	{
		gpuData->outerSpread = glm::cos(pOuter);
		updateProj();
		updateProjView();
	}

	void setDirection(glm::fvec3 pDirection)
	{
		gpuData->direction = pDirection;
		updateView();
		updateProjView();
	}

	void setColour(glm::fvec3 pColour)
	{
		gpuData->colour = pColour;
	}

	void setQuadratic(float pQuad)
	{
		gpuData->quadratic = pQuad;
		updateRadius();
	}

	void setLinear(float pLinear)
	{
		gpuData->linear = pLinear;
		updateRadius();
	}

	void updateView()
	{
		view = glm::lookAt(gpuData->position, gpuData->position + gpuData->direction, glm::fvec3(0, 1, 0));
	}

	void updateProj()
	{
		proj = glm::perspective<float>(gpuData->outerSpread, 1.f, 0.01f, gpuData->radius * 2.f);
	}

	void updateProjView()
	{
		gpuData->projView = proj * view;
	}

	void initTexture(Sampler& sampler)
	{
		shadowTex.createFromStream(GL_DEPTH_COMPONENT32F_NV, 512, 512, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		gpuData->textureHandle = shadowTex.makeResident(sampler.getGLID());
	}

	GPUData* gpuData;
	GLTexture2D shadowTex;
	glm::fmat4 proj;
	glm::fmat4 view;
};

class LightManager
{
public: ///TODO: Max light count is 500, add setters etc
	LightManager() 
	{
		spotLightsGPUData.reserve(500);
		pointLightsGPUData.reserve(500);
	}
	~LightManager() {}

	void drawLightIcons()
	{
		int i = 0;
		for (auto& b : pointLightIcons)
		{
			b.pos = pointLightsGPUData[i].position;
			b.draw();
			++i;
		}

		i = 0;
		for (auto& b : spotLightIcons)
		{
			b.pos = spotLightsGPUData[i].position;
			b.draw();
			++i;
		}
	}

	PointLight& addPointLight()
	{
		return addPointLight(PointLight::GPUData());
	}

	PointLight& addPointLight(PointLight::GPUData& data);

	PointLight* getPointLight(u32 pIndex)
	{
		return pointLights.data() + pIndex;
	}

	void updatePointLight(u32 pIndex)
	{
		auto data = (PointLight::GPUData*)pointLightsBuffer.mapRange(pIndex * sizeof(PointLight::GPUData), sizeof(PointLight::GPUData), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
		data->positionRad = pointLightsGPUData[pIndex].positionRad;
		data->colourQuad = pointLightsGPUData[pIndex].colourQuad;
		pointLightsBuffer.unmap();
	}

	void updateAllPointLights()
	{
		pointLightsBuffer.bufferData(sizeof(PointLight::GPUData) * pointLightsGPUData.size(), pointLightsGPUData.data(), GL_DYNAMIC_DRAW);
	}

	SpotLight& addSpotLight()
	{
		return addSpotLight(SpotLight::GPUData());
	}

	SpotLight& addSpotLight(SpotLight::GPUData& data);

	SpotLight* getSpotLight(u32 pIndex)
	{
		return spotLights.data() + pIndex;
	}

	void updateSpotLight(u32 pIndex)
	{
		auto data = (SpotLight::GPUData*)spotLightsBuffer.mapRange(pIndex * sizeof(SpotLight::GPUData), sizeof(SpotLight::GPUData), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
		data->posRad = spotLights[pIndex].gpuData->posRad;
		data->dirInner = spotLights[pIndex].gpuData->dirInner;
		data->colOuter = spotLights[pIndex].gpuData->colOuter;
		data->linQuadTex = spotLights[pIndex].gpuData->linQuadTex;
		spotLightsBuffer.unmap();
	}

	void updateAllSpotLights()
	{
		spotLightsBuffer.bufferData(sizeof(SpotLight::GPUData) * spotLightsGPUData.size(), spotLightsGPUData.data(), GL_DYNAMIC_DRAW);
	}

	std::vector<PointLight> pointLights;
	std::vector<PointLight::GPUData> pointLightsGPUData;
	SSBO pointLightsBuffer;

	std::vector<SpotLight> spotLights;
	std::vector<SpotLight::GPUData> spotLightsGPUData;
	SSBO spotLightsBuffer;

	std::vector<DirectLightData> directLights;

	std::vector<Billboard> pointLightIcons;
	std::vector<Billboard> spotLightIcons;
	
	//std::vector<SpotLight::GPUData> staticSpotLights;
	//std::vector<PointLight::GPUData> staticPointLightsGPUData;
	//std::vector<GLTextureCube> staticPointLightsShadowTex;
};
