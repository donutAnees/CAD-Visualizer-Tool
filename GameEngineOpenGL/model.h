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
    std::unique_ptr<SpatialAccelerator> accelerator;
    std::unique_ptr<ViewProjMethodGLM> projectionMethod;
    
	Model() : camera(), grid(camera) {
        // Create the appropriate spatial accelerator based on optimization mode
        accelerator.reset(SpatialAcceleratorFactory::createAccelerator());
	}

	void init() {
		// To enable depth testing, which can be used to determine which objects, or parts of objects, are visible
		glEnable(GL_DEPTH_TEST);
		// Clear the color and depth buffer
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

    void buildAccelerator() {
        if (accelerator) {
            accelerator->build(meshes);
    }
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
        // Update the aspect ratio based on the window size
        float aspect = float(width) / float(height);
        
        if (camera.mode == PERSPECTIVE_MODE) {
            projectionMethod = std::make_unique<PerspectiveProj>(
                camera.zoom, aspect, camera.nearPlane, camera.farPlane);
        }
        else {
            float orthoZoom = 1.0f / camera.zoom;
            
            // For orthographic projection, we need to ensure the near and far planes 
            // are set appropriately to prevent depth clipping issues
            float nearOrtho = -camera.farPlane;  // Use a negative near plane for orthographic to ensure objects aren't clipped
            float farOrtho = camera.farPlane;    // Keep far plane the same
            
            projectionMethod = std::make_unique<OrthoProj>(
                -aspect * orthoZoom, aspect * orthoZoom,
                -orthoZoom, orthoZoom,
                nearOrtho, farOrtho);
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

    }

    // Update mesh properties and rebuild spatial accelerator
    void updateMeshProperties(int meshIndex, float rotX, float rotY, float rotZ, 
                              float posX, float posY, float posZ) {
        if (meshIndex >= 0 && meshIndex < static_cast<int>(meshes.size())) {
            Mesh& mesh = meshes[meshIndex];
            
            // Update mesh properties
            mesh.rotationX = rotX;
            mesh.rotationY = rotY;
            mesh.rotationZ = rotZ;
            mesh.centerX = posX;
            mesh.centerY = posY;
            mesh.centerZ = posZ;
            
            // Update the mesh geometry
            mesh.updateMesh();
            
            // Rebuild spatial accelerator because mesh geometry changed
            buildAccelerator();
        }
    }
    
    // Comprehensive update of mesh properties including scale, color, etc.
    void updateMeshAllProperties(int meshIndex, 
                               float rotX, float rotY, float rotZ,
                               float posX, float posY, float posZ,
                               float scaleX, float scaleY, float scaleZ,
                               float colorR, float colorG, float colorB, 
                               float transparency, float shininess, int materialType,
                               bool wireframe, bool visible) {
        if (meshIndex >= 0 && meshIndex < static_cast<int>(meshes.size())) {
            Mesh& mesh = meshes[meshIndex];
            
            // First, handle color and material changes
            if (mesh.colorR != colorR || mesh.colorG != colorG || mesh.colorB != colorB) {
                mesh.colorR = colorR;
                mesh.colorG = colorG;
                mesh.colorB = colorB;
                mesh.updateColors(colorR, colorG, colorB);
            }
            
            // Update material properties
            mesh.transparency = transparency;
            mesh.isTransparent = (transparency > 0.01f);
            mesh.shininess = shininess;
            mesh.materialType = materialType;
            
            // Update display options
            mesh.wireframeMode = wireframe;
            mesh.isVisible = visible;
            
            // Apply scale
            mesh.applyScale(scaleX, scaleY, scaleZ);
            
            // Update transform properties
            bool transformChanged = (fabs(mesh.rotationX - rotX) > 0.001f ||
                                   fabs(mesh.rotationY - rotY) > 0.001f ||
                                   fabs(mesh.rotationZ - rotZ) > 0.001f ||
                                   fabs(mesh.centerX - posX) > 0.001f ||
                                   fabs(mesh.centerY - posY) > 0.001f ||
                                   fabs(mesh.centerZ - posZ) > 0.001f);
                                   
            if (transformChanged) {
                mesh.rotationX = rotX;
                mesh.rotationY = rotY;
                mesh.rotationZ = rotZ;
                mesh.centerX = posX;
                mesh.centerY = posY;
                mesh.centerZ = posZ;
            }
            
            // Always update the mesh to apply all changes
            mesh.updateMesh();
            
            // Rebuild spatial accelerator because mesh geometry changed
            buildAccelerator();
        }
    }

    void createCube(int x, int y, int z) {
        Mesh mesh;

        float halfSize = 0.5f; // Default size is 1 unit

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
        
        // Set cube-specific properties
        mesh.objectName = "Cube";
        mesh.objectType = "Cube";
        mesh.colorR = 1.0f;
        mesh.colorG = 0.0f;
        mesh.colorB = 0.0f;
        
        meshes.push_back(mesh);
		buildAccelerator();
    }


    void createPyramid(int x, int y, int z) {
        Mesh mesh;
        float halfSize = 0.5f; // Default size is 1 unit
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
       
       // Set pyramid-specific properties
       mesh.objectName = "Pyramid";
       mesh.objectType = "Pyramid";
       mesh.colorR = 0.0f;
       mesh.colorG = 1.0f;
       mesh.colorB = 0.0f;
       
       meshes.push_back(mesh);
       buildAccelerator();
    }

    void createCircle(int x, int y, int z) {
        Mesh mesh;
        const int segments = 36; // Number of segments for the circle
        const float radius = 0.5f; // Default radius is 0.5 units

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> colors;
        std::vector<unsigned int> indices;

        // Center vertex
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        colors.push_back(1.0f);
        colors.push_back(1.0f);
        colors.push_back(0.0f); // Yellow center

        // Generate circle vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.14159265358979323846 * i / segments;
            float vx = x + radius * cos(angle);
            float vz = z + radius * sin(angle);

            vertices.push_back(vx);
            vertices.push_back(y);
            vertices.push_back(vz);

            colors.push_back(0.0f);
            colors.push_back(0.0f);
            colors.push_back(1.0f); // Blue circle

            if (i > 0) {
                indices.push_back(0);
                indices.push_back(i);
                indices.push_back(i + 1);
            }
        }

        mesh.init(vertices, colors, indices);
        
        // Set circle-specific properties
        mesh.objectName = "Circle";
        mesh.objectType = "Circle";
        mesh.colorR = 0.0f;
        mesh.colorG = 0.0f;
        mesh.colorB = 1.0f;
        
        meshes.push_back(mesh);
        buildAccelerator();
    }

    void createCylinder(int x, int y, int z) {
        Mesh mesh;
        const int segments = 36; // Number of segments for the cylinder
        const float radius = 0.5f; // Default radius is 0.5 units
        const float height = 1.0f; // Default height is 1 unit

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> colors;
        std::vector<unsigned int> indices;

        // Generate top and bottom circle vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.14159265358979323846 * i / segments;
            float vx = radius * cos(angle);
            float vz = radius * sin(angle);

            // Bottom circle
            vertices.push_back(x + vx);
            vertices.push_back(y);
            vertices.push_back(z + vz);
            colors.push_back(1.0f);
            colors.push_back(0.0f);
            colors.push_back(0.0f); // Red bottom

            // Top circle
            vertices.push_back(x + vx);
            vertices.push_back(y + height);
            vertices.push_back(z + vz);
            colors.push_back(0.0f);
            colors.push_back(1.0f);
            colors.push_back(0.0f); // Green top

            if (i > 0) {
                // Side faces
                indices.push_back(2 * (i - 1));
                indices.push_back(2 * i);
                indices.push_back(2 * (i - 1) + 1);

                indices.push_back(2 * (i - 1) + 1);
                indices.push_back(2 * i);
                indices.push_back(2 * i + 1);

                // Bottom face
                indices.push_back(0);
                indices.push_back(2 * (i - 1));
                indices.push_back(2 * i);

                // Top face
                indices.push_back(1);
                indices.push_back(2 * (i - 1) + 1);
                indices.push_back(2 * i + 1);
            }
        }

        mesh.init(vertices, colors, indices);
        
        // Set cylinder-specific properties
        mesh.objectName = "Cylinder";
        mesh.objectType = "Cylinder";
        mesh.colorR = 1.0f;
        mesh.colorG = 0.0f;
        mesh.colorB = 0.0f;
        
        meshes.push_back(mesh);
        buildAccelerator();
    }

    void createSphere(int x, int y, int z) {
        Mesh mesh;
        const int segments = 36; // Number of segments for the sphere
        const int rings = 18;   // Number of rings for the sphere
        const float radius = 0.5f; // Default radius is 0.5 units

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> colors;
        std::vector<unsigned int> indices;

        for (int i = 0; i <= rings; ++i) {
            float phi = 3.14159265358979323846 * i / rings;
            for (int j = 0; j <= segments; ++j) {
                float theta = 2.0f * 3.14159265358979323846 * j / segments;
                float vx = radius * sin(phi) * cos(theta);
                float vy = radius * cos(phi);
                float vz = radius * sin(phi) * sin(theta);

                vertices.push_back(x + vx);
                vertices.push_back(y + vy);
                vertices.push_back(z + vz);

                colors.push_back(0.5f);
                colors.push_back(0.5f);
                colors.push_back(0.5f); // Gray sphere

                if (i < rings && j < segments) {
                    int first = i * (segments + 1) + j;
                    int second = first + segments + 1;

                    indices.push_back(first);
                    indices.push_back(second);
                    indices.push_back(first + 1);

                    indices.push_back(second);
                    indices.push_back(second + 1);
                    indices.push_back(first + 1);
                }
            }
        }

        mesh.init(vertices, colors, indices);
        
        // Set sphere-specific properties
        mesh.objectName = "Sphere";
        mesh.objectType = "Sphere";
        mesh.colorR = 0.5f;
        mesh.colorG = 0.5f;
        mesh.colorB = 0.5f;
        
        meshes.push_back(mesh);
        buildAccelerator();
    }

    void createCone(int x, int y, int z) {
        Mesh mesh;
        const int segments = 36; // Number of segments for the cone
        const float radius = 0.5f; // Default radius is 0.5 units
        const float height = 1.0f; // Default height is 1 unit

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> colors;
        std::vector<unsigned int> indices;

        // Apex vertex
        vertices.push_back(x);
        vertices.push_back(y + height);
        vertices.push_back(z);
        colors.push_back(1.0f);
        colors.push_back(0.5f);
        colors.push_back(0.0f); // Orange apex

        // Base vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.14159265358979323846 * i / segments;
            float vx = radius * cos(angle);
            float vz = radius * sin(angle);

            vertices.push_back(x + vx);
            vertices.push_back(y);
            vertices.push_back(z + vz);
            colors.push_back(0.0f);
            colors.push_back(0.0f);
            colors.push_back(1.0f); // Blue base

            if (i > 0) {
                // Side faces
                indices.push_back(0);
                indices.push_back(i);
                indices.push_back(i + 1);

                // Base face
                indices.push_back(1);
                indices.push_back(i);
                indices.push_back(i + 1);
            }
        }

        mesh.init(vertices, colors, indices);
        
        // Set cone-specific properties
        mesh.objectName = "Cone";
        mesh.objectType = "Cone";
        mesh.colorR = 1.0f;
        mesh.colorG = 0.5f;
        mesh.colorB = 0.0f;
        
        meshes.push_back(mesh);
        buildAccelerator();
    }

    void createTorus(int x, int y, int z) {
        Mesh mesh;
        const int segments = 36; // Number of segments for the torus
        const int rings = 18;   // Number of rings for the torus
        const float majorRadius = 0.5f; // Default major radius
        const float minorRadius = 0.2f; // Default minor radius

        std::vector<GLfloat> vertices;
        std::vector<GLfloat> colors;
        std::vector<unsigned int> indices;

        for (int i = 0; i <= rings; ++i) {
            float phi = 2.0f * 3.14159265358979323846 * i / rings;
            for (int j = 0; j <= segments; ++j) {
                float theta = 2.0f * 3.14159265358979323846 * j / segments;
                float vx = (majorRadius + minorRadius * cos(theta)) * cos(phi);
                float vy = minorRadius * sin(theta);
                float vz = (majorRadius + minorRadius * cos(theta)) * sin(phi);

                vertices.push_back(x + vx);
                vertices.push_back(y + vy);
                vertices.push_back(z + vz);

                colors.push_back(1.0f);
                colors.push_back(1.0f);
                colors.push_back(0.0f); // Yellow torus

                if (i < rings && j < segments) {
                    int first = i * (segments + 1) + j;
                    int second = first + segments + 1;

                    indices.push_back(first);
                    indices.push_back(second);
                    indices.push_back(first + 1);

                    indices.push_back(second);
                    indices.push_back(second + 1);
                    indices.push_back(first + 1);
                }
            }
        }

        mesh.init(vertices, colors, indices);
        
        // Set torus-specific properties
        mesh.objectName = "Torus";
        mesh.objectType = "Torus";
        mesh.colorR = 1.0f;
        mesh.colorG = 1.0f;
        mesh.colorB = 0.0f;
        
        meshes.push_back(mesh);
        buildAccelerator();
    }

    void createPlane(int x, int y, int z) {
        Mesh mesh;
        const float size = 1.0f; // Default size is 1 unit

        std::vector<GLfloat> vertices = {
            static_cast<float>(x) - size / 2, static_cast<float>(y), static_cast<float>(z) - size / 2,
            static_cast<float>(x) + size / 2, static_cast<float>(y), static_cast<float>(z) - size / 2,
            static_cast<float>(x) + size / 2, static_cast<float>(y), static_cast<float>(z) + size / 2,
            static_cast<float>(x) - size / 2, static_cast<float>(y), static_cast<float>(z) + size / 2
        };

        std::vector<GLfloat> colors = {
            0.0f, 1.0f, 0.0f, // Green plane
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f
        };

        std::vector<unsigned int> indices = {
            0, 1, 2,
            0, 2, 3
        };

        mesh.init(vertices, colors, indices);
        
        // Set plane-specific properties
        mesh.objectName = "Plane";
        mesh.objectType = "Plane";
        mesh.colorR = 0.0f;
        mesh.colorG = 1.0f;
        mesh.colorB = 0.0f;
        
        meshes.push_back(mesh);
        buildAccelerator();
    }

    void createFromFile(std::wstring filePath) {
		Mesh mesh;
		if (mesh.loadFromSTL(std::string(filePath.begin(), filePath.end()))) {
			meshes.push_back(mesh);
			buildAccelerator();
			MessageBox(NULL, L"File loaded successfully!", L"Info", MB_OK);
		} 
		else {
			MessageBox(NULL, L"Failed to load the file.", L"Error", MB_OK);
		}
    }
    
    void deleteMesh(int index) {
        if (index >= 0 && index < static_cast<int>(meshes.size())) {
            meshes.erase(meshes.begin() + index);
            buildAccelerator();
        }
    }
};
