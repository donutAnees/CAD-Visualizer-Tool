#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void getOrthographicProj(GLdouble width, GLdouble height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, width, 0.0, height, -1.0, 1.0);
}

void getPerspectiveProj(GLdouble width, GLdouble height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, width / height, 0.1, 100.0);
}