#pragma once
#include <glm/glm.hpp>

class ViewProjMethodGLM {
public:
    virtual ~ViewProjMethodGLM() = default;

    // Returns the projection matrix (either perspective or orthographic).
    virtual glm::mat4 getProjectionMatrix() const = 0;

    // Optional: Set a transformation matrix (e.g., camera transform) to apply
    // after the projection.
    void setTransform(const glm::mat4& transform) {
        this->transform = transform;
        hasTransform = true;
    }

    // Returns the combined projection * transform matrix.
    // If no transform is set, just returns the projection matrix.
    glm::mat4 getComposedProjectionMatrix() const {
        return hasTransform ? getProjectionMatrix() * transform : getProjectionMatrix();
    }

protected:
    // Transformation matrix to be optionally applied after projection.
    glm::mat4 transform = glm::mat4(1.0f);
    bool hasTransform = false;
};

class PerspectiveProj : public ViewProjMethodGLM {
public:
    PerspectiveProj(float fov, float aspect, float nearPlane, float farPlane);

    glm::mat4 getProjectionMatrix() const override;

private:
	float fov;        // Field of view in degree, controls how tall the frustum is, y axis
	float aspect;     // Aspect ratio (width / height), controls how wide the frustum is, x axis
    float nearPlane;  // Distance to near clipping plane
    float farPlane;   // Distance to far clipping plane
};

class OrthoProj : public ViewProjMethodGLM {
public:
    OrthoProj(float left, float right, float bottom, float top, float nearPlane, float farPlane);

    glm::mat4 getProjectionMatrix() const override;

private:
    float left;       // Left boundary of the view volume
    float right;      // Right boundary
    float bottom;     // Bottom boundary
    float top;        // Top boundary
    float nearPlane;  // Near clipping plane
    float farPlane;   // Far clipping plane
};

