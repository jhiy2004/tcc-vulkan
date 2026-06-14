#include "glfw_window_opengl.h"
#include <stdexcept>

void GLFWOpenGLWindow::init() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    _window = glfwCreateWindow(_width, _height, _title.c_str(), NULL, NULL);
    if (_window == NULL)
    {
        glfwTerminate();
        throw std::runtime_error("Falha ao criar a janela");
    }
    glfwMakeContextCurrent(_window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Falha ao inicializar o glad");
    }
    glViewport(0, 0, _width, _height);
}

void GLFWOpenGLWindow::pollEvents() {
    glfwSwapBuffers(_window);
    glfwPollEvents();
}

bool GLFWOpenGLWindow::shouldClose() {
    return glfwWindowShouldClose(_window);
}

void GLFWOpenGLWindow::set_camera(Camera *camera) {
    _camera = camera;
}
