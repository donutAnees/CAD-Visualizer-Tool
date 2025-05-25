#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <iostream>
#include <glm/glm.hpp>

class Mesh {
public:
    // Data
    std::vector<GLfloat> vertices;
	// Transformed vertices, so that we can apply transformations to the original vertices
    std::vector<GLfloat> transformedVertices; 
    std::vector<GLfloat> colors;
    std::vector<unsigned int> indices;
	bool isTransparent = false; 
	GLfloat minX, maxX, minY, maxY, minZ, maxZ; // Bounding box coordinates
    glm::vec4 transformedBoundingBoxCorners[8];
	GLfloat centerX, centerY, centerZ; // Center of the mesh
	GLfloat sizeX, sizeY, sizeZ; // Size of the mesh in each dimension
	GLfloat rotationX, rotationY, rotationZ; // Rotation angles in degrees
    bool showBoundingBox = false;
    bool showVertices = false;
	// Model Matrix to apply transformations
	glm::mat4 modelMatrix = glm::mat4(1.0f); 

    // Constructor
    Mesh() {}

    // Destructor
    ~Mesh() {
		vertices.clear();
		transformedVertices.clear();
		indices.clear();
		colors.clear();
    }

    void calculateBounds() {
        minX = transformedVertices[0];
        maxX = transformedVertices[0];
        minY = transformedVertices[1];
        maxY = transformedVertices[1];
        minZ = transformedVertices[2];
        maxZ = transformedVertices[2];

        for (size_t i = 0; i < transformedVertices.size(); i += 3) {
            minX = std::min(minX, transformedVertices[i]);
            maxX = std::max(maxX, transformedVertices[i]);
            minY = std::min(minY, transformedVertices[i + 1]);
            maxY = std::max(maxY, transformedVertices[i + 1]);
            minZ = std::min(minZ, transformedVertices[i + 2]);
            maxZ = std::max(maxZ, transformedVertices[i + 2]);
        }

        // Calculate the size of the object
        sizeX = maxX - minX;
        sizeY = maxY - minY;
        sizeZ = maxZ - minZ;
        
        // Set up the transformed corners as the default corners
        glm::vec4 corners[8] = {
			{minX, minY, minZ, 1.0f}, {maxX, minY, minZ, 1.0f},
			{maxX, minY, maxZ, 1.0f}, {minX, minY, maxZ, 1.0f},
			{minX, maxY, minZ, 1.0f}, {maxX, maxY, minZ, 1.0f},
			{maxX, maxY, maxZ, 1.0f}, {minX, maxY, maxZ, 1.0f}
		};

        for (int i = 0; i < 8; ++i) {
            transformedBoundingBoxCorners[i] = corners[i];
        }

    }

