#include "camera.h"

#include "glm/trigonometric.hpp"
#include <glm/ext/matrix_transform.hpp>

glm::mat4 Camera::getViewMatrix() {
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), pos);
    glm::mat4 cameraRotation = getRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::getRotationMatrix() {
    glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));

    return model;
}

void Camera::update() {
    glm::mat4 rotation = getRotationMatrix();
    pos += glm::vec3(rotation * glm::vec4(vel, 0.0f));
}

void Camera::update_vel(glm::vec3 v) {
    vel = v;
}

glm::vec3 Camera::get_vel() {
    return vel;
}
