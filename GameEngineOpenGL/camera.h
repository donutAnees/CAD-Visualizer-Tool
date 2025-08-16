#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <sstream>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
	UP,
	DOWN
};

constexpr unsigned int PERSPECTIVE_MODE = 0;
constexpr unsigned int ORTHOGRAPHIC_MODE = 1;

constexpr float FOV = 45.0f;
constexpr float NEAR_PLANE = 0.1f;
constexpr float FAR_PLANE = 1000.0f;

class Camera {
public:
	glm::vec3 position; // Position of the camera at the world space, the camera looks at the -z axis
    glm::vec3 target;   // Camera look at position at the target
	glm::vec3 angle;    // Camera angle - YAW (AROUND Y), PITCH (AROUND X), ROLL (AROUND Z)
    float nearPlane;    // Nearest plane from camera, where the object are rendered 
	float farPlane;     // Farthest plane from camera, where the object are rendered
	float moveSpeed;
    float mouseSensitivity;
    // Tracks the FOV, can be converted to get the zoom level
    float zoom;
	// Camera mode - PERSPECTIVE_MODE or ORTHOGRAPHIC_MODE
	unsigned int mode;
	// View Matrix
	glm::mat4 viewMatrix;

	// Orbit camera parameters
	bool orbitMode;
	glm::vec3 orbitTarget;
	float orbitDistance;

    Camera(){
        position = glm::vec3(0.0f, 10.0f, -10.0f);
        angle = glm::vec3(-45.0f, 90.0f, 0.0f);
		moveSpeed = 10.0f;
		mouseSensitivity = 0.1f;
        nearPlane = NEAR_PLANE;
        farPlane = FAR_PLANE;
		mode = PERSPECTIVE_MODE;
		orbitMode = false;
		orbitTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		orbitDistance = 10.0f;
		zoom = (ORTHOGRAPHIC_MODE) ? 1/10.0f : FOV;
		// Initialize the view matrix
		updateViewMatrix();
    }

	void setCameraPosition(float x, float y, float z) {
		position = glm::vec3(x, y, z);
	}

	void setCameraAngle(float yaw, float pitch, float roll) {
		angle = glm::vec3(yaw, pitch, roll);
	}

	void setCameraYaw(float yaw) {
		angle.y = yaw;
	}

	void setCameraPitch(float pitch) {
		angle.x = pitch;
	}

	void setCameraRoll(float roll) {
		angle.z = roll;
	}

	void setCameraNearPlane(float nearPlane) {
		this->nearPlane = nearPlane;
	}

	void setCameraFarPlane(float farPlane) {
		this->farPlane = farPlane;
	}

	void setCameraMoveSpeed(float speed) {
		moveSpeed = speed;
	}

	void setCameraMouseSensitivity(float sensitivity) {
		mouseSensitivity = sensitivity;
	}

	void setCameraMode(unsigned int mode) {
		if (this->mode == mode) return;  // No change needed
		
		if (mode == ORTHOGRAPHIC_MODE) {
			zoom = 1.0f;
			// When switching to orthographic, adjust near/far planes to allow wider viewing range
			nearPlane = -FAR_PLANE;  // Negative near plane ensures objects in front are visible
			farPlane = FAR_PLANE;
		}
		else if (mode == PERSPECTIVE_MODE) {
			zoom = FOV;
			// Reset to default perspective near/far planes
			nearPlane = NEAR_PLANE;
			farPlane = FAR_PLANE;
		}
		this->mode = mode;
	}

	void zoomIn(float amount) {
		if (mode == ORTHOGRAPHIC_MODE) {
			// Increase zoom 
			zoom *= (1.0f + amount);
			if (zoom > 10.0f) zoom = 10.0f;
			if (zoom < 0.01f) zoom = 0.01f;
		}
		else if (mode == PERSPECTIVE_MODE) {
			// Decrease FOV 
			zoom -= amount * 10.0f;
			if (zoom < 10.0f) zoom = 10.0f;
			if (zoom > 120.0f) zoom = 120.0f;
		}
	}

	void zoomOut(float amount) {
		if (mode == ORTHOGRAPHIC_MODE) {
			// Decrease zoom
			zoom /= (1.0f + amount);
			if (zoom > 10.0f) zoom = 10.0f;
			if (zoom < 0.01f) zoom = 0.01f;
		}
		else if (mode == PERSPECTIVE_MODE) {
			// Increase FOV
			zoom += amount * 10.0f;
			if (zoom < 10.0f) zoom = 10.0f;
			if (zoom > 120.0f) zoom = 120.0f;
		}
	}

	void setOrbitMode(bool enabled, const glm::vec3& target = glm::vec3(0.0f, 0.0f, 0.0f), float distance = 10.0f) {
		if (enabled == orbitMode) return; 
		// When in orbit mode, the camera looks at the orbit target
		// but in free flow, it doesn't have any target to look at
		// therefore switching has to be done in a way that, we consider
		// the target of orbit camera when switching to free camera mode

		if (enabled) {
			// Switching to orbit mode
			orbitMode = true;
			orbitTarget = target;

			glm::vec3 offset = position - orbitTarget;
			orbitDistance = glm::length(offset);

			if (orbitDistance > 0.0001f) {
				angle.x = glm::degrees(asin(offset.y / orbitDistance)); // pitch
				angle.y = glm::degrees(atan2(offset.z, offset.x));      // yaw
			}

			updateOrbitCameraViewMatrix();
		}
		else {
			// Switching to free camera
			orbitMode = false;

			// Set angle so free camera looks toward orbitTarget (same as in orbit)
			glm::vec3 direction = glm::normalize(orbitTarget - position);
			angle.x = glm::degrees(asin(direction.y));                     // pitch
			angle.y = glm::degrees(atan2(direction.z, direction.x));       // yaw

			updateFreeCameraViewMatrix();
		}
	}

