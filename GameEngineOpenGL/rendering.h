#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <glm/gtc/type_ptr.hpp>
#include "camera.h"

constexpr unsigned PERSPECTIVE_MODE = 0x0000;
constexpr unsigned ORTHOGRAPHIC_MODE = 0x0001;

GLfloat vertices[] = {
    // Positions         
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
};

unsigned int indices[] = {
    // Front face
    0, 1, 2,
    2, 3, 0,

    // Back face
    4, 5, 6,
    6, 7, 4,

    // Left face
    0, 3, 7,
    7, 4, 0,

    // Right face
    1, 2, 6,
    6, 5, 1,

    // Bottom face
    0, 1, 5,
    5, 4, 0,

    // Top face
    3, 2, 6,
    6, 7, 3
};

void getOrthographicProj(GLdouble width, GLdouble height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLdouble aspectRatio = (GLdouble)width / (GLdouble)height;

    GLdouble left = -1.0;
    GLdouble right = 1.0;
    GLdouble bottom = -1.0;
    GLdouble top = 1.0;

    if (aspectRatio > 1.0) {
        left *= aspectRatio;
        right *= aspectRatio;
    }
    else if (aspectRatio < 1.0) {
        bottom *= (1.0 / aspectRatio);
        top *= (1.0 / aspectRatio);
    }
    glOrtho(left, right, bottom, top, -1000.0f, 1000.0f);

}

void getPerspectiveProj(GLdouble width, GLdouble height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, width / height, 0.1, 100.0);
}

void render(float width, float height, Camera& camera, unsigned mode = ORTHOGRAPHIC_MODE) {
    // Clear the screen
    glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (mode == ORTHOGRAPHIC_MODE) {
        getOrthographicProj(static_cast<GLdouble>(width), static_cast<GLdouble>(height));
	}
	else if (mode == PERSPECTIVE_MODE) {
        getPerspectiveProj(static_cast<GLdouble>(width), static_cast<GLdouble>(height));
	}

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glm::mat4 view = camera.getViewMatrix();
    glLoadMatrixf(glm::value_ptr(view));

    // Enable and point to the vertex array
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);

    // Draw the triangle
    glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(unsigned int), GL_UNSIGNED_INT, indices);

    // Disable the client state
    glDisableClientState(GL_VERTEX_ARRAY);
}
