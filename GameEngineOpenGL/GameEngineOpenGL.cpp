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
int g_selectedFaceIdx = -1;
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
                model.setCameraMode(ORTHOGRAPHIC_MODE);
                model.updateProjection(view.getWindowWidth(), view.getWindowHeight());
                break;
            case IDM_VIEW_PERSP:
                model.setCameraMode(PERSPECTIVE_MODE);
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
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < static_cast<int>(model.getMeshCount())) {
                    controller.toggleBoundingBox();
                }
                break;
            case IDM_CONTEXT_VERTICES:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < static_cast<int>(model.getMeshCount())) {
                    controller.toggleVertices();
                }
                break;
            case IDM_CONTEXT_DELETE:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < static_cast<int>(model.getMeshCount())) {
                    model.deleteMesh(g_selectedMeshIdx);
                    controller.clearAllSelections();
                    g_selectedMeshIdx = -1;
                    g_selectedFaceIdx = -1;
                }
                break;
            case IDM_CONTEXT_ORBIT:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < static_cast<int>(model.getMeshCount())) {
                    Mesh selectedMeshCopy;
                    if (model.getMeshProperties(g_selectedMeshIdx, selectedMeshCopy)) {
                        if (model.isCameraOrbitMode()) {
                            model.setCameraOrbitMode(false);
                        } else {
                            model.setCameraOrbitMode(true, selectedMeshCopy.getCenter(), 10.0f);
                        }
                    }
                }
                break;
            case IDM_CONTEXT_FIT_TO_VIEW:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < static_cast<int>(model.getMeshCount())) {
                    Mesh selectedMeshCopy;
                    if (model.getMeshProperties(g_selectedMeshIdx, selectedMeshCopy)) {
                        model.zoomCameraToBoundingBox(
                            selectedMeshCopy.getCenter(),
                            selectedMeshCopy.getSize(),
                            view.getAspectRatio()
                        );
                    }
                }
                break;
            case IDM_CONTEXT_EDIT_PROPERTIES:
                if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < static_cast<int>(model.getMeshCount())) {
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
void GetSelectedIndices(int x, int y, int& outMeshIndex, int& outFaceIndex) {
    // Get camera and projection data in thread-safe manner
    auto camResult = model.getCameraThreadSafe();
    auto& camLock = camResult.first;
    Camera& camera = camResult.second;
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 projMatrix = model.getProjectionMatrix();
    int width = view.getWindowWidth();
    int height = view.getWindowHeight();

    // Convert screen point to ray
    glm::vec3 rayOrigin, rayDir;
    controller.screenPointToRay(x, y, width, height, viewMatrix, projMatrix, camera, rayOrigin, rayDir);

    // Set appropriate ray range based on camera mode
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();
    // For orthographic mode, use a large ray range since we could have objects in negative space
    if (camera.mode == ORTHOGRAPHIC_MODE) {
        tMin = -std::numeric_limits<float>::max(); // Allow intersections behind the ray origin
        tMax = std::numeric_limits<float>::max();
    }
    // Create ray with proper parameters
    Ray ray(rayOrigin, rayDir, tMin, tMax);
    // Debug output
    std::wstringstream ss;
    ss << L"[GetSelectedIndices] Ray: Origin (" << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z 
       << ") Dir (" << rayDir.x << ", " << rayDir.y << ", " << rayDir.z
       << ") tMin: " << tMin << " tMax: " << tMax << "\n";
    OutputDebugString(ss.str().c_str());
    controller.findRayIntersection(ray, outMeshIndex, outFaceIndex);
    // Debug Selected Mesh and Face Index
    ss.str(L"");
    ss << L"[GetSelectedIndices] OutMeshIndex: " << outMeshIndex << ", OutFaceIndex: " << outFaceIndex << "\n";
    OutputDebugString(ss.str().c_str());
}

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONDOWN: {
        controller.handleMouseDown(wParam, LOWORD(lParam), HIWORD(lParam));
        
        // Update global selection state to match controller's state
        g_selectedMeshIdx = controller.selectedMeshIndex;
        g_selectedFaceIdx = controller.selectedFaceIndex;
        
        SetFocus(GetParent(hWnd)); // Give focus to the parent window
    }
        break;
    case WM_RBUTTONDOWN: {
        // Store click position for the context menu
        g_clickPoint.x = LOWORD(lParam);
        g_clickPoint.y = HIWORD(lParam);
        
        OutputDebugString(L"WM_RBUTTONDOWN event received\n");
        
        // Determine which mesh (if any) is at the click position
        GetSelectedIndices(g_clickPoint.x, g_clickPoint.y, g_selectedMeshIdx, g_selectedFaceIdx);
        
        // Debug output
        std::wstringstream ss;
        ss << L"Right-click detected at (" << g_clickPoint.x << ", " << g_clickPoint.y 
           << "), selected mesh index: " << g_selectedMeshIdx 
           << ", selected face index: " << g_selectedFaceIdx << "\n";
        OutputDebugString(ss.str().c_str());
        
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
            } else {
                OutputDebugString(L"Failed to load context menu resource\n");
            }
        } else {
            OutputDebugString(L"No object selected, context menu not shown\n");
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
        SendDlgItemMessage(hDlg, IDC_OBJECT_TYPE, CB_ADDSTRING, 0, (LPARAM)L"Circle");
        SendDlgItemMessage(hDlg, IDC_OBJECT_TYPE, CB_ADDSTRING, 0, (LPARAM)L"Cylinder");
        SendDlgItemMessage(hDlg, IDC_OBJECT_TYPE, CB_ADDSTRING, 0, (LPARAM)L"Sphere");
        SendDlgItemMessage(hDlg, IDC_OBJECT_TYPE, CB_ADDSTRING, 0, (LPARAM)L"Cone");
        SendDlgItemMessage(hDlg, IDC_OBJECT_TYPE, CB_ADDSTRING, 0, (LPARAM)L"Torus");
        SendDlgItemMessage(hDlg, IDC_OBJECT_TYPE, CB_ADDSTRING, 0, (LPARAM)L"Plane");
        ShowWindow(GetDlgItem(hDlg, IDC_SIZE), SW_HIDE); // Hide the size input field
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            // Retrieve user input
            wchar_t objectType[50];
            GetDlgItemText(hDlg, IDC_OBJECT_TYPE, objectType, 50);

            wchar_t posX[10], posY[10], posZ[10];
            GetDlgItemText(hDlg, IDC_POSITION_X, posX, 10);
            GetDlgItemText(hDlg, IDC_POSITION_Y, posY, 10);
            GetDlgItemText(hDlg, IDC_POSITION_Z, posZ, 10);

            // Convert input to appropriate types
            float x = _wtof(posX);
            float y = _wtof(posY);
            float z = _wtof(posZ);

            // Create the object
            controller.createDialogHandle(objectType, x, y, z);

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
        if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < static_cast<int>(model.getMeshCount()))
        {
            Mesh mesh;
            if (model.getMeshProperties(g_selectedMeshIdx, mesh))
            {
                // Initialize dialog with mesh values
                wchar_t buffer[64];
                
                // Object identity
                SetDlgItemTextA(hDlg, IDC_PROP_NAME, mesh.objectName.c_str());
                SetDlgItemTextA(hDlg, IDC_PROP_TYPE, mesh.objectType.c_str());
                
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
                
                // Scale - these should now persist
                swprintf_s(buffer, L"%.2f", mesh.scaleX);
                SetDlgItemText(hDlg, IDC_PROP_SCALE_X, buffer);
                swprintf_s(buffer, L"%.2f", mesh.scaleY);
                SetDlgItemText(hDlg, IDC_PROP_SCALE_Y, buffer);
                swprintf_s(buffer, L"%.2f", mesh.scaleZ);
                SetDlgItemText(hDlg, IDC_PROP_SCALE_Z, buffer);
                
                // Bounding box size information
                glm::vec3 boundingBoxSize = mesh.getTightDimensions();
                swprintf_s(buffer, L"X: %.2f, Y: %.2f, Z: %.2f", boundingBoxSize.x, boundingBoxSize.y, boundingBoxSize.z);
                SetDlgItemText(hDlg, IDC_BOUNDING_BOX_SIZE, buffer);
                
                // Color
                swprintf_s(buffer, L"%.2f", mesh.colorR);
                SetDlgItemText(hDlg, IDC_PROP_COLOR_R, buffer);
                swprintf_s(buffer, L"%.2f", mesh.colorG);
                SetDlgItemText(hDlg, IDC_PROP_COLOR_G, buffer);
                swprintf_s(buffer, L"%.2f", mesh.colorB);
                SetDlgItemText(hDlg, IDC_PROP_COLOR_B, buffer);
                
                // Material properties
                swprintf_s(buffer, L"%.2f", mesh.transparency);
                SetDlgItemText(hDlg, IDC_PROP_TRANSPARENCY, buffer);
                swprintf_s(buffer, L"%.2f", mesh.shininess);
                SetDlgItemText(hDlg, IDC_PROP_SHINY, buffer);
                
                // Material types combo box
                SendDlgItemMessage(hDlg, IDC_PROP_MATERIAL, CB_RESETCONTENT, 0, 0); // Clear existing items
                SendDlgItemMessage(hDlg, IDC_PROP_MATERIAL, CB_ADDSTRING, 0, (LPARAM)L"Default");
                SendDlgItemMessage(hDlg, IDC_PROP_MATERIAL, CB_ADDSTRING, 0, (LPARAM)L"Plastic");
                SendDlgItemMessage(hDlg, IDC_PROP_MATERIAL, CB_ADDSTRING, 0, (LPARAM)L"Metal");
                SendDlgItemMessage(hDlg, IDC_PROP_MATERIAL, CB_ADDSTRING, 0, (LPARAM)L"Glass");
                SendDlgItemMessage(hDlg, IDC_PROP_MATERIAL, CB_ADDSTRING, 0, (LPARAM)L"Wood");
                SendDlgItemMessage(hDlg, IDC_PROP_MATERIAL, CB_SETCURSEL, mesh.materialType, 0);
                
                // Display options
                CheckDlgButton(hDlg, IDC_PROP_WIREFRAME, mesh.wireframeMode ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_PROP_VISIBILITY, mesh.isVisible ? BST_CHECKED : BST_UNCHECKED);
            }
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            if (g_selectedMeshIdx >= 0 && g_selectedMeshIdx < static_cast<int>(model.getMeshCount()))
            {
                // Get current mesh properties first
                Mesh mesh;
                if (model.getMeshProperties(g_selectedMeshIdx, mesh))
                {
                    // Get values from dialog
                    wchar_t buffer[64];
                    
                    // Object identity
                    char nameBuffer[64] = {0};
                    GetDlgItemTextA(hDlg, IDC_PROP_NAME, nameBuffer, 64);
                    std::string objectName = nameBuffer;
                    
                    // Transform values
                    // Rotation
                    float rotX, rotY, rotZ;
                    GetDlgItemText(hDlg, IDC_PROP_ROT_X, buffer, 64);
                    rotX = (float)_wtof(buffer);
                    GetDlgItemText(hDlg, IDC_PROP_ROT_Y, buffer, 64);
                    rotY = (float)_wtof(buffer);
                    GetDlgItemText(hDlg, IDC_PROP_ROT_Z, buffer, 64);
                    rotZ = (float)_wtof(buffer);
                    
                    // Position
                    float posX, posY, posZ;
                    GetDlgItemText(hDlg, IDC_PROP_POS_X, buffer, 64);
                    posX = (float)_wtof(buffer);
                    GetDlgItemText(hDlg, IDC_PROP_POS_Y, buffer, 64);
                    posY = (float)_wtof(buffer);
                    GetDlgItemText(hDlg, IDC_PROP_POS_Z, buffer, 64);
                    posZ = (float)_wtof(buffer);
                    
                    // Scale - should now persist between updates
                    float scaleX, scaleY, scaleZ;
                    GetDlgItemText(hDlg, IDC_PROP_SCALE_X, buffer, 64);
                    scaleX = (float)_wtof(buffer);
                    GetDlgItemText(hDlg, IDC_PROP_SCALE_Y, buffer, 64);
                    scaleY = (float)_wtof(buffer);
                    GetDlgItemText(hDlg, IDC_PROP_SCALE_Z, buffer, 64);
                    scaleZ = (float)_wtof(buffer);
                    
                    // Color
                    float colorR, colorG, colorB;
                    GetDlgItemText(hDlg, IDC_PROP_COLOR_R, buffer, 64);
                    colorR = (float)_wtof(buffer);
                    GetDlgItemText(hDlg, IDC_PROP_COLOR_G, buffer, 64);
                    colorG = (float)_wtof(buffer);
                    GetDlgItemText(hDlg, IDC_PROP_COLOR_B, buffer, 64);
                    colorB = (float)_wtof(buffer);
                    
                    // Material properties
                    float transparency, shininess;
                    GetDlgItemText(hDlg, IDC_PROP_TRANSPARENCY, buffer, 64);
                    transparency = (float)_wtof(buffer);
                    GetDlgItemText(hDlg, IDC_PROP_SHINY, buffer, 64);
                    shininess = (float)_wtof(buffer);
                    
                    int materialType = SendDlgItemMessage(hDlg, IDC_PROP_MATERIAL, CB_GETCURSEL, 0, 0);
                    
                    // Display options
                    bool wireframe = (IsDlgButtonChecked(hDlg, IDC_PROP_WIREFRAME) == BST_CHECKED);
                    bool visible = (IsDlgButtonChecked(hDlg, IDC_PROP_VISIBILITY) == BST_CHECKED);
                    
                    // Update the mesh properties using thread-safe methods
                    mesh.objectName = objectName;
                    
                    // Update all mesh properties using the enhanced Model method
                    model.updateMeshAllProperties(
                        g_selectedMeshIdx,
                        rotX, rotY, rotZ,
                        posX, posY, posZ,
                        scaleX, scaleY, scaleZ,
                        colorR, colorG, colorB,
                        transparency, shininess, materialType,
                        wireframe, visible
                    );
                }
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
