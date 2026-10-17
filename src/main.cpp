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

    //GLFWVulkanWindow window(800, 600, "Vulkan App");
    //VulkanRenderer renderer;

    
    GLFWOpenGLWindow window(800, 600, "OpenGL App");
    OpenGLRenderer renderer;
    

    App app(&window, &renderer, std::filesystem::path(FILES_DIR) / argv[1]);

    app.run();

    std::cout << "Exited app.run()" << std::endl;

    return 0;
}
