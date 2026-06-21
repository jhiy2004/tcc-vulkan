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

void print_frame(Frame &frame) {
    std::cout << "Points:\n";
    for (float z : frame.z_data) {
        if (z > 0) {
            std::cout << "(" << z << ")" << std::endl;
        }
    }
}

void frame_producer(FrameBuffer& fb, Loader& loader) {
    Frame frame;
    while (true) {
        bool res = loader.load_frame();
        if (!res) {
            break;
        }

        frame.z_data = loader.get_frame_z();
        fb.push(std::move(frame));
    }

    fb.set_finished();
}

void consumer(FrameBuffer& fb) {
    Frame frame;
    int count = 1;
    while (true) {
        bool res = fb.pop(frame);
        if (!res) {
            break;
        }
        count++;
        //std::cout << "loading frame " << count << std::endl;
        print_frame(frame);
    }
}

int main() {
    /*
    Loader loader(std::filesystem::path(FILES_DIR) / "example.sim");
    FrameBuffer fb(10);

    std::thread producer(frame_producer, std::ref(fb), std::ref(loader));

    consumer(fb);

    producer.join();
    return 0;
    */

    std::cout << "Started application" << std::endl;

    GLFWVulkanWindow vk_window(800, 600, "Vulkan App");
    //GLFWOpenGLWindow gl_window(800, 600, "OpenGL App");
    //OpenGLRenderer gl_renderer;
    VulkanRenderer vk_renderer;

    App app(&vk_window, &vk_renderer, std::filesystem::path(FILES_DIR) / "example.sim");

    app.run();
    
    return 0;
}
