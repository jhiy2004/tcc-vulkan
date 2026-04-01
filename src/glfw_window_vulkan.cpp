#include "glfw_window_vulkan.h"

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

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

