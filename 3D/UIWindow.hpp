#pragma once
#include "UIUtil.hpp"
#include "Framebuffer.hpp"
#include "Shape2DShader.hpp"
#include "Window.hpp"
#include "Event.hpp"
#include "Rect.hpp"
#include <unordered_map>

class Font;
class UIElement;
class UILabel;

class UIWindow
{
public:
	UIWindow(irect pWindowArea, int pBorderWidth, const Window* pParentWindow);
	~UIWindow();

	void draw();
	void update();
	void mouseDown(MouseEvent& pMouseEvent);
	void mouseUp(MouseEvent& pMouseEvent);
	void keyDown(KeyEvent& pMouseEvent);
	void keyUp(KeyEvent& pMouseEvent);
	void checkMouseEnter(MouseEvent & pMouseEvent);
	const GLTexture2D& getTexture() { return renderTarget.textureAttachments[0]; }

	void setBorderWidth(u32 pWidth)
	{
		borderWidth = pWidth;
		updateWindowVBO();
	}
	void setTitle(std::string pTitle);
	void addElement(UIElement* pEl);

	glm::fmat4& getProj() { return proj; }

	void resizeWindow(u32 newX, u32 newY)
	{
		windowArea.width = newX;
		windowArea.height = newY;
	}

	void updateWindowVBO();

	irect getWindowArea() { return windowArea; }
	irect getWindowRect() { return irect(windowArea.left, windowArea.top, windowArea.width, windowArea.height); }

	void setWindowPosition(glm::ivec2 pos)
	{
		if (!movable)
			return;
		windowArea.left = pos.x;
		windowArea.top = pos.y;
		updateWindowVBO();
	}

	void setMovable(bool pMovable)
	{
		movable = pMovable;
	}

//private:

	irect windowArea;
	irect elementArea;
	int borderWidth;
	int titleWidth;
	bool movable;
	bool dragging;
	glm::ivec2 clickedPos;
	bool hasTitle;
	bool hasFocus;

	Framebuffer renderTarget;

	std::unordered_map<std::string,UIElement*> elements;
	UILabel* title;

	Shape2DShader* shader;

	GLuint vao;
	GLuint vbo;

	glm::fmat4 proj;
	const Window* parentWindow;
};
