#pragma once

struct GLFWwindow;

class IWindow {
public:
    virtual void init() = 0;
    virtual void pollEvents() = 0;
    virtual bool shouldClose() = 0;
    virtual GLFWwindow* get_window() const = 0;
};

