#include "opengl_renderer.h"

void OpenGLRenderer::init(IWindow *window, 
                          const std::vector<glm::vec2>& grid,
                          const std::vector<float>& bathymetryZ,
                          const std::vector<Triangle>& triangles
                          ) {
    return;
}

void OpenGLRenderer::draw(AppInfo &info) {
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

Camera& OpenGLRenderer::get_camera() {
    return camera;
}


void OpenGLRenderer::update_scene(float zmin, float zmax) {}
