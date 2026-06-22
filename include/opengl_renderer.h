#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer.h"

class OpenGLRenderer : public IRenderer {
public:
    void init(IWindow* window, 
              const std::vector<glm::vec2>& grid,
              const std::vector<float>& bathymetryZ,
              const std::vector<Triangle>& triangles
              ) override;
    void draw() override;

    void update_scene(float zmin, float zmax) override;
    Camera& get_camera() override;
private:
    Camera camera;
};

