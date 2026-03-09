#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <stb_image.h>
#include <imgui.h>
#include "app.h"
#include "vulkan_renderer.h"

int main() {
    GLFWVulkanWindow vk_window(800, 600, "Vulkan App");
    VulkanRenderer vk_renderer;

    App app(&vk_window, &vk_renderer);

    app.run();
    
    return 0;
}
