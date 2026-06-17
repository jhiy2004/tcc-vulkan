#include "app.h"

#include <iostream>

void App::run() {
    while(!_window->shouldClose()) {
        _window->pollEvents();

        Frame frame{loader.get_frame_z()};

        _renderer->update_frame_z_data(frame);
        _renderer->update_scene();
        _renderer->draw();
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
