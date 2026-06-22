#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer.h"

#include "app_info.h"

class OpenGLRenderer : public IRenderer {
public:
    void init(IWindow* window, 
              const std::vector<glm::vec2>& grid,
              const std::vector<float>& bathymetryZ,
              const std::vector<Triangle>& triangles
              ) override;
    void draw(AppInfo& info) override;

    void update_scene(float zmin, float zmax) override;
    Camera& get_camera() override;
private:
    Camera camera;
};

