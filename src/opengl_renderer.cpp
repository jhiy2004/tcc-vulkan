#include "opengl_renderer.h"
#include <GL/gl.h>

void OpenGLRenderer::init(IWindow *window) {
    return;
}

void OpenGLRenderer::draw_triangle() {
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderer::draw_rectangle() {
    return;
}
