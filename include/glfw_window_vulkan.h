#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.h"
#include <iostream>

#include <string_view>
#include <string>

class GLFWVulkanWindow : public IWindow {
public:
    GLFWVulkanWindow(uint32_t w, uint32_t h, std::string_view title)
    : _width(w), _height(h), _title(title) {}

    void init() override;
    void pollEvents() override;
    bool shouldClose() override;
    GLFWwindow* get_window() const override {
        return _window;
    }

    void set_camera(Camera *camera) override;
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void camera_input_handler(int key, int action);

    ~GLFWVulkanWindow() {
        std::cout << "Destroy glfw vulkan window" << std::endl;
        
        glfwDestroyWindow(_window);
        glfwTerminate();
    }
private:
    uint32_t _width;
    uint32_t _height;
    std::string _title;
    GLFWwindow *_window = nullptr;
    Camera *_camera = nullptr;
};
