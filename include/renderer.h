#pragma once

#include "window.h"

class IRenderer {
public:
    virtual void init(IWindow* window) = 0;
    virtual void draw_triangle() = 0;
    virtual void draw_rectangle() = 0;
};

class OpenGLRenderer : public IRenderer {
public:
    void init(IWindow* window) override {
        return;
    }

    void draw_triangle() override {
        return;
    }

    void draw_rectangle() override {
        return;
    }
};

