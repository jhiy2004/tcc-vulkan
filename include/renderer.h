#pragma once

#include "camera.h"
#include "loader.h"
#include "window.h"
#include "app_info.h"
#include "simulation_metadata.h"
#include "playback_state.h"

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void init(IWindow* window, 
                      const std::vector<glm::vec2>& grid,
                      const std::vector<float>& bathymetryZ,
                      const std::vector<Triangle>& triangles,
                      float x_extent,
                      float y_extent
                      ) = 0;
    virtual void draw(AppInfo& info, PlaybackState& playback_state) = 0;
    virtual void update_frame_z_data(Frame& frame) = 0;
    virtual void update_scene(float zmin, float zmax, float dt) = 0;
    virtual Camera& get_camera() = 0;
    virtual void set_metadata(const SimulationMetadata& metadata) = 0;
    virtual void recreate_swap_chain() = 0;
};