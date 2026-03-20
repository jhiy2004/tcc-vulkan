#include "glfw_window_vulkan.h"

void GLFWVulkanWindow::init() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    _window = glfwCreateWindow(_width, _height, _title.c_str(), NULL, NULL);
}

void GLFWVulkanWindow::pollEvents() {
    glfwPollEvents();
}

bool GLFWVulkanWindow::shouldClose() {
    return glfwWindowShouldClose(_window);
}

