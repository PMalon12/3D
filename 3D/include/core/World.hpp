#pragma once
#include "SceneGraph.hpp"
#include "BufferObjects.hpp"
#include <unordered_map>
#include <map>
#include "DrawModes.hpp"
#include "Texture.hpp"

class ModelInstance;
class Model;

class World
{
public:
	World();
	~World() {}

	SGNode* getWorldRootNode();

	ModelInstance* addModelInstance(Model* model, SGNode* parent);
	ModelInstance* addModelInstance(Model& model, SGNode* parent);
	ModelInstance* addModelInstance(std::string modelName, SGNode* parent);
	ModelInstance* addModelInstance(Model& model);
	ModelInstance* addModelInstance(std::string modelName);

	ModelInstance* getModelInstance(u32 pInstanceID);

	void initialiseGLBuffers();

	/// TODO: updateInstanceRange, updateInstanceList

	void calculateGLMetrics();

	void updateGLTransforms();
	void updateGLBuffers();
	void updateDrawBuffer();

	void setSkybox(std::string pathToFolder);

	/*
		texHandleBuffer :: array of visible instance tex handles, 
			re-generated by compute shader each frame
	*/
	GLBufferObject texHandleBuffer[DrawModesCount];

	/*
		drawIndirectBuffer :: array of indirect draw commands, 
			re-generated by compute shader each frame
	*/
	GLBufferObject drawIndirectBuffer[DrawModesCount];

	/*
		instanceTransformsBuffer :: array of transforms of each instance, 
			order should be determined by how dynamic an instance is(how much/often it moves) for ease of mapping/updating
	*/
	GLBufferObject instanceTransformsBuffer[DrawModesCount];


	/*
		instanceIDBuffer :: array of instance IDs of each visible instance
			re-generated by compute shader each frame
	*/
	GLBufferObject instanceIDBuffer;

	std::unordered_map<u32, ModelInstance> modelInstances;
	u32 numTriLists[DrawModesCount];

	GLTextureCube skybox;

	struct ObjectScopes
	{
		ObjectScopes() : maxRegular(0), maxMultiTextured(0) {}
		ObjectScopes(u32 pMaxRegular, u32 pMaxMultiTextured) : maxRegular(pMaxRegular), maxMultiTextured(pMaxMultiTextured) {}
		u32 maxRegular;
		u32 maxMultiTextured;

		u32 getTotalMaxObjects() { return maxRegular + maxMultiTextured; }
	};

	ObjectScopes objectScopes;

	struct GLMetrics
	{
		s32 regIndirectBufferSize;
		s32 shadowIndirectBufferSize;
		
	} glMetrics;

	u32 objectCount;
	//u32 maxObjects;
	SceneGraph sg;
};