#pragma once

#include <glm/vec3.hpp>
#include <glm/matrix.hpp>

class Camera {
public:
    Camera() : pos(0), vel(0), rot(0) {}
    Camera(glm::vec3 p, glm::vec3 v, glm::vec3 r) : pos(p), vel(v), rot(r) {}

    glm::mat4 getViewMatrix();
    glm::mat4 getRotationMatrix();

    void update();
    void update_vel(glm::vec3 v);

    glm::vec3 get_vel();
private:
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec3 rot;
};
