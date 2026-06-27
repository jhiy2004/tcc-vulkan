#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer.h"

#include "app_info.h"
#include "simulation_metadata.h"

class OpenGLRenderer : public IRenderer {
public:
    void init(IWindow* window, 
              const std::vector<glm::vec2>& grid,
              const std::vector<float>& bathymetryZ,
              const std::vector<Triangle>& triangles,
              float x_extent,
              float y_extent
              ) override;
    void draw(AppInfo& info) override;

    void update_scene(float zmin, float zmax, float dt) override;
    Camera& get_camera() override;
    void set_metadata(const SimulationMetadata& metadata) override;
private:
    Camera camera;
    SimulationMetadata _metadata;
};

