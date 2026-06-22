#include "app.h"

#include <iostream>

void App::run()
{
    using clock = std::chrono::high_resolution_clock;

    auto last = clock::now();

    float playbackTime = 0.0f;
    float frameDuration = 0.033f; // in seconds
    uint32_t count{1};

    Frame currentFrame;

    fb.pop(currentFrame);

    while (!_window->shouldClose())
    {
        auto now = clock::now();

        float dt = std::chrono::duration<float>(
            now - last
        ).count();

        last = now;

        std::cout << "FPS: " << 1/dt << std::endl;

        // nunca atrasa a câmera
        _window->pollEvents();

        // controla apenas a simulação
        playbackTime += dt;

        if (playbackTime >= frameDuration) {
            Frame next;

            if (fb.pop(next)) {
                currentFrame = std::move(next);

                _renderer->update_frame_z_data(currentFrame);
            }

            playbackTime -= frameDuration;
            count++;
        }

        _renderer->update_scene(
            currentFrame.zmin,
            currentFrame.zmax
        );
        _renderer->draw();

        std::cout << "Current Frame: " << count << std::endl;
    }
}

void App::init_window() {
    if (!_window) {
        std::cerr << "Falha ao inicializar _window";
        return;
    }

    _window->set_camera(&_renderer->get_camera());
    _window->init();
    std::cout << "_window inicializado com sucesso\n";
}

void App::init_renderer() {
    if (!_renderer) {
        std::cerr << "Falha ao inicializar _renderer";
        return;
    }

    _renderer->init(_window, loader.get_grid_xy(), loader.get_bathymetry_z(), loader.get_triangles());
    std::cout << "_renderer inicializado com sucesso\n";
}

void App::frame_producer() {
    Frame frame;
    while (true) {
        bool res = loader.load_frame();
        if (!res) {
            break;
        }

        frame.z_data = loader.get_frame_z();
        frame.zmin = loader.get_zmin();
        frame.zmax = loader.get_zmax();
        fb.push(std::move(frame));
    }

    fb.set_finished();
}
