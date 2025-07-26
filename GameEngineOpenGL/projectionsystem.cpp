#include "ProjectionSystem.h"
#include <glm/gtc/matrix_transform.hpp>

PerspectiveProj::PerspectiveProj(float fov, float aspect, float nearPlane, float farPlane)
    : fov(fov), aspect(aspect), nearPlane(nearPlane), farPlane(farPlane) {
}

glm::mat4 PerspectiveProj::getProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

OrthoProj::OrthoProj(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    : left(left), right(right), bottom(bottom), top(top), nearPlane(nearPlane), farPlane(farPlane) {
}

glm::mat4 OrthoProj::getProjectionMatrix() const {
    return glm::ortho(left, right, bottom, top, nearPlane, farPlane);
}