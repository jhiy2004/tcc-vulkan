#include "glfw_window_vulkan.h"
#include "GLFW/glfw3.h"

#include <iostream>


void GLFWVulkanWindow::set_camera(Camera *camera) {
    _camera = camera;
}

void GLFWVulkanWindow::key_callback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods)
{
    auto* self = static_cast<GLFWVulkanWindow*>(
        glfwGetWindowUserPointer(window)
    );

    if (self) {
        self->camera_input_handler(key, action);
    }
}

void GLFWVulkanWindow::camera_input_handler(int key, int action) {
    if (_camera == nullptr) {
        std::cout << "Camera is nullptr";
        return;
    }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_W) { _camera->update_vel(glm::vec3(0, 0, -1)); }
        if (key == GLFW_KEY_S) { _camera->update_vel(glm::vec3(0, 0, 1)); }
        if (key == GLFW_KEY_A) { _camera->update_vel(glm::vec3(-1, 0, 0)); }
        if (key == GLFW_KEY_D) { _camera->update_vel(glm::vec3(1, 0, 0)); }
    }

    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_W) { _camera->update_vel(glm::vec3(0)); }
        if (key == GLFW_KEY_S) { _camera->update_vel(glm::vec3(0)); }
        if (key == GLFW_KEY_A) { _camera->update_vel(glm::vec3(0)); }
        if (key == GLFW_KEY_D) { _camera->update_vel(glm::vec3(0)); }
    }

    std::cout << "(" << _camera->get_vel().x << "," << _camera->get_vel().y << "," << _camera->get_vel().z << ")" << std::endl;
    std::cout << "Input: " << key << std::endl;
}

void GLFWVulkanWindow::init() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    _window = glfwCreateWindow(_width, _height, _title.c_str(), NULL, NULL);

    glfwSetWindowUserPointer(_window, this);
    glfwSetKeyCallback(_window, key_callback);
}

void GLFWVulkanWindow::pollEvents() {
    glfwPollEvents();
}

bool GLFWVulkanWindow::shouldClose() {
    return glfwWindowShouldClose(_window);
}

