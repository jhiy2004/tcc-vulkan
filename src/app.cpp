#include "app.h"

#include <iostream>

void App::run()
{
    using clock = std::chrono::high_resolution_clock;

    auto last = clock::now();

    float playbackTime = 0.0f;

    info.set_frame_duration(0.008f);

    Frame currentFrame;

    fb.pop(currentFrame);

    while (!_window->shouldClose()) {
        auto now = clock::now();

        float dt = std::chrono::duration<float>(
            now - last
        ).count();
        info.set_last_dt(dt);

        last = now;

        // nunca atrasa a câmera
        _window->pollEvents();

        // controla apenas a simulação
        playbackTime += dt;

        float frameDuration{info.get_frame_duration()};
        if (playbackTime >= frameDuration) {
            Frame next;

            if (fb.pop(next)) {
                currentFrame = std::move(next);

                _renderer->update_frame_z_data(currentFrame);
            }

            playbackTime -= frameDuration;
            info.increment_frame_count();
        }

        _renderer->update_scene(
            currentFrame.zmin,
            currentFrame.zmax,
            dt
        );
        _renderer->draw(info);
    }

    std::cout << "Closed application" << std::endl;
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

    _renderer->init(
        _window,
        loader.get_grid_xy(),
        loader.get_bathymetry_z(),
        loader.get_triangles(),
        loader.get_x_extent(),
        loader.get_y_extent()
    );
    
    std::cout << "_renderer inicializado com sucesso\n";
}

void App::frame_producer() {
    Frame frame;

    while (_running) {
        bool res = loader.load_frame();

        if (!res) {
            break;
        }

        fb.push(std::move(frame));
    }
    fb.set_finished();
}