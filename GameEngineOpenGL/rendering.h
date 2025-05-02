#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "camera.h"
#include "Mesh.h"

constexpr unsigned PERSPECTIVE_MODE = 0x0000;
constexpr unsigned ORTHOGRAPHIC_MODE = 0x0001;

Mesh cubeMesh;

void initializeCubeMesh() {
    // Define the cube's vertices, colors, and indices
    cubeMesh.vertices = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
    };

    cubeMesh.colors = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 0.5f, 0.5f,
        0.5f, 1.0f, 1.0f,
        1.0f, 1.5f, 1.5f,
        1.5f, 1.5f, 1.5f,
    };

    cubeMesh.indices = {
        0, 1, 2, 2, 3, 0, // Front face
        4, 5, 6, 6, 7, 4, // Back face
        0, 3, 7, 7, 4, 0, // Left face
        1, 2, 6, 6, 5, 1, // Right face
        0, 1, 5, 5, 4, 0, // Bottom face
        3, 2, 6, 6, 7, 3  // Top face
    };
    cubeMesh.initialize();
}


void getOrthographicProj(GLdouble width, GLdouble height, Camera& camera) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLdouble aspectRatio = (GLdouble)width / (GLdouble)height;

    GLdouble left = -1.0 / camera.OrthoZoom;
    GLdouble right = 1.0 / camera.OrthoZoom;
    GLdouble bottom = -1.0 / camera.OrthoZoom;
    GLdouble top = 1.0 / camera.OrthoZoom;

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

void getPerspectiveProj(GLdouble width, GLdouble height, Camera& camera) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera.Zoom, width / height, 0.1, 100.0);
}

void render(float width, float height, Camera& camera, unsigned mode = ORTHOGRAPHIC_MODE) {
    // Clear the screen
    glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (mode == ORTHOGRAPHIC_MODE) {
        getOrthographicProj(static_cast<GLdouble>(width), static_cast<GLdouble>(height), camera);
	}
	else if (mode == PERSPECTIVE_MODE) {
        getPerspectiveProj(static_cast<GLdouble>(width), static_cast<GLdouble>(height), camera);
	}

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glm::mat4 view = camera.getViewMatrix();
    glLoadMatrixf(glm::value_ptr(view));
	cubeMesh.render();
}
