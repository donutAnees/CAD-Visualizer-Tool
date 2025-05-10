#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <sstream>

class Mesh {
public:
    // Data
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> colors;
    std::vector<unsigned int> indices;

    // Constructor
    Mesh() {}

    // Destructor
    ~Mesh() {
        cleanup();
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
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

    }

    // Cleanup resources
    void cleanup() {
    }
};
