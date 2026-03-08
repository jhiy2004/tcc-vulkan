#pragma once

class IRenderer {
public:
    virtual void init() = 0;
    virtual void draw_triangle() = 0;
    virtual void draw_rectangle() = 0;
};

class OpenGLRenderer : public IRenderer {
public:
    void init() override {
        return;
    }

    void draw_triangle() override {
        return;
    }

    void draw_rectangle() override {
        return;
    }
};

class VulkanRenderer : public IRenderer {
public:
    void init() override {
        return;
    }

    void draw_triangle() override {
        return;
    }

    void draw_rectangle() override {
        return;
    }
};
