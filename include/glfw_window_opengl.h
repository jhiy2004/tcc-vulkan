#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "window.h"

#include <string_view>
#include <string>

class GLFWOpenGLWindow : public IWindow {
public:
    GLFWOpenGLWindow(uint32_t w, uint32_t h, std::string_view title)
    : _width(w), _height(h), _title(title) {}

    void init() override;
    void pollEvents() override;
    bool shouldClose() override;
    GLFWwindow* get_window() const override {
        return _window;
    }

    void set_camera(Camera *camera) override;
    bool consume_framebuffer_resized() override;

    ~GLFWOpenGLWindow() {
        glfwDestroyWindow(_window);
        glfwTerminate();
    }
private:
    uint32_t _width;
    uint32_t _height;
    std::string _title;
    GLFWwindow *_window = nullptr;
    Camera *_camera;
};