	glm::vec3 getPosition() const {
		return position;
	}

	bool isOrbitMode() const {
		return orbitMode;
	}

	// Rotate the camera using delta x and y (in degrees), affects pitch and yaw
	void rotateBy(float dx, float dy) {
		angle.x += dy * mouseSensitivity; // pitch (around X)
		angle.y += dx * mouseSensitivity; // yaw (around Y)

		// Clamp pitch to avoid flipping
		if (angle.x > 89.0f) angle.x = 89.0f;
		if (angle.x < -89.0f) angle.x = -89.0f;

		// Update the view matrix after rotation
		if (orbitMode) {
			updateOrbitCameraViewMatrix();
		}
		else {
			updateViewMatrix();
		}
	}

	void move(Camera_Movement direction, float deltaTime) {
		if (orbitMode) {
			// In orbit mode, we don't move the camera directly, but rather adjust the orbit distance
			if (direction == FORWARD) {
				orbitDistance -= moveSpeed * deltaTime;
			}
			else if (direction == BACKWARD) {
				orbitDistance += moveSpeed * deltaTime;
			}
			updateOrbitCameraViewMatrix();
			return;
		}
		else {
			glm::vec3 front;
			front.x = cos(glm::radians(angle.y)) * cos(glm::radians(angle.x));
			front.y = sin(glm::radians(angle.x));
			front.z = sin(glm::radians(angle.y)) * cos(glm::radians(angle.x));
			front = glm::normalize(front);

			glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
			glm::vec3 up = glm::normalize(glm::cross(right, front));

			float velocity = moveSpeed * deltaTime;

			if (direction == FORWARD)
				position += front * velocity;
			if (direction == BACKWARD)
				position -= front * velocity;
			if (direction == LEFT)
				position -= right * velocity;
			if (direction == RIGHT)
				position += right * velocity;
			if (direction == UP)
				position += up * velocity;
			if (direction == DOWN)
				position -= up * velocity;

			// Update the view matrix after moving
			updateViewMatrix();
		}
	}

	glm::mat4 getViewMatrix() {
		return viewMatrix;
	}

	// We are using a free look camera, therefore Euler angles are used to calculate the camera direction
	void updateViewMatrix() {
		if (orbitMode) {
			updateOrbitCameraViewMatrix();
		}
		else {
			updateFreeCameraViewMatrix();
		}
	}

	void updateFreeCameraViewMatrix() {
		glm::vec3 front;
		front.x = cos(glm::radians(angle.y)) * cos(glm::radians(angle.x));
		front.y = sin(glm::radians(angle.x));
		front.z = sin(glm::radians(angle.y)) * cos(glm::radians(angle.x));
		front = glm::normalize(front);

		glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
		glm::vec3 up = glm::normalize(glm::cross(right, front));

		viewMatrix = glm::lookAt(position, position + front, up);
	}

	void updateOrbitCameraViewMatrix() {
        float yaw = glm::radians(angle.y);
        float pitch = glm::radians(angle.x);

        position.x = orbitTarget.x + orbitDistance * cos(pitch) * cos(yaw);
        position.y = orbitTarget.y + orbitDistance * sin(pitch);
        position.z = orbitTarget.z + orbitDistance * cos(pitch) * sin(yaw);

        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        viewMatrix = glm::lookAt(position, orbitTarget, up);
    }

	void zoomToBoundingBox(glm::vec3 center, glm::vec3 size, float aspectRatio) {
		if (mode == ORTHOGRAPHIC_MODE) {
			// Find the largest dimension, adjust for aspect ratio
			float width = size.x;
			float height = size.y;
			float depth = size.z;
			float viewWidth = width;
			float viewHeight = height;
			if (aspectRatio > 1.0f) {
				viewWidth = std::max(width, height * aspectRatio);
				viewHeight = viewWidth / aspectRatio;
			}
			else {
				viewHeight = std::max(height, width / aspectRatio);
				viewWidth = viewHeight * aspectRatio;
			}
			// Set zoom so the object fits
			zoom = 2.0f / std::max(viewWidth, viewHeight); 
			
			// Center position on the object
			position = center; 
			updateFreeCameraViewMatrix();
		}
		else if (mode == PERSPECTIVE_MODE) {
			float radius = glm::length(size) * 0.5f;

			float fovY = glm::radians(zoom);
			float fovX = 2.0f * atanf(tanf(fovY / 2.0f) * aspectRatio);

			float distanceY = radius / tan(fovY / 2.0f);
			float distanceX = radius / tan(fovX / 2.0f);
			float distance = std::max(distanceY, distanceX) * 1.1f; // Add some margin
			
			// Place the camera at the required distance from the center, looking at the center
			if (orbitMode) {
				orbitTarget = center;
				orbitDistance = distance;
				updateOrbitCameraViewMatrix();
			}
			else {
				// Move the camera along its current forward direction, but so it looks at the center
				glm::vec3 front;
				front.x = cos(glm::radians(angle.y)) * cos(glm::radians(angle.x));
				front.y = sin(glm::radians(angle.x));
				front.z = sin(glm::radians(angle.y)) * cos(glm::radians(angle.x));
				front = glm::normalize(front);
				position = center - front * distance;
				updateFreeCameraViewMatrix();
			}
		}
	}
 };
