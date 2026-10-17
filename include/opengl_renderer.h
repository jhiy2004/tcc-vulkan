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
    void draw(AppInfo &info, PlaybackState &playback_state, SimulationMetadata& metadata, RenderSettings& settings) override;

    void update_frame_z_data(Frame& frame) override;
    void update_scene(float zmin, float zmax, float dt) override;
    Camera& get_camera() override;
    void recreate_swap_chain() override;
private:
    void init_imgui();
    void draw_imgui(AppInfo& info, PlaybackState& playback_state, SimulationMetadata& metadata, RenderSettings& settings);
    void create_shader();

    unsigned int shader_program;
    Camera camera;
    GLFWwindow* _window{nullptr};
};

