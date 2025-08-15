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

// To track the selected mesh and position from context menu
int g_selectedMeshIdx = -1;
POINT g_clickPoint;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ChildWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ObjectDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    PropertiesDialogProc(HWND, UINT, WPARAM, LPARAM);


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

   // Initialize the Controller with NULL for sidebar (we don't use it anymore)
   if (controller.create(hChildWnd, hWnd, NULL)) {
       MessageBox(NULL, L"Failed to set opengl thread", L"Error", MB_OK);
   }
   
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
                model.camera.setCameraMode(ORTHOGRAPHIC_MODE);
                model.updateProjection(view.getWindowWidth(), view.getWindowHeight());
                break;
            case IDM_VIEW_PERSP:
                model.camera.setCameraMode(PERSPECTIVE_MODE);
                model.updateProjection(view.getWindowWidth(), view.getWindowHeight());
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
            // Context menu commands
            case IDM_CONTEXT_BOUNDINGBOX:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < (int)model.meshes.size()) {
                    model.meshes[g_selectedMeshIdx].toggleBoundingBox();
                }
                break;
            case IDM_CONTEXT_VERTICES:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < (int)model.meshes.size()) {
                    model.meshes[g_selectedMeshIdx].toggleVertices();
                }
                break;
            case IDM_CONTEXT_DELETE:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < (int)model.meshes.size()) {
                    model.meshes.erase(model.meshes.begin() + g_selectedMeshIdx);
                    g_selectedMeshIdx = -1; // Reset selection
                }
                break;
            case IDM_CONTEXT_ORBIT:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < (int)model.meshes.size()) {
                    if (model.camera.isOrbitMode()) {
                        model.camera.setOrbitMode(false);
                    } else {
                        model.camera.setOrbitMode(true, model.meshes[g_selectedMeshIdx].getCenter(), 10.0f);
                    }
                }
                break;
            case IDM_CONTEXT_FIT_TO_VIEW:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < (int)model.meshes.size()) {
                    model.camera.zoomToBoundingBox(
                        model.meshes[g_selectedMeshIdx].getCenter(),
                        model.meshes[g_selectedMeshIdx].getSize(),
                        view.getAspectRatio()
                    );
                }
                break;
            case IDM_CONTEXT_EDIT_PROPERTIES:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < (int)model.meshes.size()) {
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_PROPERTIES_DIALOG), hWnd, PropertiesDialogProc);
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

// Test ray intersection and return the index of the selected mesh
int GetSelectedMeshIndex(int x, int y) {
    // Get camera matrices and window size
    glm::mat4 viewMatrix = model.camera.getViewMatrix();
    glm::mat4 projMatrix = model.getProjectionMatrix();
    int width = view.getWindowWidth();
    int height = view.getWindowHeight();

    // Convert screen point to ray
    glm::vec3 rayOrigin, rayDir;
    controller.screenPointToRay(x, y, width, height, viewMatrix, projMatrix, model.camera, rayOrigin, rayDir);

    // Create ray and do intersection test
    Ray ray(rayOrigin, rayDir, 0.0f, 100.0f);
    std::vector<Face*> hitFaces;
    model.bvh.traverse(model.bvh.getRoot(), ray, hitFaces);
    
    if (!hitFaces.empty()) {
        Face* firstHit = hitFaces[0];
        int meshIndex = -1;
        
        // Find which mesh owns this face
        for (size_t m = 0; m < model.meshes.size(); ++m) {
            for (size_t f = 0; f < model.meshes[m].faces.size(); ++f) {
                if (&model.meshes[m].faces[f] == firstHit) {
                    meshIndex = (int)m;
                    break;
                }
            }
            if (meshIndex != -1) break;
        }
        
        return meshIndex;
    }
    
    return -1;
}

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONDOWN: {
        controller.handleMouseDown(wParam, LOWORD(lParam), HIWORD(lParam));
        SetFocus(GetParent(hWnd)); // Give focus to the parent window
    }
        break;
    case WM_RBUTTONDOWN: {
        // Store click position for the context menu
        g_clickPoint.x = LOWORD(lParam);
        g_clickPoint.y = HIWORD(lParam);
        
        // Determine which mesh (if any) is at the click position
        g_selectedMeshIdx = GetSelectedMeshIndex(g_clickPoint.x, g_clickPoint.y);
        
        // Only show context menu if an object was clicked
        if (g_selectedMeshIdx >= 0) {
            // Convert to screen coordinates
            POINT pt = g_clickPoint;
            ClientToScreen(hWnd, &pt);
            
            HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_MENU));
            if (hMenu) {
                HMENU hSubMenu = GetSubMenu(hMenu, 0);
                // Show popup menu
                TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                              pt.x, pt.y, 0, GetParent(hWnd), NULL);
                DestroyMenu(hMenu);
            }
        }
    }
        break;
    case WM_MOUSEMOVE:
        controller.handleMouseInput(wParam, LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_MOUSEWHEEL: {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0) {
            controller.zoomIn();
        }
        else if (delta < 0) {
            controller.zoomOut();
        }
        break;
    }
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

// Dialog procedure for the properties dialog
INT_PTR CALLBACK PropertiesDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < (int)model.meshes.size())
        {
            const Mesh& mesh = model.meshes[g_selectedMeshIdx];
            
            // Initialize dialog with mesh values
            // Convert float values to strings and set them in the dialog
            wchar_t buffer[32];
            
            // Rotation
            swprintf_s(buffer, L"%.2f", mesh.rotationX);
            SetDlgItemText(hDlg, IDC_PROP_ROT_X, buffer);
            swprintf_s(buffer, L"%.2f", mesh.rotationY);
            SetDlgItemText(hDlg, IDC_PROP_ROT_Y, buffer);
            swprintf_s(buffer, L"%.2f", mesh.rotationZ);
            SetDlgItemText(hDlg, IDC_PROP_ROT_Z, buffer);
            
            // Position
            swprintf_s(buffer, L"%.2f", mesh.centerX);
            SetDlgItemText(hDlg, IDC_PROP_POS_X, buffer);
            swprintf_s(buffer, L"%.2f", mesh.centerY);
            SetDlgItemText(hDlg, IDC_PROP_POS_Y, buffer);
            swprintf_s(buffer, L"%.2f", mesh.centerZ);
            SetDlgItemText(hDlg, IDC_PROP_POS_Z, buffer);
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < (int)model.meshes.size())
            {
                Mesh& mesh = model.meshes[g_selectedMeshIdx];
                
                // Get values from dialog
                wchar_t buffer[32];
                
                // Rotation
                GetDlgItemText(hDlg, IDC_PROP_ROT_X, buffer, 32);
                mesh.rotationX = (float)_wtof(buffer);
                GetDlgItemText(hDlg, IDC_PROP_ROT_Y, buffer, 32);
                mesh.rotationY = (float)_wtof(buffer);
                GetDlgItemText(hDlg, IDC_PROP_ROT_Z, buffer, 32);
                mesh.rotationZ = (float)_wtof(buffer);
                
                // Position
                GetDlgItemText(hDlg, IDC_PROP_POS_X, buffer, 32);
                mesh.centerX = (float)_wtof(buffer);
                GetDlgItemText(hDlg, IDC_PROP_POS_Y, buffer, 32);
                mesh.centerY = (float)_wtof(buffer);
                GetDlgItemText(hDlg, IDC_PROP_POS_Z, buffer, 32);
                mesh.centerZ = (float)_wtof(buffer);
                
                // Update the mesh
                mesh.updateMesh();
            }
            
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
