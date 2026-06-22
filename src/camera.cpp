#include "camera.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>

Camera::Camera()
    : target(0.0f, 0.0f, 0.0f),
      distance(200.0f),
      yaw(0.0f),
      pitch(-30.0f),
      deltas(0.0f)
{
}

glm::vec3 Camera::getPosition() const
{
    float pitchRad = glm::radians(pitch);
    float yawRad   = glm::radians(yaw);

    glm::vec3 position;

    position.x =
        target.x +
        distance * cos(pitchRad) * sin(yawRad);

    position.y =
        target.y +
        distance * sin(pitchRad);

    position.z =
        target.z +
        distance * cos(pitchRad) * cos(yawRad);

    return position;
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(
        getPosition(),
        target,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

void Camera::set_deltas(glm::vec3 d) {
    deltas = d;
}

void Camera::update()
{
    rotate(deltas.x, deltas.y);
    zoom(deltas.z);

    glm::vec3 pos = getPosition();

    std::cout << "Target: (" << target.x << "," << target.y << "," << target.z << ")" << std::endl;
    std::cout << "Pos: (" << pos.x << "," << pos.y << "," << pos.z << ")" << std::endl;
}

void Camera::rotate(float deltaYaw, float deltaPitch)
{
    yaw += deltaYaw;

    setPitch(pitch + deltaPitch);
}

void Camera::zoom(float deltaDistance)
{
    distance += deltaDistance;

    if (distance < 1.0f)
    {
        distance = 1.0f;
    }
}

void Camera::pan(glm::vec3 delta)
{
    target += delta;
}

float Camera::getYaw() const {
    return yaw;
}
float Camera::getPitch() const {
    return pitch;
}

float Camera::getDistance() const {
    return distance;
}

void Camera::setYaw(float value) {
    yaw = value;
}
void Camera::setPitch(float value) {
    pitch = glm::clamp(
        value,
        -89.0f,
        89.0f
    );
}

void Camera::setDistance(float value) {
    distance = value;
}