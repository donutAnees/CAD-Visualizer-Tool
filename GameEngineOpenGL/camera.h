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

    Camera(){
        position = glm::vec3(0.0f, 10.0f, -10.0f);
        angle = glm::vec3(-45.0f, 90.0f, 0.0f);
		moveSpeed = 10.0f;
		mouseSensitivity = 0.1f;
        zoom = FOV;
        nearPlane = NEAR_PLANE;
        farPlane = FAR_PLANE;
		mode = PERSPECTIVE_MODE;
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

	void setCameraZoom(float zoom) {
		this->zoom = zoom;
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
		this->mode = mode;
	}

	// Rotate the camera using delta x and y (in degrees), affects pitch and yaw
	void rotateBy(float dx, float dy) {
		angle.x += dy * mouseSensitivity; // pitch (around X)
		angle.y += dx * mouseSensitivity; // yaw (around Y)

		// Clamp pitch to avoid flipping
		if (angle.x > 89.0f) angle.x = 89.0f;
		if (angle.x < -89.0f) angle.x = -89.0f;

		// Update the view matrix after rotation
		updateViewMatrix();
	}

	void move(Camera_Movement direction, float deltaTime) {
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

	glm::mat4 getViewMatrix() {
		return viewMatrix;
	}

	// We are using a free look camera, therefore Euler angles are used to calculate the camera direction
	void updateViewMatrix() {
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

 };
