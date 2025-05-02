#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
	float moveSpeed;
    float MouseSensitivity;
    // Basically changing the FOV for perspective
    float Zoom;
	// Orthographic zoom factor
	float OrthoZoom;
    // Euler angles
    float Yaw;
    float Pitch;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f)){
        Position = position;
        WorldUp = up;
		moveSpeed = 45.0f;
		MouseSensitivity = 0.1f;
        Zoom = 45.0f;
		OrthoZoom = 1.0f;
		Yaw = -90.0f;
		Pitch = 0.0f;
		Front = glm::vec3(0.0f, 0.0f, 0.0f);
		updateCameraVectors();
    }

    glm::mat4 getViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void processKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = moveSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }
        updateCameraVectors();
    }

     void ProcessMouseScroll(float yoffset) {
		 Zoom -= yoffset;
		 if (Zoom < 1.0f) Zoom = 1.0f;
		 if (Zoom > 45.0f) Zoom = 45.0f;
		 OrthoZoom -= yoffset * 0.1f;
		 if (OrthoZoom < 0.1f) OrthoZoom = 0.1f;
     }
    
    void updateCameraVectors()
    {
        glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp)); 
        Up = glm::normalize(glm::cross(Right, Front));
    }

};
