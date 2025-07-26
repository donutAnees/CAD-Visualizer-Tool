#pragma once

#include "camera.h"
#include <glm/gtc/type_ptr.hpp>
#include <GL/gl.h>
#include <GL/glu.h>
#include <sstream>
#include <vector>
#include "Mesh.h"
#include "grid.h"
#include "spacialaccelerator.h"
#include "ProjectionSystem.h"
#include <memory>

class Model {
public:
	Camera camera;
	std::vector<Mesh> meshes;
	Grid grid;
    BVH bvh;
    std::unique_ptr<ViewProjMethodGLM> projectionMethod;

	Model() : camera(), grid(camera) {
	}

	void init() {
		// To enable depth testing, which can be used to determine which objects, or parts of objects, are visible
		glEnable(GL_DEPTH_TEST);
		// Clear the color and depth buffer
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

    void buildBVH() {
        bvh.build(meshes);
    }

	// Get Projection Matrix
	glm::mat4 getProjectionMatrix() const {
		if (projectionMethod) {
			return projectionMethod->getComposedProjectionMatrix();
		}
		return glm::mat4(1.0f);	
    }

    // Call this whenever screen size or camera mode changes
    void updateProjection(int width, int height) {
        float aspect = float(width) / float(height);

        if (camera.mode == PERSPECTIVE_MODE) {
            projectionMethod = std::make_unique<PerspectiveProj>(
                camera.zoom, aspect, camera.nearPlane, camera.farPlane);
        }
        else {
            float orthoZoom = 1.0f / camera.zoom;
            projectionMethod = std::make_unique<OrthoProj>(
                -aspect * orthoZoom, aspect * orthoZoom,
                -orthoZoom, orthoZoom,
                camera.nearPlane, camera.farPlane);
        }
    }

	void draw(int width, int height) {
		// Clear the color and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Set Projection Matrix
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
        if (projectionMethod) {
            glm::mat4 proj = projectionMethod->getComposedProjectionMatrix();
            glLoadMatrixf(glm::value_ptr(proj));
        }

		// Set Model View Matrix
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glLoadMatrixf(glm::value_ptr(camera.getViewMatrix()));

		// Draw the plane grid
        grid.drawXZGrid();

		// Draw the meshes
		for (const auto& mesh : meshes) {
			mesh.draw();
			mesh.drawLocalAxis(); // Draw local axis for each mesh
		}
		bvh.drawBVHNode(bvh.getRoot());
	}

    void createCube(int x, int y, int z, float size) {
        Mesh mesh;

        // Define half size for easier calculations
        float halfSize = size / 2.0f;

        // Define vertices for a cube centered at (x, y, z)
        std::vector<GLfloat> vertices = {
            // Front face
            x - halfSize, y - halfSize, z + halfSize, // Bottom-left
            x + halfSize, y - halfSize, z + halfSize, // Bottom-right
            x + halfSize, y + halfSize, z + halfSize, // Top-right
            x - halfSize, y + halfSize, z + halfSize, // Top-left

            // Back face
            x - halfSize, y - halfSize, z - halfSize, // Bottom-left
            x + halfSize, y - halfSize, z - halfSize, // Bottom-right
            x + halfSize, y + halfSize, z - halfSize, // Top-right
            x - halfSize, y + halfSize, z - halfSize  // Top-left
        };

        std::vector<GLfloat> colors = {
            // Front face (red)
            1.0f, 0.0f, 0.0f, // Bottom-left
            1.0f, 0.0f, 0.0f, // Bottom-right
            1.0f, 0.0f, 0.0f, // Top-right
            1.0f, 0.0f, 0.0f, // Top-left

            // Back face (blue)
            0.0f, 0.0f, 1.0f, // Bottom-left
            0.0f, 0.0f, 1.0f, // Bottom-right
            0.0f, 0.0f, 1.0f, // Top-right
            0.0f, 0.0f, 1.0f  // Top-left
        };

        std::vector<unsigned int> indices = {
            // Front face
            0, 1, 2, 0, 2, 3,
            // Back face
            4, 5, 6, 4, 6, 7,
            // Left face
            4, 0, 3, 4, 3, 7,
            // Right face
            1, 5, 6, 1, 6, 2,
            // Top face
            3, 2, 6, 3, 6, 7,
            // Bottom face
            4, 5, 1, 4, 1, 0
        };

		mesh.init(vertices, colors, indices);
        meshes.push_back(mesh);
		bvh.build(meshes);
    }


    void createPyramid(int x, int y, int z, float size) {
       Mesh mesh;
       // Define half size for easier calculations
       float halfSize = size / 2.0f;
       // Define vertices for a pyramid centered at (x, y, z)
       std::vector<GLfloat> vertices = {
           // Base
           static_cast<float>(x) - halfSize, static_cast<float>(y) - halfSize, static_cast<float>(z) - halfSize,
           static_cast<float>(x) + halfSize, static_cast<float>(y) - halfSize, static_cast<float>(z) - halfSize,
           static_cast<float>(x) + halfSize, static_cast<float>(y) - halfSize, static_cast<float>(z) + halfSize,
           static_cast<float>(x) - halfSize, static_cast<float>(y) - halfSize, static_cast<float>(z) + halfSize,
           // Apex
           static_cast<float>(x), static_cast<float>(y) + halfSize, static_cast<float>(z)
       };
       std::vector<GLfloat> colors = {
           // Base (green)
           0.0f, 1.0f, 0.0f,
           0.0f, 1.0f, 0.0f,
           0.0f, 1.0f, 0.0f,
           0.0f, 1.0f, 0.0f,
           // Apex (yellow)
           1.0f, 1.0f, 0.0f
       };
       std::vector<unsigned int> indices = {
           // Base
           0, 1, 2, 2, 3, 0,
           // Sides
           4, 1, 2,
           4, 2, 3,
           4, 3, 0,
           4, 0, 1
       };
       mesh.init(vertices, colors, indices);
       meshes.push_back(mesh);
	   bvh.build(meshes);
    }

    void createFromFile(std::wstring filePath) {
		Mesh mesh;
		if (mesh.loadFromSTL(std::string(filePath.begin(), filePath.end()))) {
			meshes.push_back(mesh);
			bvh.build(meshes);
			MessageBox(NULL, L"File loaded successfully!", L"Info", MB_OK);
		} 
		else {
			MessageBox(NULL, L"Failed to load the file.", L"Error", MB_OK);
		}
    }
};
