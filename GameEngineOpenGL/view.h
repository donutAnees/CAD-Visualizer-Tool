#pragma once

#include <windows.h>
#include "model.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <sstream>
#include <iostream>
#include "grid.h"


// View is responsible for rendering the visual content to the screen

class View {
private:
	HDC hdc; // Handle to device context
	HGLRC hglrc; // Handle to OpenGL rendering context
	Model* model; // Pointer to the model class
	int screenWidth;
	int screenHeight;
	bool isScreenResized;

public: 
	// Constructor to initialize the handles to NULL
	View(Model* model) : hdc(NULL), hglrc(NULL), screenWidth(0), screenHeight(0), isScreenResized(FALSE), model(model) {
		
	}

	void setViewPort(int x, int y, int width, int height) {
		glViewport((GLint)x,(GLint) y, (GLsizei)width, (GLsizei)height);
	}

	float getAspectRatio() {
		return  (float)this->screenWidth / (float)this->screenHeight;
	}
	
	int getWindowWidth() {
		return screenWidth;
	}

	int getWindowHeight() {
		return screenHeight;
	}

	// Function to set the device context and rendering context
	int setContext(HWND hWnd) {
		this->hdc = GetDC(hWnd);

		if (this->hdc == NULL)
		{
			MessageBox(NULL, L"GetDC failed", L"Error", MB_OK);
			return 0;
		}

		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
			PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
			32,                   // Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                   // Number of bits for the depthbuffer
			8,                    // Number of bits for the stencilbuffer
			0,                    // Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		int pixelFormat;
		if ((pixelFormat = ChoosePixelFormat(this->hdc, &pfd)) == 0)
		{
			MessageBox(NULL, L"ChoosePixelFormat failed", L"Error", MB_OK);
			ReleaseDC(hWnd, this->hdc);
			return 0;
		}

		if (SetPixelFormat(this->hdc, pixelFormat, &pfd) == FALSE)
		{
			MessageBox(NULL, L"SetPixelFormat failed", L"Error", MB_OK);
			ReleaseDC(hWnd, this->hdc);
			return 0;
		}

		// Create the rendering context
		this->hglrc = wglCreateContext(this->hdc);
		if (this->hglrc == NULL)
		{
			MessageBox(NULL, L"wglCreateContext failed", L"Error", MB_OK);
			ReleaseDC(hWnd, this->hdc);
			return 0;
		}
		return 1;
	}

	int closeContext(HWND handle) {
		if (hglrc) {
			wglMakeCurrent(hdc, NULL);
			wglDeleteContext(hglrc);
			hglrc = NULL;
		}
		if (hdc) {
			ReleaseDC(handle, hdc);
			hdc = NULL;
		}
		return 1;
	}

	// Function to get the device context
	HDC getHdc() {
		return hdc;
	}

	// Function to get the rendering context
	HGLRC getHglrc() {
		return hglrc;
	}

	void swapBuffer() {
		// Swap the OpenGL buffers
		SwapBuffers(hdc);
	}

	void setWindowSize(int widht, int height) {
		screenWidth = widht;
		screenHeight = height;
		isScreenResized = TRUE;
	}

	void preRender() {
		if (isScreenResized) {
			glViewport(0, 0, screenWidth, screenHeight);
			model->updateProjection(screenWidth, screenHeight);
			isScreenResized = FALSE;
		}
	}

	void render() {
		preRender();
		if (model) {
			model->draw(screenWidth, screenHeight);
		}
	}
};
