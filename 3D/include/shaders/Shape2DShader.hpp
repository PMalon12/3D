#pragma once
#include "Shader.hpp"

class Shape2DShader : public ShaderProgram
{
public:
	Shape2DShader() 
	{
		name = "Shape2DShader";
		type = VertFrag;
	}
	~Shape2DShader() {}

	int initialise()
	{
		//load("res/shader/Shape2DShader", "res/shader/Shape2DShader");
		load("Shape2DShader", ShaderProgram::VertFrag);
		compile();
		use();
		projModelLoc = glGetUniformLocation(getGLID(), "projModel");
		colourLoc = glGetUniformLocation(getGLID(), "colour");
		stop();
		return 1;
	}

	void setProjModel(glm::fmat4& pProjModel)
	{
		projModel = pProjModel;
	}

	void setProjModel(glm::fmat4&& pProjModel)
	{
		setProjModel(pProjModel);
	}

	void setColour(glm::fvec4& pColour)
	{
		colour = pColour;
	}

	void setColour(glm::fvec4&& pColour)
	{
		setColour(pColour);
	}

	void sendUniforms()
	{
		glUniformMatrix4fv(projModelLoc, 1, GL_FALSE, glm::value_ptr(projModel));
		glUniform4fv(colourLoc, 1, &colour[0]);
	}

private:
	int projModelLoc;
	int colourLoc;

	glm::fmat4 projModel;
	glm::fvec4 colour;
};

