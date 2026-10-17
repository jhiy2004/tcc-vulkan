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

    void create_vbos(const std::vector<glm::vec2>& grid, const std::vector<float>& bathymetryZ, const std::vector<Triangle>& triangles);

    unsigned int shader_program;
    Camera camera;
    
    uint32_t _window_height = 0;
    uint32_t _window_width = 0;
    GLFWwindow* _window{nullptr};

    GLuint _vao = 0;
    GLuint _grid_xy_vbo = 0;
    GLuint _bathymetry_z_vbo = 0;
    GLuint _triangles_ebo = 0;

    GLint _mvp_location = 0;
    GLint _render_mode_location = 0;
    GLint _z_min_location = 0;
    GLint _z_max_location = 0;
    GLint _z_scale_location = 0;

    // Surface data
    GLuint _frame_z_vbo = 0;
    size_t _frame_z_capacity = 0;

    int _indices_size;
};

