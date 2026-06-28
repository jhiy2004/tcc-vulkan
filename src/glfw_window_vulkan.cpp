#include "glfw_window_vulkan.h"
#include "GLFW/glfw3.h"
#include <iostream>

void GLFWVulkanWindow::set_camera(Camera *camera) { _camera = camera; }

void GLFWVulkanWindow::key_callback(GLFWwindow *window, int key, int scancode,
                                    int action, int mods) {
  auto *self =
      static_cast<GLFWVulkanWindow *>(glfwGetWindowUserPointer(window));

  if (self) {
    self->camera_input_handler(key, action);
  }
}

void GLFWVulkanWindow::framebuffer_resize_callback(GLFWwindow *window, int width, int height) {
  auto *self =
      static_cast<GLFWVulkanWindow *>(glfwGetWindowUserPointer(window));

  self->_framebuffer_resized = true;
}

bool GLFWVulkanWindow::consume_framebuffer_resized() {
    bool resized = _framebuffer_resized;
    _framebuffer_resized = false;
    return resized;
}

void GLFWVulkanWindow::camera_input_handler(int key, int action) {
  if (!_camera)
    return;


  float angle_vel = 60.0f; 
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_W) {
      _camera->set_deltas(glm::vec3(0, 0, -100));
    }
    if (key == GLFW_KEY_S) {
      _camera->set_deltas(glm::vec3(0, 0, 100));
    }
    if (key == GLFW_KEY_A) {
      _camera->set_deltas(glm::vec3(-angle_vel, 0, 0));
    }
    if (key == GLFW_KEY_D) {
      _camera->set_deltas(glm::vec3(angle_vel, 0, 0));
    }
    if (key == GLFW_KEY_UP) {
      _camera->set_deltas(glm::vec3(0, angle_vel, 0));
    }
    if (key == GLFW_KEY_DOWN) {
      _camera->set_deltas(glm::vec3(0, -angle_vel, 0));
    }
  }

  if (action == GLFW_RELEASE) {
    if (key == GLFW_KEY_W) {
      _camera->set_deltas(glm::vec3(0));
    }
    if (key == GLFW_KEY_S) {
      _camera->set_deltas(glm::vec3(0));
    }
    if (key == GLFW_KEY_A) {
      _camera->set_deltas(glm::vec3(0));
    }
    if (key == GLFW_KEY_D) {
      _camera->set_deltas(glm::vec3(0));
    }
    if (key == GLFW_KEY_UP) {
      _camera->set_deltas(glm::vec3(0));
    }
    if (key == GLFW_KEY_DOWN) {
      _camera->set_deltas(glm::vec3(0));
    }
  }
}

void GLFWVulkanWindow::init() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  _window = glfwCreateWindow(_width, _height, _title.c_str(), NULL, NULL);

  glfwSetWindowUserPointer(_window, this);
  glfwSetKeyCallback(_window, key_callback);
  glfwSetFramebufferSizeCallback(_window, framebuffer_resize_callback);
}

void GLFWVulkanWindow::pollEvents() { glfwPollEvents(); }

bool GLFWVulkanWindow::shouldClose() { return glfwWindowShouldClose(_window); }