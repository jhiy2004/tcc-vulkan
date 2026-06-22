#pragma once

#include "camera.h"
#include "loader.h"
#include "window.h"
#include "app_info.h"

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void init(IWindow* window, 
                      const std::vector<glm::vec2>& grid,
                      const std::vector<float>& bathymetryZ,
                      const std::vector<Triangle>& triangles
                      ) = 0;
    virtual void draw(AppInfo& info) = 0;
    virtual void update_frame_z_data(Frame& frame) = 0;
    virtual void update_scene(float zmin, float zmax) = 0;
    virtual Camera& get_camera() = 0;
};

