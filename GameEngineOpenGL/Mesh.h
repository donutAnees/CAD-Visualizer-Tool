#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>

class Mesh {
public:
    // Data
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> colors;
    std::vector<unsigned int> indices;

    // OpenGL resources
    GLuint displayListID;

    // Constructor
    Mesh() : displayListID(0) {}

    // Destructor
    ~Mesh() {
        cleanup();
    }

    // Initialize the mesh (create display list)
    void initialize() {
		// Enable and point to the vertex array
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vertices.data());

		// Enable and point to the color array
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_FLOAT, 0, colors.data());

        if (displayListID == 0) {
            displayListID = glGenLists(1);
            glNewList(displayListID, GL_COMPILE);
            // Draw the model
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());
            // Disable the client state
            glEndList();
        }
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
    }

    // Render the mesh
    void render() const {
        if (displayListID != 0) {
            glCallList(displayListID);
        }
    }

    // Cleanup resources
    void cleanup() {
        if (displayListID != 0) {
            glDeleteLists(displayListID, 1);
            displayListID = 0;
        }
    }
};
