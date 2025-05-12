#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <glm/glm.hpp>

class Mesh {
public:
    // Data
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> colors;
    std::vector<unsigned int> indices;
	bool isTransparent = false; 
	GLfloat minX, maxX, minY, maxY, minZ, maxZ; // Bounding box coordinates
	GLfloat centerX, centerY, centerZ; // Center of the mesh
	GLfloat sizeX, sizeY, sizeZ; // Size of the mesh in each dimension
	GLfloat rotationX, rotationY, rotationZ; // Rotation angles in degrees

    // Constructor
    Mesh() {}

    // Destructor
    ~Mesh() {
        cleanup();
    }

    void init(const std::vector<GLfloat>& vertices, const std::vector<GLfloat>& colors, const std::vector<unsigned int>& indices) {
		this->vertices = vertices;
		this->colors = colors;
		this->indices = indices;
        // Calculate the bounds of the object
        minX = vertices[0];
        maxX = vertices[0];
        minY = vertices[1];
        maxY = vertices[1];
        minZ = vertices[2];
        maxZ = vertices[2];

        for (size_t i = 0; i < vertices.size(); i += 3) {
            minX = std::min(minX, vertices[i]);
            maxX = std::max(maxX, vertices[i]);
            minY = std::min(minY, vertices[i + 1]);
            maxY = std::max(maxY, vertices[i + 1]);
            minZ = std::min(minZ, vertices[i + 2]);
            maxZ = std::max(maxZ, vertices[i + 2]);
        }

        // Calculate the center of the object
        centerX = (minX + maxX) / 2.0f;
        centerY = (minY + maxY) / 2.0f;
        centerZ = (minZ + maxZ) / 2.0f;
        
        // Calculate the size of the object
        sizeX = maxX - minX;
        sizeY = maxY - minY;
        sizeZ = maxZ - minZ;

		// Set the rotation angles to 0
		rotationX = 0.0f;
		rotationY = 0.0f;
		rotationZ = 0.0f;

    }

	// Set vertices and colors
	void setVertices(const std::vector<GLfloat>& vertices) {
		this->vertices = vertices;
	}

	void setColors(const std::vector<GLfloat>& colors) {
		this->colors = colors;
	}

	// Set indices
	void setIndices(const std::vector<unsigned int>& indices) {
		this->indices = indices;
	}

    // Render the mesh
    void draw() const {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vertices.data());

        // Enable and point to the color array
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_FLOAT, 0, colors.data());
        
		// If the mesh is transparent, enable blending
        if (isTransparent) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1.0f, 1.0f, 1.0f, 0.5f); // Set transparency (alpha = 0.5)
        }


        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());

        if (isTransparent) {
            glDisable(GL_BLEND);
        }
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

    }

    void drawLocalAxis() const {
        if (vertices.empty()) {
            return; // No vertices, nothing to draw
        }
       
        // Draw the axes
        glBegin(GL_LINES);

        // X-axis (red) scaled to the object's size
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(centerX, centerY, centerZ);
        glVertex3f(centerX + sizeX, centerY, centerZ);

        // Y-axis (green) scaled to the object's size
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(centerX, centerY, centerZ);
        glVertex3f(centerX, centerY + sizeY, centerZ);

        // Z-axis (blue) scaled to the object's size
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(centerX, centerY, centerZ);
        glVertex3f(centerX, centerY, centerZ + sizeZ);

        glEnd();
    }

    bool loadFromSTL(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open STL file: " << filePath << std::endl;
            return false;
        }

        // Read the first 80 bytes to check if it's binary or ASCII
        char header[80];
        file.read(header, 80);

        // Read the number of triangles
        uint32_t numTriangles;
        file.read(reinterpret_cast<char*>(&numTriangles), sizeof(numTriangles));

        // Check if the file size matches the expected size for binary STL
        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.seekg(80 + sizeof(numTriangles), std::ios::beg);

        std::streamsize expectedSize = 80 + sizeof(numTriangles) + numTriangles * (sizeof(float) * 12 + sizeof(uint16_t));
        if (fileSize == expectedSize) {
            // Binary STL
            vertices.clear();
            indices.clear();
            colors.clear();

            for (uint32_t i = 0; i < numTriangles; ++i) {
                float normal[3];
                float vertex1[3], vertex2[3], vertex3[3];
                uint16_t attributeByteCount;

                // Read normal
                file.read(reinterpret_cast<char*>(normal), sizeof(normal));

                // Read vertices
                file.read(reinterpret_cast<char*>(vertex1), sizeof(vertex1));
                file.read(reinterpret_cast<char*>(vertex2), sizeof(vertex2));
                file.read(reinterpret_cast<char*>(vertex3), sizeof(vertex3));

                // Read attribute byte count
                file.read(reinterpret_cast<char*>(&attributeByteCount), sizeof(attributeByteCount));

                // Add vertices to the mesh
                size_t baseIndex = vertices.size() / 3;
                vertices.insert(vertices.end(), { vertex1[0], vertex1[1], vertex1[2] });
                vertices.insert(vertices.end(), { vertex2[0], vertex2[1], vertex2[2] });
                vertices.insert(vertices.end(), { vertex3[0], vertex3[1], vertex3[2] });

                // Add indices
                indices.insert(indices.end(), { static_cast<unsigned int>(baseIndex),
                                               static_cast<unsigned int>(baseIndex + 1),
                                               static_cast<unsigned int>(baseIndex + 2) });

                colors.insert(colors.end(), { 1.0f, 1.0f, 1.0f });
                colors.insert(colors.end(), { 1.0f, 1.0f, 1.0f });
                colors.insert(colors.end(), { 1.0f, 1.0f, 1.0f });
            }
        }
        else {
            // ASCII STL
            file.close();
            file.open(filePath, std::ios::in);
            if (!file.is_open()) {
                std::cerr << "Failed to reopen STL file in ASCII mode: " << filePath << std::endl;
                return false;
            }

            vertices.clear();
            indices.clear();
            colors.clear();

            std::string line;
            size_t baseIndex = 0;
            while (std::getline(file, line)) {
                if (line.find("vertex") != std::string::npos) {
                    std::istringstream iss(line);
                    std::string vertexKeyword;
                    float x, y, z;
                    iss >> vertexKeyword >> x >> y >> z;

                    vertices.insert(vertices.end(), { x, y, z });
                    colors.insert(colors.end(), { 1.0f, 1.0f, 1.0f }); // Default to white
                }
                else if (line.find("endfacet") != std::string::npos) {
                    indices.insert(indices.end(), { static_cast<unsigned int>(baseIndex),
                                                   static_cast<unsigned int>(baseIndex + 1),
                                                   static_cast<unsigned int>(baseIndex + 2) });
                    baseIndex += 3;
                }
            }
        }

        file.close();

        init(vertices, colors, indices);
		MessageBox(NULL, L"File loaded successfully!", L"Info", MB_OK);

        return true;
    }

    Mesh* performObjectPicking(int mouseX, int mouseY) {
		// Check if the mouse coordinates intersect with the mesh's bounding box
		// If it does, return this mesh; otherwise, return nullptr
        return nullptr;
    }



    // Cleanup resources
    void cleanup() {
    }
};
