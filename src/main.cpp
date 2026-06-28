#include "glfw_window_opengl.h"
#include "glfw_window_vulkan.h"

#include <iostream>
#include <glm/common.hpp>
#include <stb_image.h>
#include <imgui.h>
#include "app.h"
#include "loader.h"
#include "vulkan_renderer.h"
#include "opengl_renderer.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <simulation_file.sim>" << std::endl;
        return 1;
    }

    std::cout << "Started application" << std::endl;

    GLFWVulkanWindow vk_window(800, 600, "Vulkan App");
    //GLFWOpenGLWindow gl_window(800, 600, "OpenGL App");
    //OpenGLRenderer gl_renderer;
    VulkanRenderer vk_renderer;

    App app(&vk_window, &vk_renderer, std::filesystem::path(FILES_DIR) / argv[1]);

    app.run();

    std::cout << "Exited app.run()" << std::endl;

    return 0;
}
