#pragma once

#include <glm/vec3.hpp>
#include <glm/matrix.hpp>

class Camera {
public:
    Camera();

    glm::mat4 getViewMatrix();

    void update();

    void rotate(float deltaYaw, float deltaPitch);
    void zoom(float deltaDistance);
    void pan(glm::vec3 delta);

    glm::vec3 getPosition() const;

    void set_deltas(glm::vec3 d);

private:
    glm::vec3 target;

    float distance;
    float yaw;
    float pitch;
    glm::vec3 deltas;
};
