#pragma once
#include "model.h"
#include "view.h"
#include <windows.h>
#include <thread>
#include <commdlg.h>
#include <string>


class Controller {
public:
	Model* model;
	View* view;
	std::thread thread;
	bool loopThreadFlag;
	HWND handle;
	HWND parentHandle;
	HWND sidebarHandle;
	int mouseX;
	int mouseY;

	Controller(Model* model, View* view) : model(model), view(view), mouseX(0), mouseY(0),
		handle(NULL), parentHandle(NULL), sidebarHandle(NULL),
		loopThreadFlag(false) {
	}

	~Controller() {
		loopThreadFlag = false; // Signal the thread to stop
		if (thread.joinable())
		{
			thread.join(); // Wait for the thread to finish
		}
	}

	int create(HWND handle, HWND parentHandle, HWND sidebarHandle) {
		this->handle = handle;
		this->parentHandle = parentHandle;
		this->sidebarHandle = sidebarHandle;
		// Create a context
		if (!view->setContext(handle)) {
			MessageBox(NULL, L"Failed to set context", L"Error", MB_OK);
			return 1;
		}
		// Create a separate thread for the OpenGL context
		thread = std::thread(&Controller::runThread, this);
		loopThreadFlag = true;
		return 0;
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

	// Handles camera rotation based on mouse movement
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

	void screenPointToRay(float mouseX, float mouseY, int screenWidth, int screenHeight,
		const glm::mat4& view, const glm::mat4& projection, const Camera& camera,
		glm::vec3& outOrigin, glm::vec3& outDir) {
		// Convert to normalized device coordinates
		float x = (2.0f * mouseX) / screenWidth - 1.0f;
		float y = 1.0f - (2.0f * mouseY) / screenHeight;
		glm::vec4 rayClip(x, y, -1.0f, 1.0f);

		// Eye space
		glm::vec4 rayEye = glm::inverse(projection) * rayClip;
		rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

		// World space
		glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

		// Camera position as ray origin
		outOrigin = model->camera.getPosition(); 
		outDir = rayWorld;
	}
	
	// Handles mouse button down events, for object selection or other actions
	void handleMouseDown(WPARAM state, float x, float y) {
		mouseX = x;
		mouseY = y;
		// 1. Get camera matrices and window size
		glm::mat4 viewMatrix = model->camera.getViewMatrix();
		glm::mat4 projMatrix = model->getProjectionMatrix(view->getWindowWidth(), view->getWindowHeight());
		int width = view->getWindowWidth();
		int height = view->getWindowHeight();

		// 2. Convert screen point to ray
		glm::vec3 rayOrigin, rayDir;
		screenPointToRay(x, y, width, height, viewMatrix, projMatrix, model->camera, rayOrigin, rayDir);

		testRayIntersections(rayOrigin.x, rayOrigin.y, rayOrigin.z, rayDir.x, rayDir.y, rayDir.z);
	}

	void createDialogHandle(wchar_t* objectType, int x, int y, int z, int size) {
		if (wcscmp(objectType, L"Cube") == 0)
		{
			model->createCube(x, y, z, size);
			updateSidebar();
		}
		else if (wcscmp(objectType, L"Pyramid") == 0)
		{
			model->createPyramid(x, y, z, size);
			updateSidebar();
		}
		else
		{
			MessageBox(parentHandle, L"Invalid object type", L"Error", MB_OK);
		}
	}

	std::wstring openFileExplorer() {
		// Initialize the OPENFILENAME structure
		OPENFILENAME ofn;
		wchar_t filePath[MAX_PATH] = L"";
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL; // No specific owner window
		ofn.lpstrFile = filePath;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = L"STL Files\0*.stl\0All Files\0*.*\0";
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Display the file dialog
		if (GetOpenFileName(&ofn)) {
			// Return the selected file path
			return std::wstring(filePath);
		}
		else {
			// Return an empty string if the user cancels
			return L"";
		}
	}

	void createFromFile() {
		std::wstring filePath = openFileExplorer();
		if (!filePath.empty()) {
			model->createFromFile(filePath);
			updateSidebar();
		}
		else {
			MessageBox(parentHandle, L"No file selected", L"Error", MB_OK);
		}
	}

	// Update the sidebar with the list of objects
	void updateSidebar() {
		HWND hList = GetDlgItem(sidebarHandle, IDC_OBJECT_LIST);
		if (!hList) return;

		SendMessage(hList, LB_RESETCONTENT, 0, 0);

		for (size_t i = 0; i < model->meshes.size(); ++i) {
			std::wstringstream ss;
			ss << L"Object " << (i + 1);
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)ss.str().c_str());
		}
	}

	Mesh* getSelectedMesh() {
		HWND hList = GetDlgItem(sidebarHandle, IDC_OBJECT_LIST);
		if (!hList) return nullptr;
		int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
		if (sel >= 0 && sel < (int)model->meshes.size()) {
			return &model->meshes[sel];
		}
		return nullptr;
	}

	void toggleOrbitAroundObject() {
		Mesh* selectedMesh = getSelectedMesh();
		if (selectedMesh) {
			if (model->camera.isOrbitMode()) {
				model->camera.setOrbitMode(false);
			}
			else {
				model->camera.setOrbitMode(true, selectedMesh->getCenter(), 10.0f);
			}
		}
		else {
			MessageBox(parentHandle, L"No object selected", L"Error", MB_OK);
		}
	}

	void deleteSelectedObject() {
		HWND hList = GetDlgItem(sidebarHandle, IDC_OBJECT_LIST);
		if (!hList) return;
		int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
		if (sel >= 0 && sel < (int)model->meshes.size()) {
			model->meshes.erase(model->meshes.begin() + sel);
			updateSidebar();
		}
	}

	void toggleBoundingBox() {
		Mesh* selectedMesh = getSelectedMesh();
		if (selectedMesh) {
			selectedMesh->toggleBoundingBox();
		}
		else {
			MessageBox(parentHandle, L"No object selected", L"Error", MB_OK);
		}
	}

	void toggleVertices() {
		Mesh* selectedMesh = getSelectedMesh();
		if (selectedMesh) {
			selectedMesh->toggleVertices();
		}
		else {
			MessageBox(parentHandle, L"No object selected", L"Error", MB_OK);
		}
	}

	void fitOjbectToView() {
		Mesh* selectedMesh = getSelectedMesh();
		if (selectedMesh) {
			model->camera.zoomToBoundingBox(selectedMesh->getCenter(), selectedMesh->getSize(), view->getAspectRatio());
		}
		else {
			MessageBox(parentHandle, L"No object selected", L"Error", MB_OK);
		}
	}

	void testRayIntersections(float originX, float originY, float originZ, float dirX, float dirY, float dirZ) {
		Ray ray(glm::vec3(originX, originY, originZ), glm::vec3(dirX, dirY, dirZ), 0.0f, 100.0f);

		std::vector<Face*> hitFaces;
		model->bvh.traverse(model->bvh.getRoot(), ray, hitFaces);
		
		if (!hitFaces.empty()) {
			Face* firstHit = hitFaces[0];
			int meshIndex = -1, faceIndex = -1;
			for (size_t m = 0; m < model->meshes.size(); ++m) {
				for (size_t f = 0; f < model->meshes[m].faces.size(); ++f) {
					if (&model->meshes[m].faces[f] == firstHit) {
						meshIndex = (int)m;
						faceIndex = (int)f;
						break;
					}
				}
				if (meshIndex != -1) break;
			}
			std::wstringstream ss;
			ss << L"[Picking] Intersections found: " << hitFaces.size();
			if (meshIndex != -1) {
				ss << L", First hit: Mesh " << meshIndex << L", Face " << faceIndex;
			}
			ss << L"\n";
			OutputDebugString(ss.str().c_str());
		}
			
	}
};
