#include "window.h"

void GLFWVulkanWindow::init() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(_width, _height, _title.c_str(), NULL, NULL);
}

void GLFWVulkanWindow::pollEvents() {
    glfwPollEvents();
}

bool GLFWVulkanWindow::shouldClose() {
    return glfwWindowShouldClose(_window);
}