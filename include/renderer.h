#pragma once

#include "window.h"

class IRenderer {
public:
    virtual void init(IWindow* window) = 0;
    virtual void draw_triangle() = 0;
    virtual void draw_rectangle() = 0;
};

