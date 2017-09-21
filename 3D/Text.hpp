#pragma once
#include "Include.hpp"
#include "Font.hpp"
#include "Transform.hpp"
#include "TextShader.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include "Rect.hpp"
#include "Strings.hpp"

class Text
{
public:
	Text() {}
	~Text() {}


};

class Text2D : public Text
{
public:
	enum Origin { BotLeft, TopLeft, TopRight, BotRight, TopMiddle, BotMiddle, MiddleLeft, MiddleRight, MiddleMiddle };

	class TextStyle
	{
	public:
		TextStyle() {}
		TextStyle(Font* pFont, u16 pCharSize, glm::fvec3 pColour = glm::fvec3(1.f,1.f,1.f) , bool pHasCustomOrigin = false, Text2D::Origin pTextOriginPreDef = Text2D::Origin::TopLeft, glm::ivec2 pTextOrigin = glm::ivec2(0, 0)) 
		: font(pFont), charSize(pCharSize), colour(pColour), hasCustomOrigin(pHasCustomOrigin), textOriginPreDef(pTextOriginPreDef), textOrigin(pTextOrigin) {}

		Font* font;
		u16 charSize;
		Text2D::Origin textOriginPreDef;
		glm::ivec2 textOrigin;
		bool hasCustomOrigin;
		glm::fvec3 colour;
	};

	Text2D()
	{
		font = nullptr;
		initt = false;
		string.expand(64);
		hasCustomOrigin = true;
	}
	~Text2D()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
	}
	void setFont(Font* pFont) { font = pFont; update(); }
	void setCharSize(u8 pSize) { charSize = pSize; update(); }
	void setString(std::string pStr) { string.setToChars(pStr.c_str()); update(); }
	void setString(StringGeneric& pStr) { string.overwrite(pStr); update(); }
	void setPosition(glm::fvec2 pPos) { position = pPos; update(); }
	u8 getCharSize() { return charSize;}
	u16 getHeight() { return boundingBox.height; }
	u16 getMaxHeight() {
		if (!font || (charSize == 0)) { return 0; }
		return font->requestGlyphs(charSize, this)->getHeight();
	}
	glm::fvec2 getPosition() { return position; }
	void setColour(glm::fvec3 pCol) { colour = pCol; }
	glm::fvec3 getColour() { return colour; }
	String<HEAP>& getString() { return string; }
	Font* getFont() { return font; }

	void setStyle(TextStyle& pStyle)
	{
		font = pStyle.font;
		charSize = pStyle.charSize;
		colour = pStyle.colour;
		hasCustomOrigin = pStyle.hasCustomOrigin;
		if (hasCustomOrigin)
		{
			textOrigin = pStyle.textOrigin;
		}
		else
		{
			textOriginPreDef = pStyle.textOriginPreDef;
		}
		update();
	}

	void init()
	{
		if (initt)
			return;

		initt = true;

		//string = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n1234567890!@#$%^&*()_\n-=+[]{};':\"\\|<>,./~`";
		string.setToChars("\0");
		charSize = 85;
		colour = glm::fvec3(1.f, 1.f, 1.f);
		setTextOrigin(TopLeft);

		//shader = new TextShader();
		shader = &Engine::r->textShader;
		shader->use();

		glGenVertexArrays(1, &vao);
		glGenVertexArrays(1, &vaoBBox);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &vboBBox);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		auto posAttrib = glGetAttribLocation(shader->getGLID(), "position");
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(posAttrib);

		auto texAttrib = glGetAttribLocation(shader->getGLID(), "texCoord");
		glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(texAttrib);

		glBindVertexArray(vaoBBox);
		glBindBuffer(GL_ARRAY_BUFFER, vboBBox);

		posAttrib = glGetAttribLocation(shader->getGLID(), "position");
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(posAttrib);

		texAttrib = glGetAttribLocation(shader->getGLID(), "texCoord");
		glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(texAttrib);

		//transform.setTranslation(glm::fvec3(-300, 100, 0));
		//transform.setTranslation(glm::fvec3(0, 500, 0));

		shader->stop();
	}

	void draw()
	{
		glBindVertexArray(vao);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		shader->use();
		shader->setColour(colour);
		font->requestGlyphs(charSize,this)->bind();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDrawArrays(GL_QUADS, 0, vboSize);

		if (Engine::r->config.drawTextBounds)
		{
			glBindVertexArray(vaoBBox);

			glm::fvec4 c(1.f, 0.f, 1.f, 1.f);
			auto s = (Shape2DShader*)Engine::r->shaderStore.getShader(String32("Shape2DShader"));
			s->use();
			s->setColour(c);
			s->sendUniforms();

			glDrawArrays(GL_LINE_LOOP, 0, 4);
		}

		glBindVertexArray(0);
	}

	void setWindowOrigin(Origin pO)
	{
		windowOrigin = pO;
		updateVBO();
	}

	void setWindowSize(glm::ivec2 pSize)
	{	
		windowSize = pSize;
		updateVBO();
	}

	void setTextOrigin(int pOx, int pOy)
	{
		setTextOrigin(glm::ivec2(pOx, pOy));
	}

	void setTextOrigin(glm::ivec2 pO)
	{
		hasCustomOrigin = true;
		textOrigin = pO;
		textOrigin.y = -textOrigin.y;
		updateVBO();
	}

	void setTextOrigin(Text2D::Origin pOrigin)
	{
		updateBoundingBox();
		textOriginPreDef = pOrigin;
		switch (pOrigin)
		{
		case BotLeft:
		{
			setTextOrigin(0, 0);
			break;
		}
		case TopLeft:
		{
			setTextOrigin(0, getMaxHeight());
			break;
		}
		case TopRight:
		{
			setTextOrigin(boundingBox.width, getMaxHeight());
			break;
		}
		case BotRight:
		{
			setTextOrigin(boundingBox.width, 0);
			break;
		}
		case TopMiddle:
		{
			setTextOrigin(boundingBox.width / 2.f, getMaxHeight());
			break;
		}
		case BotMiddle:
		{
			setTextOrigin(boundingBox.width / 2.f, 0);
			break;
		}
		case MiddleLeft:
		{
			setTextOrigin(0, getMaxHeight() / 2.f);
			break;
		}
		case MiddleRight:
		{
			setTextOrigin(boundingBox.width, getMaxHeight() / 2.f);
			break;
		}
		case MiddleMiddle:
		{
			setTextOrigin(boundingBox.width / 2.f, getMaxHeight() / 2.f);
			break;
		}
		}
		hasCustomOrigin = false;
	}

	TextShader* shader;

	frect getBoundingBox()
	{
		return boundingBox;
	}

	void forceUpdate()
	{
		update();
	}

