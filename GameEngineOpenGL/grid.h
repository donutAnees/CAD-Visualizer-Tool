#pragma once
#include <GL/gl.h>
#include "camera.h"

class Grid {
public:
	float size;
	int divisions;
	Camera *camera;
	Grid(Camera& camera) : size(100), divisions(100), camera(&camera) {}

	// Draws a grid on the XZ plane
	void drawXZGrid() {
        glDisable(GL_LIGHTING);
        glColor3f(0.7f, 0.7f, 0.7f);
        glLineWidth(1.0f);

        glm::vec3 cameraPos = camera->position;

        float step = (2 * size) / divisions;
        float halfGrid = size;

        // Grid doesn't move always with the camera, only moves when the camera crosses each step
		// Thereby the grid doesnt move with the camera but only snaps if it crosses the step
        // Snap grid origin to camera's position aligned to the grid, rounding off to the nearest step
        float originX = step * std::floor(cameraPos.x / step);
        float originZ = step * std::floor(cameraPos.z / step);


        glBegin(GL_LINES);
        for (int i = 0; i <= divisions; ++i) {
            float offset = -halfGrid + i * step;

            // Lines parallel to X-axis (varying Z)
            glVertex3f(originX - halfGrid, 0.0f, originZ + offset);
            glVertex3f(originX + halfGrid, 0.0f, originZ + offset);

            // Lines parallel to Z-axis (varying X)
            glVertex3f(originX + offset, 0.0f, originZ - halfGrid);
            glVertex3f(originX + offset, 0.0f, originZ + halfGrid);
        }
        glEnd();
    }
};