#include "glfw_window_opengl.h"
#include "glfw_window_vulkan.h"

#include <iostream>
#include <glm/common.hpp>
#include <stb_image.h>
#include <imgui.h>
#include "app.h"
#include "vulkan_renderer.h"
#include "opengl_renderer.h"


int main() {
    //GLFWVulkanWindow vk_window(800, 600, "Vulkan App");
    GLFWOpenGLWindow gl_window(800, 600, "OpenGL App");
    OpenGLRenderer gl_renderer;
    //VulkanRenderer vk_renderer;

    App app(&gl_window, &gl_renderer);

    app.run();
    
    return 0;
}
