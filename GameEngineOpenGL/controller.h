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
	int mouseX;
	int mouseY;
	int selectedMeshIndex; // Track the currently selected mesh index
	int selectedFaceIndex; // Track the currently selected face index

	Controller(Model* model, View* view) : model(model), view(view), mouseX(0), mouseY(0),
		handle(NULL), parentHandle(NULL),
		loopThreadFlag(false), selectedMeshIndex(-1), selectedFaceIndex(-1) {
	}

	~Controller() {
		loopThreadFlag = false; // Signal the thread to stop
		if (thread.joinable())
		{
			thread.join(); // Wait for the thread to finish
		}
	}

	int create(HWND handle, HWND parentHandle, HWND /* unused */) {
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
			break;
		default:
			break;
		}
	}

	void zoomIn() {
		model->camera.zoomIn(0.1f);
		view->setWindowSize(view->getWindowWidth(), view->getWindowHeight());
	}

	void zoomOut() {
		model->camera.zoomOut(0.1f);
		view->setWindowSize(view->getWindowWidth(), view->getWindowHeight());
	}

	// Clear selection for all meshes
	void clearAllSelections() {
		for (auto& mesh : model->meshes) {
			mesh.setSelected(false);
		}
		selectedMeshIndex = -1;
		selectedFaceIndex = -1;
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

		if (camera.mode == ORTHOGRAPHIC_MODE) {
			// In orthographic mode, we need to use the camera's actual near and far planes
			// instead of the fixed -1.0 and 1.0 normalized device coordinates
			glm::vec4 nearPoint(x, y, -1.0f, 1.0f); // Using NDC z = -1 for near plane
			glm::vec4 farPoint(x, y, 1.0f, 1.0f);   // Using NDC z = 1 for far plane

			// Convert to world space
			glm::mat4 invViewProj = glm::inverse(projection * view);
			glm::vec4 nearWorldPoint = invViewProj * nearPoint;
			glm::vec4 farWorldPoint = invViewProj * farPoint;

			// Convert to cartesian coordinates
			if (nearWorldPoint.w != 0.0f) nearWorldPoint /= nearWorldPoint.w;
			if (farWorldPoint.w != 0.0f) farWorldPoint /= farWorldPoint.w;

			// Calculate ray origin and direction
			outOrigin = glm::vec3(nearWorldPoint);
			outDir = glm::normalize(glm::vec3(farWorldPoint) - glm::vec3(nearWorldPoint));
			
			// Debug output for ray picking
			std::wstringstream ss;
			ss << L"[Ortho Ray] Origin: (" << outOrigin.x << ", " << outOrigin.y << ", " << outOrigin.z 
			   << ") Dir: (" << outDir.x << ", " << outDir.y << ", " << outDir.z << ")\n";
			OutputDebugString(ss.str().c_str());
		}
		else {
			// Perspective projection
			glm::vec4 rayClip(x, y, -1.0f, 1.0f);

			// Eye space
			glm::vec4 rayEye = glm::inverse(projection) * rayClip;
			rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

			// World space
			glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

			// Camera position as ray origin
			outOrigin = camera.getPosition();
			outDir = rayWorld;
			
			// Debug output for ray picking
			std::wstringstream ss;
			ss << L"[Persp Ray] Origin: (" << outOrigin.x << ", " << outOrigin.y << ", " << outOrigin.z 
			   << ") Dir: (" << outDir.x << ", " << outDir.y << ", " << outDir.z << ")\n";
			OutputDebugString(ss.str().c_str());
		}
	}
	
	// Handles mouse button down events, for object selection or other actions
	void handleMouseDown(WPARAM state, float x, float y) {
		mouseX = x;
		mouseY = y;
		// 1. Get camera matrices and window size
		glm::mat4 viewMatrix = model->camera.getViewMatrix();
		glm::mat4 projMatrix = model->getProjectionMatrix();
		int width = view->getWindowWidth();
		int height = view->getWindowHeight();

		// 2. Convert screen point to ray
		glm::vec3 rayOrigin, rayDir;
		screenPointToRay(x, y, width, height, viewMatrix, projMatrix, model->camera, rayOrigin, rayDir);

		// 3. Perform ray test to find intersections and track both mesh and face indices
		int meshIndex = -1;
		int faceIndex = -1;
		
		// Set appropriate ray range based on camera mode
		float tMin = 0.0f;
		float tMax = std::numeric_limits<float>::max();
		
		// For orthographic mode, use a large ray range since we could have objects in negative space
		if (model->camera.mode == ORTHOGRAPHIC_MODE) {
			tMin = -std::numeric_limits<float>::max(); // Allow intersections behind the ray origin
			tMax = std::numeric_limits<float>::max();
		}
		
		Ray ray(rayOrigin, rayDir, tMin, tMax);
		findRayIntersection(ray, meshIndex, faceIndex);
        
        // 4. Selection logic:
        if (meshIndex >= 0) {
            // Clicked on an object
            if (selectedMeshIndex == meshIndex) {
                // Clicked on the same object - select triangle or deselect object
                if (faceIndex >= 0) {
                    // If we clicked on a different triangle of the same mesh
                    if (selectedFaceIndex != faceIndex) {
                        // Select the specific triangle (keep the mesh selected)
                        selectFace(meshIndex, faceIndex);
						selectedFaceIndex = faceIndex;
                    } 
                    // If clicked on same triangle, do nothing (keep it selected)
                }
            } 
            else {
                // Clicked on a different object - clear previous selection and select new object
                clearAllSelections();
                selectMesh(meshIndex);
            }
        } 
        else {
            // Clicked on nothing - deselect everything
            clearAllSelections();
        }

        // Save the selected mesh index for the controller state
		selectedMeshIndex = meshIndex;
	}

	void selectMesh(int meshIndex) {
	    if (meshIndex >= 0 && meshIndex < static_cast<int>(model->meshes.size())) {
	        model->meshes[meshIndex].setSelected(true);
	    }
	}
	
	void selectFace(int meshIndex, int faceIndex) {
	    if (meshIndex >= 0 && meshIndex < static_cast<int>(model->meshes.size())) {
	        if (faceIndex >= 0 && faceIndex < static_cast<int>(model->meshes[meshIndex].faces.size())) {
	            model->meshes[meshIndex].selectFace(faceIndex);
	        }
	    }
	}

	void createDialogHandle(wchar_t* objectType, int x, int y, int z, int size) {
		if (wcscmp(objectType, L"Cube") == 0)
		{
			model->createCube(x, y, z, size);
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

	std::wstring openFileExplorer() {
		// Initialize the OPENFILENAME structure
		OPENFILENAME ofn;
		wchar_t filePath[MAX_PATH] = L"";
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = parentHandle; // Use parent window as owner
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
		}
		else {
			MessageBox(parentHandle, L"No file selected", L"Error", MB_OK);
		}
	}

	Mesh* getSelectedMesh() {
		if (selectedMeshIndex >= 0 && selectedMeshIndex < (int)model->meshes.size()) {
			return &model->meshes[selectedMeshIndex];
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
		if (selectedMeshIndex >= 0 && selectedMeshIndex < (int)model->meshes.size()) {
			model->deleteMesh(selectedMeshIndex);
			selectedMeshIndex = -1; // Reset selection
			selectedFaceIndex = -1;
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

	// Helper method to find the intersection of a ray with the closest face
	void findRayIntersection(const Ray& ray, int& outMeshIndex, int& outFaceIndex) {
		outMeshIndex = -1;
		outFaceIndex = -1;
		
		std::vector<Face*> hitFaces;
		float closestDistance = std::numeric_limits<float>::max();
		Face* closestFace = nullptr;
		
		// Use the spatial accelerator to efficiently get hit faces
		if (model->accelerator) {
		#if SPACIAL_OPT_MODE == SPACIAL_OPT_MEMORY
			void* root = static_cast<BVH*>(model->accelerator.get())->getRoot();
		#else
			void* root = static_cast<KDTree*>(model->accelerator.get())->getRoot();
		#endif
			model->accelerator->traverse(root, ray, hitFaces);
		}
		
		if (!hitFaces.empty()) {
			// Find the closest face by computing distances
			for (Face* face : hitFaces) {
				// For simplicity, use the centroid of the face to determine distance
				float dist = glm::length(face->centroid - ray.origin);
				if (dist < closestDistance) {
					closestDistance = dist;
					closestFace = face;
				}
			}
			
			// Find which mesh and face index this belongs to
			if (closestFace != nullptr) {
				for (size_t m = 0; m < model->meshes.size(); ++m) {
					const Mesh& mesh = model->meshes[m];
					for (size_t f = 0; f < mesh.faces.size(); ++f) {
						if (&mesh.faces[f] == closestFace) {
							outMeshIndex = static_cast<int>(m);
							outFaceIndex = static_cast<int>(f);
							return;
						}
					}
				}
			}
		}
		
		// If we didn't hit any faces or couldn't identify the mesh/face, leave outputs as -1
	}

	// Returns the index of the intersected mesh, or -1 if no intersection
	int testRayIntersections(float originX, float originY, float originZ, float dirX, float dirY, float dirZ) {
		// Set appropriate ray range based on camera mode
		float tMin = 0.0f;
		float tMax = std::numeric_limits<float>::max();
		
		// For orthographic mode, use a large ray range since we could have objects in negative space
		if (model->camera.mode == ORTHOGRAPHIC_MODE) {
			tMin = -std::numeric_limits<float>::max(); // Allow intersections behind the ray origin
			tMax = std::numeric_limits<float>::max();
		}
		
		Ray ray(glm::vec3(originX, originY, originZ), glm::vec3(dirX, dirY, dirZ), tMin, tMax);

		int meshIndex, faceIndex;
		findRayIntersection(ray, meshIndex, faceIndex);
		
		if (meshIndex != -1) {
			std::wstringstream ss;
			ss << L"[Picking] Intersection found: Mesh " << meshIndex;
			if (faceIndex != -1) {
				ss << L", Face " << faceIndex;
			}
			ss << L"\n";
			OutputDebugString(ss.str().c_str());
		}
		
		return meshIndex;
	}
};
