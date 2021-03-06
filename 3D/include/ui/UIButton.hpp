#pragma once
#include "UIElement.hpp"
#include "Text.hpp"
#include "UIRectangleShape.hpp"

class UIButton : public UIElement
{
private:
	/// TODO: Colour transitions for all classes 
	/// make a class that takes the address of the colour to change and some parameters like time, start and end colour
	glm::fvec4 backColour;
	glm::fvec4 backHoverColour;
	glm::fvec4 backClickColour;

	Text2D text;
	RectangleShape rect;
	glm::fvec2 border;

public:
	UIButton() : UIElement(Button, nullptr) {}
	UIButton(UIWindow* pParent);
	~UIButton();
	
	void onMouseDown(MouseEvent& pMouseEvent)
	{
		if (onMouseDownImpl)
			onMouseDownImpl(parentWindow, this, pMouseEvent);
		
		setRectColour(backClickColour);
	}

	void onMouseUp(MouseEvent& pMouseEvent)
	{
		if (onMouseUpImpl)
			onMouseUpImpl(parentWindow, this, pMouseEvent);

		if (isHovered())
			setRectColour(backHoverColour);
		else
			setRectColour(backColour);
	}

	void onMouseEnter(MouseEvent& pMouseEvent)
	{
		if (onMouseEnterImpl)
			onMouseEnterImpl(parentWindow, this, pMouseEvent);

		if(isClicked())
			setRectColour(backClickColour);
		else
			setRectColour(backHoverColour);
	}

	void onMouseLeave(MouseEvent& pMouseEvent)
	{
		if (onMouseLeaveImpl)
			onMouseLeaveImpl(parentWindow, this, pMouseEvent);

		if (isClicked())
			setRectColour(backHoverColour);
		else
			setRectColour(backColour);
	}

	void draw()
	{
		rect.setProj(parentWindow->getProj());
		rect.draw();

		text.shader->setProj(parentWindow->getProj());
		auto ident = glm::fmat4();
		text.shader->setModel(ident);
		text.shader->setView(ident);
		text.shader->setFontBinding(12);
		text.draw();
	}

	void setString(std::string&& str)
	{
		text.setString(str);
		updateBounds();
	}

	void setString(std::string& str)
	{
		text.setString(str);
		updateBounds();
	}

	void setPosition(glm::fvec2 pos)
	{
		position = pos;
		updateBounds();
	}

	void setSize(glm::fvec2 pSize)
	{
		size = pSize;
		updateBounds();
	}

	void updateBounds()
	{
		auto b = text.getBoundingBox();
		glm::fvec2 textPosition(0, 0);
		if (text.getGlyphs() != nullptr) {
			textPosition = glm::fvec2(position.x + size.x*0.5, position.y - (text.getGlyphs()->getAscender() - b.height) + size.y*0.5);
		}
		
		text.setPosition(textPosition);
		rect.setBounds(frect(position.x, position.y, size.x, size.y));
	}

	void setRectColour(glm::fvec4 pCol)
	{
		rect.setColour(pCol);
	}

	void setTextColour(glm::fvec3 pCol)
	{
		text.setColour(pCol);
	}

	Text2D& getText()
	{
		return text;
	}

	frect getBounds()
	{
		return rect.getBounds();
		//return frect(position.x, position.y, dimensions.width, dimensions.height);
	}

	glm::ivec2 getPosition() { return rect.getBounds().topLeft; }
	glm::ivec2 getTopLeft() { return getPosition(); }
	glm::ivec2 getSize() { return rect.getBounds().size; }
};

