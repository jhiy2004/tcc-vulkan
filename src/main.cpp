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
    int count = 0;
    std::cout << "Points:\n";
    for (Point p : frame.points) {
        std::cout << "(" << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;

        if (count > 100) {
            break;
        }

        count++;
    }
}

void frame_producer(FrameBuffer& fb, Loader& loader) {
    Frame frame;
    while (true) {
        bool res = loader.load_frame();
        if (!res) {
            break;
        }

        frame.points = loader.get_frame_points();
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
        std::cout << "loading frame " << count << std::endl;
        //print_frame(frame);
    }
}

int main() {
    Loader loader(std::filesystem::path(FILES_DIR) / "example.sim");
    FrameBuffer fb(10);

    std::thread producer(frame_producer, std::ref(fb), std::ref(loader));

    consumer(fb);

    producer.join();
    return 0;

    GLFWVulkanWindow vk_window(800, 600, "Vulkan App");
    //GLFWOpenGLWindow gl_window(800, 600, "OpenGL App");
    //OpenGLRenderer gl_renderer;
    VulkanRenderer vk_renderer;

    App app(&vk_window, &vk_renderer);

    app.run();
    
    return 0;
}
