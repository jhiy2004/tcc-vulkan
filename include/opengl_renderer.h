#pragma once

#include "renderer.h"

class OpenGLRenderer : public IRenderer {
public:
    void init(IWindow* window) override;
    void draw_triangle() override;
    void draw_rectangle() override;
};

