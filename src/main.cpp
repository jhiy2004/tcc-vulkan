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


int main() {
    Loader loader(std::filesystem::path(FILES_DIR) / "example.sim");

    int count = 0;
    std::cout << "Points:\n";
    for (Point p : loader.get_bathymetry()) {
        std::cout << "(" << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;

        if (count > 100) {
            break;
        }

        count++;
    }

    count = 0;
    std::cout << "Triangles:\n";
    for (Triangle t : loader.get_triangles()) {
        std::cout << "(" << t.p1 << ", " << t.p2 << ", " << t.p3 << ")" << std::endl;

        if (count > 100) {
            break;
        }

        count++;
    }


    return 0;

    GLFWVulkanWindow vk_window(800, 600, "Vulkan App");
    //GLFWOpenGLWindow gl_window(800, 600, "OpenGL App");
    //OpenGLRenderer gl_renderer;
    VulkanRenderer vk_renderer;

    App app(&vk_window, &vk_renderer);

    app.run();
    
    return 0;
}
