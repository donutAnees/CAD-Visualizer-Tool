// GameEngineOpenGL.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "GameEngineOpenGL.h"
#include "model.h"
#include "view.h"
#include "controller.h"
#include <iostream>
#include <windows.h>
#include <sstream>
#include <ostream>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib") 

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Initialize MVC
Model model;
View view(&model);
Controller controller(&model, &view);

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ChildWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ObjectDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SidebarDialogProc(HWND, UINT, WPARAM, LPARAM);


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


ATOM RegisterChildWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = ChildWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr; 
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"OpenGLChildWindow";
    wcex.hIconSm = nullptr;

    return RegisterClassExW(&wcex);
}

HWND CreateOpenGLChildWindow(HWND parent, HINSTANCE hInstance) {  
    RECT rect;
    GetClientRect(parent, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    HWND hChildWnd = CreateWindowEx(
        0,                              // Extended styles
        L"OpenGLChildWindow",           // Class name
        nullptr,                        // No title
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, // Child window style
        0, 0, width, height,                 // Position and size
        parent,                         // Parent window
        nullptr,                        // No menu
        hInstance,                      // Instance handle
        nullptr                         // No additional parameters
    );

    if (!hChildWnd) {
        MessageBox(nullptr, L"Failed to create OpenGL child window", L"Error", MB_OK);
    }

    return hChildWnd;
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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

    if (!RegisterChildWindowClass(hInstance)) {
        MessageBox(nullptr, L"Failed to register child window class", L"Error", MB_OK);
        return FALSE;
    }
    HWND hChildWnd = CreateOpenGLChildWindow(hWnd, hInstance);
	if (!hChildWnd) {
		return FALSE;
	}

   HWND hSidebar = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SIDEBAR_DIALOG), hWnd, SidebarDialogProc);
   if (!hSidebar) {
	   return FALSE;
   }

   // Initialize the Controller, this will set the context and run opengl as a separate thread
   if (controller.create(hChildWnd, hWnd, hSidebar)) {
       MessageBox(NULL, L"Failed to set opengl thread", L"Error", MB_OK);
   }
   SetWindowPos(hSidebar, HWND_TOP, 0, 0, 265, 700, SWP_SHOWWINDOW);
   ShowWindow(hChildWnd, nCmdShow);
   UpdateWindow(hChildWnd);
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

    switch (message)
    {
    case WM_KEYDOWN:
		controller.handleKeyboardInput(wParam);
	break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
            {
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            }
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_VIEW_ORTHO:
                break;
            case IDM_VIEW_PERSP:
                break;
            case IDM_OBJECT:
            {
                DialogBox(hInst, MAKEINTRESOURCE(IDD_OBJECT_DIALOG), hWnd, ObjectDialogProc);
            }
                break;
            case ID_CREATE_FILE: {
                controller.createFromFile();
            }
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
    {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
    }
    break;
    case WM_SIZE:
	{
		RECT rect;
		GetClientRect(hWnd, &rect);    
        // Resize the child window to match the parent window's client area
		HWND hChildWnd = FindWindowEx(hWnd, nullptr, L"OpenGLChildWindow", nullptr);
		if (hChildWnd) {
			MoveWindow(hChildWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		}
		float width = static_cast<float>(rect.right - rect.left);
		float height = static_cast<float>(rect.bottom - rect.top);
		controller.resizeWindow(static_cast<int>(width), static_cast<int>(height));
	}
	break;
    case WM_DESTROY:
		PostQuitMessage(0); 
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_MOUSEMOVE:
        controller.handleMouseInput(wParam, LOWORD(lParam), HIWORD(lParam));
        break;
	case WM_LBUTTONDOWN:
		controller.handleMouseDown(wParam, LOWORD(lParam), HIWORD(lParam));
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


INT_PTR CALLBACK ObjectDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        // Initialize the dialog (e.g., populate the combo box)
        SendDlgItemMessage(hDlg, IDC_OBJECT_TYPE, CB_ADDSTRING, 0, (LPARAM)L"Cube");
        SendDlgItemMessage(hDlg, IDC_OBJECT_TYPE, CB_ADDSTRING, 0, (LPARAM)L"Pyramid");
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            // Retrieve user input
            wchar_t objectType[50];
            GetDlgItemText(hDlg, IDC_OBJECT_TYPE, objectType, 50);

            wchar_t posX[10], posY[10], posZ[10], size[10];
            GetDlgItemText(hDlg, IDC_POSITION_X, posX, 10);
            GetDlgItemText(hDlg, IDC_POSITION_Y, posY, 10);
            GetDlgItemText(hDlg, IDC_POSITION_Z, posZ, 10);
            GetDlgItemText(hDlg, IDC_SIZE, size, 10);

            // Convert input to appropriate types
            float x = _wtof(posX);
            float y = _wtof(posY);
            float z = _wtof(posZ);
            float ObjectSize = _wtof(size);

            // Create the object (example: create a cube)
			controller.createDialogHandle(objectType,x,y,z,ObjectSize);

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK SidebarDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        HWND hList = GetDlgItem(hDlg, IDC_OBJECT_LIST);
        for (size_t i = 0; i < model.meshes.size(); ++i) {
            std::wstringstream ss;
            ss << L"Object " << (i + 1);
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)ss.str().c_str());
        }
    }
    return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_OBJECT_LIST && HIWORD(wParam) == LBN_SELCHANGE)
        {
            HWND hList = GetDlgItem(hDlg, IDC_OBJECT_LIST);
            int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < (int)model.meshes.size()) {
                const Mesh& mesh = model.meshes[sel];

                // Rotation
                SetDlgItemInt(hDlg, IDC_ROT_X, (int)mesh.rotationX, TRUE);
                SetDlgItemInt(hDlg, IDC_ROT_Y, (int)mesh.rotationY, TRUE);
                SetDlgItemInt(hDlg, IDC_ROT_Z, (int)mesh.rotationZ, TRUE);

                // Position (center)
                SetDlgItemInt(hDlg, IDC_POS_X, (int)mesh.centerX, TRUE);
                SetDlgItemInt(hDlg, IDC_POS_Y, (int)mesh.centerY, TRUE);
                SetDlgItemInt(hDlg, IDC_POS_Z, (int)mesh.centerZ, TRUE);
            }
        }
        switch (LOWORD(wParam)) {
		case IDC_BTN_BOUNDINGBOX:
			controller.toggleBoundingBox();
			break;
		case IDC_BTN_VERTICES:
			controller.toggleVertices();
			break;
        }
        // Handle property edit box changes
        if (HIWORD(wParam) == EN_KILLFOCUS) {
            HWND hList = GetDlgItem(hDlg, IDC_OBJECT_LIST);
            int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < (int)model.meshes.size()) {
                Mesh& mesh = model.meshes[sel];

                // Get new values from edit boxes
                wchar_t buf[32];
 
                // Rotation
                GetDlgItemText(hDlg, IDC_ROT_X, buf, 32);
                mesh.rotationX = (float)_wtof(buf);
                GetDlgItemText(hDlg, IDC_ROT_Y, buf, 32);
                mesh.rotationY = (float)_wtof(buf);
                GetDlgItemText(hDlg, IDC_ROT_Z, buf, 32);
                mesh.rotationZ = (float)_wtof(buf);

                // Position
                GetDlgItemText(hDlg, IDC_POS_X, buf, 32);
                mesh.centerX = (float)_wtof(buf);
                GetDlgItemText(hDlg, IDC_POS_Y, buf, 32);
                mesh.centerY = (float)_wtof(buf);
                GetDlgItemText(hDlg, IDC_POS_Z, buf, 32);
                mesh.centerZ = (float)_wtof(buf);

                // Update the mesh geometry
                mesh.updateMesh();
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}
