#pragma once

#include <camera.h>

struct GLFWwindow;

class IWindow {
public:
    virtual ~IWindow() = default;
    virtual void init() = 0;
    virtual void pollEvents() = 0;
    virtual bool shouldClose() = 0;
    virtual void set_camera(Camera *camera) = 0;
    virtual GLFWwindow* get_window() const = 0;
    virtual bool consume_framebuffer_resized() = 0;
};

