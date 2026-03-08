#include <iostream>
#include <glfw/glfw3.h>
#include <glm/common.hpp>
#include <stb_image.h>
#include <imgui.h>
#include "app.h"

int main() {
    GLFWVulkanWindow vk_window(800, 600, "Vulkan App");
    VulkanRenderer vk_renderer;

    App app(&vk_window, &vk_renderer);

    app.run();
    
    return 0;
}