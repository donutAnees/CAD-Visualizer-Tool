#pragma once
#include "model.h"
#include "view.h"
#include <windows.h>
#include <thread>

class Controller {
public:
	Model* model;
	View* view;
	std::thread thread;
	bool loopThreadFlag;
	HWND handle;
	HWND parentHandle;
	int mouseX;
	int mouseY;

	Controller(Model* model, View* view) : model(model), view(view), mouseX(0), mouseY(0) {

	}

	int create(HWND handle, HWND parentHandle) {
		this->handle = handle;
		this->parentHandle = parentHandle;
		// Create a context
		if (!view->setContext(handle)) {
			MessageBox(NULL, L"Failed to set context", L"Error", MB_OK);
			return 1;
		}
		// Create a separate thread for the OpenGL context
		thread = std::thread(&Controller::runThread, this);
		loopThreadFlag = true;
		return 1;
	}

	void runThread() {
		wglMakeCurrent(view->getHdc(), view->getHglrc());
		model->init();
		RECT rect;    
		while (loopThreadFlag) {
		GetClientRect(handle, &rect);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		view->setWindowSize(width, height);
		view->render();
		view->swapBuffer();
		}
		view->closeContext(handle);
		wglMakeCurrent(NULL, NULL);
	}

	void resizeWindow(int width, int height) {
		view->setWindowSize(width, height);
	}

	void handleKeyboardInput(WPARAM wParam) {
		switch (wParam) {
		case 'W':
			model->camera.move(FORWARD, 0.01f);
			break;
		case 'S':
			model->camera.move(BACKWARD, 0.01f);
			break;
		case 'A':
			model->camera.move(LEFT, 0.01f);
			break;
		case 'D':
			model->camera.move(RIGHT, 0.01f);
			break;
		case VK_ESCAPE:
			loopThreadFlag = false; 
			break;
		default:
			break;
		}
	}

	void handleMouseInput(WPARAM state, float x, float y) {
		if (state == MK_LBUTTON)
		{
			float mouseDeltaX = (x - mouseX);
			float mouseDeltaY = (y - mouseY);
			model->camera.rotateBy(mouseDeltaX, mouseDeltaY);
			mouseX = x;
			mouseY = y;
		}
	}
	
	void handleMouseDown(WPARAM state, float x, float y) {
		mouseX = x;
		mouseY = y;
	}

	void createDialogHandle(wchar_t* objectType, int x, int y, int z, int size) {
		if (wcscmp(objectType, L"Cube") == 0)
		{
			model->createCube(x,y,z,size);
		}
		else if (wcscmp(objectType, L"Pyramid") == 0)
		{
			model->createPyramid(x, y, z, size);
		}
		else
		{
			MessageBox(parentHandle, L"Invalid object type", L"Error", MB_OK);
		}
	}
};
