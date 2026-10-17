#include "glfw_window_opengl.h"
#include <stdexcept>

void GLFWOpenGLWindow::init() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    _window = glfwCreateWindow(
        _width, _height, _title.c_str(), NULL, NULL
    );

    if (_window == NULL) {
        glfwTerminate();
        throw std::runtime_error("Falha ao criar a janela");
    }

    glfwMakeContextCurrent(_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Falha ao inicializar o glad");
    }

    // Input: permite que os callbacks acessem esta instância.
    glfwSetWindowUserPointer(_window, this);
    glfwSetKeyCallback(_window, key_callback);
    glfwSetFramebufferSizeCallback(_window, framebuffer_resize_callback);

    glViewport(0, 0, _width, _height);
}

void GLFWOpenGLWindow::pollEvents() {
    glfwSwapBuffers(_window);
    glfwPollEvents();
}

bool GLFWOpenGLWindow::shouldClose() {
    return glfwWindowShouldClose(_window);
}

void GLFWOpenGLWindow::set_camera(Camera* camera) {
    _camera = camera;
}

bool GLFWOpenGLWindow::consume_framebuffer_resized() {
    bool resized = _framebuffer_resized;
    _framebuffer_resized = false;
    return resized;
}

void GLFWOpenGLWindow::key_callback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods
) {
    auto* self = static_cast<GLFWOpenGLWindow*>(
        glfwGetWindowUserPointer(window)
    );

    if (!self) {
        return;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    self->camera_input_handler(key, action);
}

void GLFWOpenGLWindow::framebuffer_resize_callback(
    GLFWwindow* window,
    int width,
    int height
) {
    auto* self = static_cast<GLFWOpenGLWindow*>(
        glfwGetWindowUserPointer(window)
    );

    if (!self) {
        return;
    }

    self->_framebuffer_resized = true;

    if (width <= 0 || height <= 0) {
        return;
    }

    self->_width = static_cast<uint32_t>(width);
    self->_height = static_cast<uint32_t>(height);

    glViewport(0, 0, width, height);
}

void GLFWOpenGLWindow::camera_input_handler(int key, int action) {
    if (!_camera) {
        return;
    }

    constexpr float angle_vel = 60.0f;

    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_W:
            _camera->set_deltas(glm::vec3(0, 0, -100));
            break;
        case GLFW_KEY_S:
            _camera->set_deltas(glm::vec3(0, 0, 100));
            break;
        case GLFW_KEY_A:
            _camera->set_deltas(glm::vec3(-angle_vel, 0, 0));
            break;
        case GLFW_KEY_D:
            _camera->set_deltas(glm::vec3(angle_vel, 0, 0));
            break;
        case GLFW_KEY_UP:
            _camera->set_deltas(glm::vec3(0, angle_vel, 0));
            break;
        case GLFW_KEY_DOWN:
            _camera->set_deltas(glm::vec3(0, -angle_vel, 0));
            break;
        default:
            break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_W:
        case GLFW_KEY_S:
        case GLFW_KEY_A:
        case GLFW_KEY_D:
        case GLFW_KEY_UP:
        case GLFW_KEY_DOWN:
            _camera->set_deltas(glm::vec3(0.0f));
            break;
        default:
            break;
        }
    }
}