    glm::vec3 computeOriginalCenter() {
        float minX = vertices[0], maxX = vertices[0];
        float minY = vertices[1], maxY = vertices[1];
        float minZ = vertices[2], maxZ = vertices[2];

        for (size_t i = 0; i < vertices.size(); i += 3) {
            float x = vertices[i];
            float y = vertices[i + 1];
            float z = vertices[i + 2];

            if (x < minX) minX = x; if (x > maxX) maxX = x;
            if (y < minY) minY = y; if (y > maxY) maxY = y;
            if (z < minZ) minZ = z; if (z > maxZ) maxZ = z;
        }

        return glm::vec3((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f);
    }


    void init(const std::vector<GLfloat>& vertices, const std::vector<GLfloat>& colors, const std::vector<unsigned int>& indices) {
		this->vertices = vertices;
		this->transformedVertices = vertices; // Initialize transformed vertices with original vertices
		this->colors = colors;
		this->indices = indices;

		// Calculate the bounding box
		calculateBounds();

        // Calculate the center of the object
        centerX = (minX + maxX) / 2.0f;
        centerY = (minY + maxY) / 2.0f;
        centerZ = (minZ + maxZ) / 2.0f;
        
		// Set the rotation angles to 0
		rotationX = 0.0f;
		rotationY = 0.0f;
		rotationZ = 0.0f;
    }

	// Set vertices and colors
	void setVertices(const std::vector<GLfloat>& vertices) {
		this->vertices = vertices;
		this->transformedVertices = vertices;
	}

	void setColors(const std::vector<GLfloat>& colors) {
		this->colors = colors;
	}

	// Set indices
	void setIndices(const std::vector<unsigned int>& indices) {
		this->indices = indices;
	}
    
    glm::vec3 getCenter() {
		return glm::vec3(centerX, centerY, centerZ);
    }

    glm::vec3 getSize() {
        return glm::vec3(sizeX, sizeY, sizeZ);
    }

    // Render the mesh
    void draw() const {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, transformedVertices.data());

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

        if (showBoundingBox) {
            drawBoundingBox();
        }

        if (showVertices) {
            drawVertices();
        }

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
        glVertex3f(centerX + 2, centerY, centerZ);

        // Y-axis (green) scaled to the object's size
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(centerX, centerY, centerZ);
        glVertex3f(centerX, centerY + 2, centerZ);

        // Z-axis (blue) scaled to the object's size
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(centerX, centerY, centerZ);
        glVertex3f(centerX, centerY, centerZ + 2);

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
        return true;
    }

