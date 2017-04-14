#pragma once
#include "Include.h"
#include <string>
#include <fstream>
#include "Shader.h"
#include "Transform.h"
#include "Engine.h"
#include "Asset.h"
#include "SceneGraph.h"

#include "Texture.h"

#include "Bounds3D.h"

struct ObjectData
{
	std::vector<float> verts;
	std::vector<float> texCoords;
	std::vector<float> normals;
	std::vector<int> indices;
	s32 numVert;
	s32 numTris;
};

struct InterlacedObjectData
{
	float* interlacedData;
	s32 size;
};

class VertexAttrib
{
public:

	enum VertexAttribType {
		flt2, flt3, flt4,
		dbl2, dbl3, dbl4,
		AttribTypesCount
	};

	static const int typeSizes[AttribTypesCount];
	static const String<16> typeNames[AttribTypesCount];

	String<32> name;
	VertexAttribType type;

	void operator=(VertexAttrib& rhs)
	{
		name.overwrite(rhs.name);
		type = rhs.type;
	}
};

class VertexFormat
{
public:

	VertexAttrib attribs[12];
	s32 numAttribs;
	s32 size;

	s32 calculateSizeInBytes()
	{
		size = 0;
		for (int i = 0; i < numAttribs; ++i)
		{
			size += VertexAttrib::typeSizes[attribs[i].type];
		}
		return size;
	}

	void operator=(VertexFormat& rhs)
	{
		for (int i = 0; i < 12; ++i)
		{
			attribs[i] = rhs.attribs[i];
		}
		numAttribs = rhs.numAttribs;
		size = rhs.size;
	}

	void defineGLVertexAttribs(GLuint vao);
};

enum MaterialID
{
	PN,
	PNUU_T_S_N,
	PNUU_TT_SS_NN,
	PNUU_TT_SS,
	PNUU_TT_NN,
	PNUU_TTT_SSS_NNN,
	MATERIALS_COUNT
};

enum DrawMode;

class Material
{
public:
	Material() {}

	MaterialID matID;

	GLTexture2D* albedo[4];
	GLTexture2D* specular[4];
	GLTexture2D* normal[4];
	GLTexture2D* alpha;

	static DrawMode drawModes[MATERIALS_COUNT];
	static VertexFormat vertexFormats[MATERIALS_COUNT];
	static void initVertexFormats()
	{
		VertexFormat* vf = vertexFormats + PNUU_T_S_N;
		vf->numAttribs = 3;
		vf->attribs[0].name.setToChars("p");
		vf->attribs[0].type = VertexAttrib::flt3;
		vf->attribs[1].name.setToChars("n");
		vf->attribs[1].type = VertexAttrib::flt3;
		vf->attribs[2].name.setToChars("t");
		vf->attribs[2].type = VertexAttrib::flt2;
		vf->calculateSizeInBytes();

		vf = vertexFormats + PNUU_TT_SS_NN;
		vf->numAttribs = 3;
		vf->attribs[0].name.setToChars("p");
		vf->attribs[0].type = VertexAttrib::flt3;
		vf->attribs[1].name.setToChars("n");
		vf->attribs[1].type = VertexAttrib::flt3;
		vf->attribs[2].name.setToChars("t");
		vf->attribs[2].type = VertexAttrib::flt2;
		vf->calculateSizeInBytes();
	}
	static void initDrawModes();

	static DrawMode getDrawMode(MaterialID matID);

	DrawMode getDrawMode();

	static s32 getFormatSize(MaterialID matID)
	{
		return vertexFormats[matID].size;
	}

	s32 getFormatSize()
	{
		return vertexFormats[matID].size;
	}
};

class OBJMeshData;

class InterleavedVertexData
{
public:
	InterleavedVertexData() : data(nullptr) {}
	~InterleavedVertexData()
	{
		if (data != nullptr)
		{
			delete[] data;
		}
	}

	void fromOBJMeshData(OBJMeshData* obj);

	s32 getDataSizeInBytes() { return size * sizeof(float); }

	VertexFormat format;
	float* data;
	s32 size;

};

class MeshBatch;

class MeshRenderMeta
{
public:
	//u32 batchID;
	MeshBatch* batchPtr;
	u32 batchIndex;
};

struct MeshInstanceMeta
{
	MeshRenderMeta renderMeta;
	SGNode* nodePtr;
};

