// GameEngineOpenGL.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "GameEngineOpenGL.h"
#include "rendering.h"
#include "camera.h"
#include <iostream>
#include <windows.h>
#include <sstream>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib") 


#ifndef VK_W  
#define VK_W 0x57 
#endif  

#ifndef VK_S  
#define VK_S 0x53 
#endif  

#ifndef VK_A  
#define VK_A 0x41
#endif  

#ifndef VK_D  
#define VK_D 0x44 
#endif
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HDC hdc;                                        // Handle to device context
HGLRC hglrc;                                    // Handle to OpenGL rendering context
Camera camera;
LARGE_INTEGER frequency;  // This will hold the frequency of the high-resolution timer
LARGE_INTEGER lastTime;   // This will store the last frame time
float fps = 0.0f;         // Variable to store FPS
bool isContextClicked = false;
unsigned int mode = PERSPECTIVE_MODE;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GAMEENGINEOPENGL, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMEENGINEOPENGL));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMEENGINEOPENGL));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_GAMEENGINEOPENGL);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // Get the handle to the device context for the window
   hdc = GetDC(hWnd);

   if (hdc == NULL)
   {
	   MessageBox(NULL, L"GetDC failed", L"Error", MB_OK);
	   return FALSE;
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
   if ((pixelFormat = ChoosePixelFormat(hdc, &pfd)) == 0)
   {
	   MessageBox(NULL, L"ChoosePixelFormat failed", L"Error", MB_OK);
	   return FALSE;
   }

   if (SetPixelFormat(hdc, pixelFormat, &pfd) == FALSE)
   {
	   MessageBox(NULL, L"SetPixelFormat failed", L"Error", MB_OK);
	   return FALSE;
   }

   // Create the rendering context
   hglrc = wglCreateContext(hdc);
   wglMakeCurrent(hdc, hglrc);

   QueryPerformanceFrequency(&frequency);  // Get the frequency of the high-res timer
   QueryPerformanceCounter(&lastTime);

   camera = Camera();
   initializeCubeMesh();

   // To enable depth testing, which can be used to determine which objects, or parts of objects, are visible
   glEnable(GL_DEPTH_TEST);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);  // Get the current timestamp

    // Calculate time difference (in seconds)
    float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;

    lastTime = currentTime;  // Update the lastTime for the next frame

    // Calculate FPS based on the delta time
    fps = 1.0f / deltaTime;

    switch (message)
    {
    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE) {
            isContextClicked = false;
            ShowCursor(TRUE);
        }
        if (wParam == VK_W)
            camera.processKeyboard(FORWARD, deltaTime);
        if (wParam == VK_S)
            camera.processKeyboard(BACKWARD, deltaTime);
        if (wParam == VK_A)
            camera.processKeyboard(LEFT, deltaTime);
        if (wParam == VK_D)
            camera.processKeyboard(RIGHT, deltaTime);
    }
    break;
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT) {
            // cursor hit opengl context window
            return HTCLIENT;
        }
        return hit;
    }
    break;
    case WM_LBUTTONDOWN: 
    {
        isContextClicked = true;
        ShowCursor(FALSE);
    }
    break;
    case WM_MOUSEMOVE:
    {
        if (!isContextClicked) break;

        static bool firstMouse = true;
        static float lastX = 0.0f, lastY = 0.0f;

        // Get the window's client area dimensions
        RECT rect;
        GetClientRect(hWnd, &rect);
        int centerX = (rect.right - rect.left) / 2;
        int centerY = (rect.bottom - rect.top) / 2;

        // Get the current cursor position
        POINT cursorPos;
        GetCursorPos(&cursorPos);

        if (firstMouse) {
            lastX = static_cast<float>(cursorPos.x);
            lastY = static_cast<float>(cursorPos.y);
            firstMouse = false;
        }

        // Calculate the offset
        float xoffset = static_cast<float>(cursorPos.x - lastX);
        float yoffset = static_cast<float>(lastY - cursorPos.y); // Reversed since y-coordinates go from bottom to top

        // Process the mouse movement
        camera.ProcessMouseMovement(xoffset, yoffset);

        // Recenter the cursor
        SetCursorPos(centerX, centerY);

        // Update the last position to the center
        lastX = static_cast<float>(centerX);
        lastY = static_cast<float>(centerY);
    }
    break;
    case WM_MOUSEWHEEL: 
    {
        float zoomOffset = static_cast<float> (GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        camera.ProcessMouseScroll(zoomOffset);
    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_VIEW_ORTHO:
                mode = ORTHOGRAPHIC_MODE;
                break;
            case IDM_VIEW_PERSP:
                mode = PERSPECTIVE_MODE;
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
        RECT rect;
        GetClientRect(hWnd, &rect); 
        float width = static_cast<float>(rect.right - rect.left);
        float height = static_cast<float>(rect.bottom - rect.top);
        render(width, height, camera, mode);
        // Swap buffers
        SwapBuffers(hdc);
        }
        break;
	case WM_SIZE:
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		float width = static_cast<float>(rect.right - rect.left);
		float height = static_cast<float>(rect.bottom - rect.top);
		glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	}
	break;
    case WM_DESTROY:
        if (hglrc = wglGetCurrentContext()) {
            // obtain its associated device context  
            hdc = wglGetCurrentDC();

            // make the rendering context not current  
            wglMakeCurrent(NULL, NULL);

            // release the device context  
            ReleaseDC(hWnd, hdc);

            // delete the rendering context  
            wglDeleteContext(hglrc);
        }
        ShowCursor(TRUE);
		PostQuitMessage(0); 
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