    void updateMesh() {
        if (vertices.empty()) {
            return;
        }

        float rx = glm::radians(rotationX);
        float ry = glm::radians(rotationY);
        float rz = glm::radians(rotationZ);

        // Rotation matrices
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), rx, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), ry, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), rz, glm::vec3(0.0f, 0.0f, 1.0f));

        // Combine rotations
        glm::mat4 rotationMatrix = rotZ * rotY * rotX;

        modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerX, centerY, centerZ)) *
                                rotationMatrix *
			// Rotation must happen around the center of the object
			// Since the object is centered at X,Y,Z, we first convert it to 0,0,0
			// Then we rotate it, and finally move it back to the original position
            glm::translate(glm::mat4(1.0f), -computeOriginalCenter());

        glm::vec3 center(centerX, centerY, centerZ);
        
        for (size_t i = 0; i < vertices.size(); i += 3) {
            glm::vec4 vertex = glm::vec4(vertices[i], vertices[i + 1], vertices[i + 2], 1.0f);
            glm::vec4 transformed = modelMatrix * vertex;

            transformedVertices[i] = transformed.x;
            transformedVertices[i + 1] = transformed.y;
            transformedVertices[i + 2] = transformed.z;
        }

        // Update the bounding box
        // Define the 8 corners of the bounding box in local space
        glm::vec4 corners[8] = {
            {minX, minY, minZ, 1.0f}, {maxX, minY, minZ, 1.0f},
            {maxX, minY, maxZ, 1.0f}, {minX, minY, maxZ, 1.0f},
            {minX, maxY, minZ, 1.0f}, {maxX, maxY, minZ, 1.0f},
            {maxX, maxY, maxZ, 1.0f}, {minX, maxY, maxZ, 1.0f}
        };

        // Transform the corners and store them
        for (int i = 0; i < 8; ++i) {
            transformedBoundingBoxCorners[i] = modelMatrix * corners[i];
        }

    }

    // Draws points at each vertex of the mesh
    void drawVertices() const {
        if (transformedVertices.empty()) return;

        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glColor3f(0.0f, 0.0f, 0.0f); 

        for (size_t i = 0; i < transformedVertices.size(); i += 3) {
            glVertex3f(transformedVertices[i], transformedVertices[i + 1], transformedVertices[i + 2]);
        }

        glEnd();
    }

    // Draws a wireframe bounding box around the mesh
    void drawBoundingBox() const {
        if (transformedVertices.empty()) return;

        // Draw the bounding box using the transformed corners
        glColor3f(1.0f, 1.0f, 0.0f); // Yellow color
        glBegin(GL_LINES);

        // Bottom face
        glVertex3f(transformedBoundingBoxCorners[0].x, transformedBoundingBoxCorners[0].y, transformedBoundingBoxCorners[0].z);
        glVertex3f(transformedBoundingBoxCorners[1].x, transformedBoundingBoxCorners[1].y, transformedBoundingBoxCorners[1].z);

        glVertex3f(transformedBoundingBoxCorners[1].x, transformedBoundingBoxCorners[1].y, transformedBoundingBoxCorners[1].z);
        glVertex3f(transformedBoundingBoxCorners[2].x, transformedBoundingBoxCorners[2].y, transformedBoundingBoxCorners[2].z);

        glVertex3f(transformedBoundingBoxCorners[2].x, transformedBoundingBoxCorners[2].y, transformedBoundingBoxCorners[2].z);
        glVertex3f(transformedBoundingBoxCorners[3].x, transformedBoundingBoxCorners[3].y, transformedBoundingBoxCorners[3].z);

        glVertex3f(transformedBoundingBoxCorners[3].x, transformedBoundingBoxCorners[3].y, transformedBoundingBoxCorners[3].z);
        glVertex3f(transformedBoundingBoxCorners[0].x, transformedBoundingBoxCorners[0].y, transformedBoundingBoxCorners[0].z);

        // Top face
        glVertex3f(transformedBoundingBoxCorners[4].x, transformedBoundingBoxCorners[4].y, transformedBoundingBoxCorners[4].z);
        glVertex3f(transformedBoundingBoxCorners[5].x, transformedBoundingBoxCorners[5].y, transformedBoundingBoxCorners[5].z);

        glVertex3f(transformedBoundingBoxCorners[5].x, transformedBoundingBoxCorners[5].y, transformedBoundingBoxCorners[5].z);
        glVertex3f(transformedBoundingBoxCorners[6].x, transformedBoundingBoxCorners[6].y, transformedBoundingBoxCorners[6].z);

        glVertex3f(transformedBoundingBoxCorners[6].x, transformedBoundingBoxCorners[6].y, transformedBoundingBoxCorners[6].z);
        glVertex3f(transformedBoundingBoxCorners[7].x, transformedBoundingBoxCorners[7].y, transformedBoundingBoxCorners[7].z);

        glVertex3f(transformedBoundingBoxCorners[7].x, transformedBoundingBoxCorners[7].y, transformedBoundingBoxCorners[7].z);
        glVertex3f(transformedBoundingBoxCorners[4].x, transformedBoundingBoxCorners[4].y, transformedBoundingBoxCorners[4].z);

        // Vertical lines
        glVertex3f(transformedBoundingBoxCorners[0].x, transformedBoundingBoxCorners[0].y, transformedBoundingBoxCorners[0].z);
        glVertex3f(transformedBoundingBoxCorners[4].x, transformedBoundingBoxCorners[4].y, transformedBoundingBoxCorners[4].z);

        glVertex3f(transformedBoundingBoxCorners[1].x, transformedBoundingBoxCorners[1].y, transformedBoundingBoxCorners[1].z);
        glVertex3f(transformedBoundingBoxCorners[5].x, transformedBoundingBoxCorners[5].y, transformedBoundingBoxCorners[5].z);

        glVertex3f(transformedBoundingBoxCorners[2].x, transformedBoundingBoxCorners[2].y, transformedBoundingBoxCorners[2].z);
        glVertex3f(transformedBoundingBoxCorners[6].x, transformedBoundingBoxCorners[6].y, transformedBoundingBoxCorners[6].z);

        glVertex3f(transformedBoundingBoxCorners[3].x, transformedBoundingBoxCorners[3].y, transformedBoundingBoxCorners[3].z);
        glVertex3f(transformedBoundingBoxCorners[7].x, transformedBoundingBoxCorners[7].y, transformedBoundingBoxCorners[7].z);

        glEnd();
    }

    void toggleBoundingBox() {
        showBoundingBox = !showBoundingBox;
    }

    void toggleVertices() {
        showVertices = !showVertices;
    }


};