class Mesh : public Asset
{
	friend class GPUMeshManager;
	friend class MeshUtility;
	friend class World;
public:
	Mesh() {}
	Mesh(String128& pPath, String32& pName) : Asset(pPath, pName) {}
	~Mesh() 
	{
		for (int i = 0; i < triangleLists.size(); ++i)
		{
			delete[] triangleLists[i].data;
		}
	}

	void loadBinary()
	{
		/*std::ifstream ifs(diskPath.getString(), std::ios_base::binary);
		s32 size;
		float* glVerts;
		ifs.read((char*)&size, sizeof(size));
		intData.interlacedData = (new float[size]);
		ifs.read((char*)intData.interlacedData, sizeof(GLfloat) * size);
		intData.size = size;
		data.numVert = size / 8;
		data.numTris = data.numVert / 3;*/
	}

	void saveBinary(StringGeneric& binPath)
	{
		/*std::ofstream ofs(binPath.getString(), std::ios_base::binary);
		ofs.write((char*)&intData.size, sizeof(intData.size));
		std::cout << intData.size << std::endl;
		ofs.write((char*)intData.interlacedData, intData.size * sizeof(float));*/
	}

	void save(StringGeneric& binPath)
	{
		saveBinV11(binPath);
	}

	void saveBinV10(StringGeneric& binPath)
	{
		std::ofstream ofs(binPath.getString(), std::ios_base::binary);
		char major = 1; char minor = 0;
		ofs.write(&major, sizeof(major));
		ofs.write(&minor, sizeof(minor));
		ofs.write(getName().getString(), getName().getCapacity());
		auto numTriLists = triangleLists.size();
		ofs.write((char*)&(numTriLists), sizeof(numTriLists));

		for (auto i = 0; i < triangleLists.size(); ++i)
		{
			TriangleList* tl = &triangleLists[i];
			ofs.write((char*)&tl->material.matID, sizeof(tl->material.matID));
			ofs.write((char*)&tl->numVerts, sizeof(tl->numVerts));
			ofs.write(tl->material.albedo[0]->getName().getString(), tl->material.albedo[0]->getName().getCapacity());
			auto size = tl->getDataSizeInBytes();
			ofs.write((char*)&size, sizeof(size));
			ofs.write((char*)tl->data, size);
		}
	}

	void saveBinV11(StringGeneric& binPath)
	{
		std::ofstream ofs(binPath.getString(), std::ios_base::binary);
		char major = 1; char minor = 0;
		ofs.write(&major, sizeof(major));
		ofs.write(&minor, sizeof(minor));
		ofs.write(getName().getString(), getName().getCapacity());
		auto numTriLists = triangleLists.size();
		ofs.write((char*)&(numTriLists), sizeof(numTriLists));

		for (auto i = 0; i < triangleLists.size(); ++i)
		{
			TriangleList* tl = &triangleLists[i];
			ofs.write((char*)&tl->material.matID, sizeof(tl->material.matID));
			ofs.write((char*)&tl->numVerts, sizeof(tl->numVerts));
			ofs.write(tl->material.albedo[0]->getName().getString(), tl->material.albedo[0]->getName().getCapacity());
			ofs.write(tl->material.albedo[1]->getName().getString(), tl->material.albedo[1]->getName().getCapacity());
			ofs.write(tl->material.albedo[2]->getName().getString(), tl->material.albedo[2]->getName().getCapacity());
			ofs.write(tl->material.albedo[3]->getName().getString(), tl->material.albedo[3]->getName().getCapacity());
			ofs.write(tl->material.specular[0]->getName().getString(), tl->material.specular[0]->getName().getCapacity());
			ofs.write(tl->material.specular[1]->getName().getString(), tl->material.specular[1]->getName().getCapacity());
			ofs.write(tl->material.specular[2]->getName().getString(), tl->material.specular[2]->getName().getCapacity());
			ofs.write(tl->material.specular[3]->getName().getString(), tl->material.specular[3]->getName().getCapacity());
			ofs.write(tl->material.normal[0]->getName().getString(), tl->material.normal[0]->getName().getCapacity());
			ofs.write(tl->material.normal[1]->getName().getString(), tl->material.normal[1]->getName().getCapacity());
			ofs.write(tl->material.normal[2]->getName().getString(), tl->material.normal[2]->getName().getCapacity());
			ofs.write(tl->material.normal[3]->getName().getString(), tl->material.normal[3]->getName().getCapacity());
			ofs.write(tl->material.alpha->getName().getString(), tl->material.alpha->getName().getCapacity());
			auto size = tl->getDataSizeInBytes();
			ofs.write((char*)&size, sizeof(size));
			ofs.write((char*)tl->data, size);
		}
	}