private:

	bool updateOrigin()
	{
		if (!hasCustomOrigin)
		{
			setTextOrigin(textOriginPreDef);
			return true;
		}
		return false;
	}

	void update()
	{
		if (!updateOrigin())
		{
			updateBoundingBox();
			updateVBO();
		}
	}

	void updateBoundingBox()
	{
		boundingBox.zero();
		if (charSize == 0) { return; }
		if (string.getLength() == 0) { return; }
		if (font == nullptr) { return; }

		auto glyphs = font->requestGlyphs(charSize, this); // glyphContainers[0];
		auto glyphsTex = glyphs->getTexture();

		auto p = string.getString();

		glm::ivec2 rgp(0,0);

		boundingBox.zero();
		boundingBox.left = rgp.x;
		boundingBox.top = rgp.y;

		s32 minX, maxX, minY, maxY;
		minX = rgp.x; maxX = rgp.x;
		minY = rgp.y; maxY = rgp.y;

		while (*p != 0)
		{
			if (*p == '\n')
			{
				rgp.x = 0;
				rgp.y -= charSize;
			}
			else
			{
				auto glyphPos = rgp + glyphs->getPosition(*p);
				auto glyphSize = glyphs->getSize(*p);
				auto glyphCoords = glyphs->getCoords(*p);

				if (glyphPos.x < minX)
					minX = glyphPos.x;
				if (glyphPos.y - glyphSize.y < minY)
					minY = glyphPos.y - glyphSize.y;

				if (glyphPos.y > maxY)
					maxY = glyphPos.y;
				if (glyphPos.x + glyphSize.x > maxX)
					maxX = glyphPos.x + glyphSize.x;

				rgp.x += glyphs->getAdvance(*p).x;
				rgp.y += glyphs->getAdvance(*p).y;
			}
			++p;
		}

		boundingBox.left = minX;
		boundingBox.top = minY;
		boundingBox.width = maxX - minX;
		boundingBox.height = maxY - minY;
	}

	void updateVBO()
	{
		boundingBox.zero(); vboSize = 0;
		if (charSize == 0) { return; }
		if (string.getLength() == 0) {  return; }
		if (font == nullptr) { return; }

		int vboNumQuad = string.getLength();
		int vboNumVert = vboNumQuad * 4;
		int sizeOfVert = 5;
		vboSize = vboNumVert;

		int vboSizeFloats = vboNumVert * sizeOfVert;
		int vboSizeChar = vboNumVert * sizeOfVert * sizeof(float);
		float* vertData = new float[vboSizeFloats];

		auto glyphs = font->requestGlyphs(charSize, this); // glyphContainers[0];
		auto glyphsTex = glyphs->getTexture();

		int index = 0;
		auto p = string.getString();

		auto tpos = position - glm::fvec2(textOrigin);

		glm::ivec2 rgp(tpos);

		switch (windowOrigin)
		{
		case(BotLeft) :

			break;
		case(TopLeft) :
			rgp.y = windowSize.y - rgp.y;
			tpos.y = rgp.y;
			break;
		case(TopRight) :
			rgp.x = windowSize.x - rgp.x;
			rgp.y = windowSize.y - rgp.y;
			tpos.y = rgp.y;
			tpos.x = rgp.x;
			break;
		case(BotRight) :
			rgp.x = windowSize.x - rgp.x;
			tpos.x = rgp.x;
			break;
		}

		boundingBox.zero();
		boundingBox.left = rgp.x;
		boundingBox.top = rgp.y;
		
		s32 minX, maxX, minY, maxY;
		minX = rgp.x; maxX = rgp.x;
		minY = rgp.y; maxY = rgp.y;

		while (*p != 0)
		{
			if (*p == '\n')
			{
				rgp.x = tpos.x;
				rgp.y -= charSize;
			}
			else
			{
				glm::uvec2 glyphPos = rgp + glyphs->getPosition(*p);
				glm::uvec2 glyphSize = glyphs->getSize(*p);
				glm::fvec2 glyphCoords = glyphs->getCoords(*p);

				auto cOffset = glm::fvec2(glyphSize) / glm::fvec2(glyphsTex->getWidth(), glyphsTex->getHeight());

				GLfloat vertices[] = {
					//Position													Texcoords
					glyphPos.x, glyphPos.y, 0,									glyphCoords.x, glyphCoords.y, // Bot-left
					glyphPos.x + glyphSize.x, glyphPos.y, 0,					glyphCoords.x + cOffset.x, glyphCoords.y, // Bot-right
					glyphPos.x + glyphSize.x, glyphPos.y - glyphSize.y, 0,		glyphCoords.x + cOffset.x, glyphCoords.y + cOffset.y,  // Top-right
					glyphPos.x, glyphPos.y - glyphSize.y, 0,					glyphCoords.x, glyphCoords.y + cOffset.y // Top-left
				};
				memcpy((char*)(vertData)+(sizeof(vertices) * index), vertices, sizeof(vertices));
						
				if (glyphPos.x < minX) 
					minX = glyphPos.x;
				if (glyphPos.y - glyphSize.y < minY) 
					minY = glyphPos.y - glyphSize.y;

				if (glyphPos.y > maxY)
					maxY = glyphPos.y;
				if (glyphPos.x + glyphSize.x > maxX) 
					maxX = glyphPos.x + glyphSize.x;

				rgp.x += glyphs->getAdvance(*p).x;
				rgp.y += glyphs->getAdvance(*p).y;
			}
			++p;
			++index;
		}

		boundingBox.left = minX;
		boundingBox.top = minY;
		boundingBox.width = maxX - minX;
		boundingBox.height = maxY - minY;

		//updateOrigin();

		const auto bb = boundingBox;

		float bbDataTmp[] = { bb.left, bb.top, 0.f, 0.f,0.f,
								bb.left + bb.width, bb.top, 0.f,0.f,0.f,
								bb.left + bb.width, bb.top + bb.height,0.f,0.f,0.f,
								bb.left,bb.top + bb.height,0.f,0.f,0.f };

		glNamedBufferData(vboBBox, sizeof(bbDataTmp), bbDataTmp, GL_STATIC_DRAW);



		glNamedBufferData(vbo, vboSizeChar, vertData, GL_STATIC_DRAW);

		

		delete[] vertData;
	}

protected:

	Origin windowOrigin;
	glm::ivec2 windowSize;

	Origin textOriginPreDef;
	bool hasCustomOrigin;
	glm::ivec2 textOrigin;
	glm::fvec2 position;
	glm::fvec2 origin;
	float rotation;
	frect boundingBox;
	glm::fvec3 colour;
	bool initt;

	glm::fvec2 relGlyphPos;
	GLuint vboBBox;
	GLuint vbo;
	int vboSize;
	GLuint vao;
	GLuint vaoBBox;
	//std::string string;
	String<HEAP> string;
	u16 charSize;
	Font* font;
};
