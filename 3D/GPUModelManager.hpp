#pragma once
#include "Renderer.hpp"
#include "Model.hpp"

#define MAX_BATCH_COUNT 512
#define MAX_BATCH_SIZE 1024*1024*128

class MeshBatch
{
public:
	GLint firsts[MAX_BATCH_COUNT];
	GLsizei counts[MAX_BATCH_COUNT];
	GLsizei length;

	GLfloat* data[MAX_BATCH_COUNT];
	GLint dataSizeInBytes[MAX_BATCH_COUNT];

	GLuint vboID;
	GLuint vaoID;
};

class GPUModelManager
{
public:
	GPUModelManager() {}
	~GPUModelManager() {}

	void init()
	{
		auto program = &Engine::r->gBufferShaderTex;

		program->use();

		glGenVertexArrays(1, &regularBatch.vaoID);
		glBindVertexArray(regularBatch.vaoID);

		glGenBuffers(1, &regularBatch.vboID);
		glBindBuffer(GL_ARRAY_BUFFER, regularBatch.vboID);

		glBufferData(GL_ARRAY_BUFFER, MAX_BATCH_SIZE, NULL, GL_STATIC_DRAW);

		regularBatch.length = 0;

		auto posAttrib = glGetAttribLocation(program->getGLID(), "p");
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(posAttrib);

		auto norAttrib = glGetAttribLocation(program->getGLID(), "n");
		glVertexAttribPointer(norAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(norAttrib);

		auto texAttrib = glGetAttribLocation(program->getGLID(), "t");
		glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(texAttrib);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		auto program2 = &Engine::r->pointShadowPassShader;
		program2->use();

		glGenVertexArrays(1, &shadowVAO);
		glBindVertexArray(shadowVAO);

		glBindBuffer(GL_ARRAY_BUFFER, regularBatch.vboID);

		posAttrib = glGetAttribLocation(program2->getGLID(), "p");
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(posAttrib);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		program2->stop();
	}

	void pushModelToBatch(Model& pModel)
	{
		for (auto itr = pModel.triLists.begin(); itr != pModel.triLists.end(); ++itr)
		{
			for (auto itr2 = itr->begin(); itr2 != itr->end(); ++itr2)
			{
				auto& batch = regularBatch;

				GLsizei size = 0;
				for (int i = 0; i < batch.length; ++i)
				{
					size += batch.dataSizeInBytes[i];
				}

				auto prevFirst = batch.length == 0 ? 0 : batch.firsts[batch.length - 1];
				auto prevCount = batch.length == 0 ? 0 : batch.counts[batch.length - 1];

				batch.firsts[batch.length] = prevFirst + prevCount;
				batch.counts[batch.length] = itr2->numVerts;
				batch.data[batch.length] = itr2->data;
				batch.dataSizeInBytes[batch.length] = itr2->getDataSizeInBytes();

				glBindVertexArray(batch.vaoID);

				glBindBuffer(GL_ARRAY_BUFFER, batch.vboID);

				glBufferSubData(GL_ARRAY_BUFFER, size, batch.dataSizeInBytes[batch.length], batch.data[batch.length]);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				itr2->renderMeta.batchPtr = &batch;
				itr2->renderMeta.batchIndex = batch.length;

				size += batch.dataSizeInBytes[batch.length];
				++batch.length;
			}
		}
	}

	void newBatch();

	MeshBatch regularBatch;
	GLuint shadowVAO;
};