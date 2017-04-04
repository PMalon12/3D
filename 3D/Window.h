#pragma once
//#include "SDL.h"
#include "Types.h"
#include <iostream>
#include <string>
#include <sstream>

#include <GL\glew.h>
#include <GL\wglew.h>
#include <glm\glm.hpp>
#include "glm\gtc\type_ptr.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include <time.h>
#include <Windows.h>
#include "resource.h"
#include <SDKDDKVer.h>
#include "SOIL.h"


#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <strsafe.h>

#include "Keyboard.h"
#include "Mouse.h"
#include "UserInputManager.h"

#include "StringGenerics.h"
#include "Event.h"

#define MM_CREATE_GL_CONTEXT 3001

class Window
{
public:
	Window() : windowCreated(false) {}
	~Window() {}

	static Window* engineWindow;

	BOOL processMessages();
	void postMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{
		PostMessage(windowHandle, message, wParam, lParam);
	}

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void setUpNewWindow()
	{

	}
	void deleteWindow();
	void createWindow(HINSTANCE pHInstance, LPCTSTR pWindowName, int pXPos, int pYPos, int pWidth, int pHeight, DWORD pWindowStyle);
	void createGLContext();

	bool hasFocus(){
		return windowHasFocus;}

	void escapePressed(){
		PostMessage(windowHandle, WM_CLOSE, 0, 0);}

	void setResolution(glm::ivec2 pRes){
		desiredClientArea = pRes;
		forceClientAreaToDesired();}

	RECT getClientArea(){
		RECT wr2;
		auto er2 = GetClientRect(windowHandle, &wr2); return wr2;}

	void forceClientAreaToDesired();
	bool isMouseInClientArea();
	void captureMouseFocus(){
		windowHasFocus = true;}

	void registerInputDevices();
	void screenshot();
	void screenshot(StringGeneric& fileName);

	inline void swapBuffers() const {
		wglSwapLayerBuffers(deviceContextHandle, WGL_SWAP_MAIN_PLANE);}

	inline s32 getSizeX() const { return actualClientArea.x; }
	inline s32 getSizeY() const { return actualClientArea.y; } 
	inline s32 getPosX() const { return posX; }
	inline s32 getPosY() const { return posY; }
	inline glm::ivec2 getSize() const { return glm::ivec2(actualClientArea); }

//private:

	bool windowCreated;
	
	glm::ivec2 desiredClientArea;
	glm::ivec2 actualClientArea;
	s32 posX, posY;

	HGLRC glContextHandle;
	HWND windowHandle;
	WNDCLASS windowClass;
	HDC deviceContextHandle;
	KeyboardState keyboard;
	MouseState mouse;
	
	EventQ eventQ;

	bool windowHasFocus;
};


