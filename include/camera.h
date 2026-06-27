#pragma once

#include <iostream>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>

class Camera {
public:
    Camera();

    glm::mat4 getViewMatrix();

    void update(float dt);

    void rotate(float deltaYaw, float deltaPitch);
    void zoom(float deltaDistance);
    void pan(glm::vec3 delta);

    glm::vec3 getPosition() const;

    void set_deltas(glm::vec3 d);

    float getYaw() const;
    float getPitch() const;
    float getDistance() const;

    void setYaw(float value);
    void setPitch(float value);
    void setDistance(float value);
    void setTarget(glm::vec3&& t);

    friend std::ostream& operator<<(std::ostream& os, const Camera& c) {
        glm::vec3 pos{c.getPosition()};

        std::cout << "Target: (" << c.target.x << "," << c.target.y << "," << c.target.z << ")" << std::endl;
        std::cout << "Pos: (" << pos.x << "," << pos.y << "," << pos.z << ")" << std::endl;
    }
private:
    glm::vec3 target;

    float distance;
    float yaw;
    float pitch;
    glm::vec3 deltas;
};
