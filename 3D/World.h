#pragma once
#include "Mesh.h"
#include "SceneGraph.h"
#include "Engine.h"
#include "AssetManager.h"
#include "BufferObjects.h"
#include "Renderer.h"
#include <unordered_map>

class World
{
public:
	World() { objectCount = 0; numTriLists = 0; }
	~World() {}

	SGNode* getWorldRootNode()
	{
		return &sg.rootNode;
	}

	MeshInstance* addMeshInstance(Mesh& mesh, SGNode* parent)
	{
		++objectCount;

		MeshInstance mi;
		//mi.meshID = meshID;
		//mi.renderMeta = Engine::resMan.getMesh(meshID).renderMeta;
		mi.renderMeta = mesh.renderMeta;
		mi.sgNode = parent->addChild(SGNode());
		//mi.texx = Engine::assets.get2DTex("g");

		numTriLists += mesh.getNumGPUTriLists();

		u32 instanceID = objectCount;
		mi.mesh = &mesh;
		instances.insert(std::make_pair(instanceID, mi));
		return &instances.at(instanceID);
	}

	MeshInstance* getMeshInstance(u32 pInstanceID)
	{
		return &instances.at(pInstanceID);
	}

	void initialiseGLBuffers(u32 pMaxObjects)
	{
		maxObjects = pMaxObjects;
		instances.reserve(maxObjects);
		objectMetaBuffer.bufferData(pMaxObjects * sizeof(MeshGPUMeta), 0, GL_STATIC_READ);
		texHandleBuffer.bufferData(pMaxObjects * sizeof(float) * 16, 0, GL_STATIC_READ);
		drawIndirectBuffer.bufferData(pMaxObjects * sizeof(GLCMD), 0, GL_STREAM_DRAW);
		drawCountBuffer.bufferData(sizeof(u32), 0, GL_STREAM_READ);
		instanceTransformsBuffer.bufferData(pMaxObjects * sizeof(float) * 16, 0, GL_STATIC_READ);
		visibleTransformsBuffer.bufferData(pMaxObjects * sizeof(float) * 16, 0, GL_STATIC_READ);
		instanceIDBuffer.bufferData(pMaxObjects * sizeof(u32), 0, GL_DYNAMIC_READ);
	}

	//TODO: updateInstanceRange, updateInstanceList

	void updateGLBuffers()
	{
		MeshGPUMeta* meta = (MeshGPUMeta*)objectMetaBuffer.mapRange(0, numTriLists * sizeof(MeshGPUMeta), GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_WRITE_BIT);
		glm::fmat4* instanceTransforms = (glm::fmat4*)instanceTransformsBuffer.mapRange(0, numTriLists * sizeof(float) * 16, GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_WRITE_BIT);
		u32 i = 0;

		const GPUMeshManager& mm = Engine::assets.meshManager;

		for (auto itr = instances.begin(); itr != instances.end(); ++itr)
		{
			for (auto itr2 = itr->second.mesh->triangleListsSorted.begin(); itr2 != itr->second.mesh->triangleListsSorted.end(); ++itr2)
			{	
				for (auto itr3 = itr2->second.begin(); itr3 != itr2->second.end(); ++itr3)
				{
					auto bptr = (*itr3)->renderMeta.batchPtr;
					auto bindex = (*itr3)->renderMeta.batchIndex;

					meta[i].cmds[0] = bptr->counts[bindex];
					meta[i].cmds[1] = 1;
					meta[i].cmds[2] = bptr->firsts[bindex];

					//auto texHandle = itr->second.texx.getHandle(Engine::r->defaultSampler.getGLID());

					//glMakeTextureHandleResidentARB(texHandle);

					///TODO: Check material and place in corresponding buffer, handle more textures
					s64 albedoHandle = (*itr3)->material.albedo[0]->getHandle(Engine::r->defaultSampler.getGLID());
					s64 normalHandle = (*itr3)->material.normal[0]->getHandle(Engine::r->defaultSampler.getGLID());

					meta[i].texHandleMatID.r = (u32)(albedoHandle);
					meta[i].texHandleMatID.g = (u32)(albedoHandle >> 32);

					meta[i].normalBumpMap.r = (u32)(normalHandle);
					meta[i].normalBumpMap.g = (u32)(normalHandle >> 32);

					meta[i].texHandleMatID.b = bptr->radii[bindex].y;
					meta[i].texHandleMatID.a = bptr->radii[bindex].z;
					meta[i].radius = bptr->radii[bindex].x;

					instanceTransforms[i] = itr->second.sgNode->transform.getTransformMat();

					++i;
				}
			}
		}

		objectMetaBuffer.unmap();
		instanceTransformsBuffer.unmap();
	}

	/*
		objectMetaBuffer :: array of structs "MeshGPUMeta"
			would need to be updated for every texture change
	*/
	GLBufferObject objectMetaBuffer; 
	/*
		texHandleBuffer :: array of visible instance tex handles, 
			re-generated by compute shader each frame
	*/
	GLBufferObject texHandleBuffer;
	/*
		drawIndirectBuffer :: array of indirect draw commands, 
			re-generated by compute shader each frame
	*/
	GLBufferObject drawIndirectBuffer;
	/*
		drawCountBuffer :: one u32 value, represents number of instances visible in current frame
			re-generated by compute shader each frame
	*/
	GLBufferObject drawCountBuffer;
	/*
		instanceTransformsBuffer :: array of transforms of each instance, 
			order should be determined by how dynamic an instance is(how much/often it moves) for ease of mapping/updating
	*/
	GLBufferObject instanceTransformsBuffer;
	/*
		visibleTransformsBuffer :: array of transforms of each visible instance,
			re-generated by compute shader each frame
	*/
	GLBufferObject visibleTransformsBuffer;
	/*
		instanceIDBuffer :: array of instance IDs of each visible instance
			re-generated by compute shader each frame
	*/
	GLBufferObject instanceIDBuffer;

	std::unordered_map<u32, MeshInstance> instances;
	u32 numTriLists;

	u32 objectCount;
	u32 maxObjects;
	SceneGraph sg;
};