	void load()
	{
		loadBinV10();
		sortTriangleLists();
	}

	void loadBinV10();
	void loadBinV11();

	void sortTriangleLists()
	{
		for (auto itr = triangleLists.begin(); itr != triangleLists.end(); ++itr)
		{
			bool added = false;
			for (auto itr2 = triangleListsSorted.begin(); itr2 != triangleListsSorted.end(); ++itr2)
			{
				if (itr2->first == Material::getDrawMode(itr->material.matID))
				{
					itr2->second.push_back(&(*itr));
					added = true;
					break;
				}
			}
			if (!added)
			{
				triangleListsSorted.push_back(std::make_pair(Material::getDrawMode(itr->material.matID), std::vector<Mesh::TriangleList*>(1, &(*itr))));
			}
		}
	}

	struct TriangleList
	{
		TriangleList() {}

		Material material;
		float* data;

		s32 numVerts;
		s32 first;

		MeshRenderMeta renderMeta;

		void copyConstruct(TriangleList& tl)
		{
			data = new float[tl.numVerts * Material::vertexFormats[tl.material.matID].size];
			numVerts = tl.numVerts;
			material = tl.material;
			first = tl.first;
			memcpy(data, tl.data, tl.numVerts * Material::vertexFormats[tl.material.matID].size);
		}

		s32 getDataSizeInBytes()
		{
			return numVerts * Material::vertexFormats[material.matID].size;
		}
	};

	u32 getNumTriLists(DrawMode pDrawMode)
	{
		u32 ret = 0;
		for (auto itr = triangleListsSorted.begin(); itr != triangleListsSorted.end(); ++itr)
		{
			if(itr->first == pDrawMode)
			{
				for (auto itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
				{
					++ret;
				}
			}
		}
		return ret;
	}

//private:

	std::vector<TriangleList> triangleLists;
	std::vector<std::pair<DrawMode, std::vector<TriangleList*>>> triangleListsSorted;
	std::vector<String<32>> listTextureNames;

	AABounds3D bounds;
	float radius;
};

struct MeshGPUMetaRegular
{
	MeshGPUMetaRegular() {}
	union
	{
		/*
		typedef struct 
		{
			uint  count;
			uint  instanceCount;
			uint  first;
			uint  baseInstance;
		} DrawArraysIndirectCommand;
		*/
		glm::uvec4 cmds;
		struct
		{
			glm::uvec3 cmdss;
			float radiusX;
		};
	};
	glm::uvec4 albedoHandle_RadiusYZ;
	glm::uvec4 normal_bump_Handles;
};

struct MeshGPUMetaMultiTextured
{
	MeshGPUMetaMultiTextured() {}
	union
	{
		/*
		typedef struct
		{
		uint  count;
		uint  instanceCount;
		uint  first;
		uint  baseInstance;
		} DrawArraysIndirectCommand;
		*/
		glm::uvec4 cmds;
		struct
		{
			glm::uvec3 cmdss;
			float radiusX;
		};
	};
	glm::uvec4 albedoHandleA_RadiusYZ;
	glm::uvec4 specularA_normalA;
	glm::uvec4 albedoB_specularB;
	glm::uvec4 normalB_albedoC;
	glm::uvec4 specularC_normalC;
	glm::uvec4 albedoD_specularD;
	glm::uvec4 normalD_alpha;
};


class MeshInstance
{
public:
	MeshInstance() {}
	~MeshInstance() {}

	void setTriListMaterial(u32 triListIndex, Material& material)
	{
		if (triListIndex > mesh->triangleLists.size() - 1)
			return;

		if (triListIndex >= overwriteMaterials.size())
		{
			for (int i = overwriteMaterials.size(); i <= triListIndex; ++i)
			{
				overwriteMaterials.push_back(mesh->triangleLists[i].material);
			}
			overwriteMaterials.push_back(material);
		}
		else
		{
			overwriteMaterials[triListIndex] = material;
		}
	}

//private:
	Mesh* mesh;
	//u32 instanceID;
	SGNode* sgNode;
	std::vector<Material> overwriteMaterials; //For each tri list
};

#define MAX_BATCH_COUNT 512
#define MAX_BATCH_SIZE 1024*1024*64

