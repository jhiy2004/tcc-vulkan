#pragma once

#include "loader.h"
#include "window.h"

class IRenderer {
public:
    virtual void init(IWindow* window, 
                      const std::vector<glm::vec2>& grid,
                      const std::vector<float>& bathymetryZ,
                      const std::vector<Triangle>& triangles
                      ) = 0;
    virtual void draw() = 0;
    virtual void update_frame_z_data(Frame& frame) = 0;
};

