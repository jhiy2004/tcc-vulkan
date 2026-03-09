#include "app.h"

#include <iostream>

void App::run() {
    while(!_window->shouldClose()) {
        _window->pollEvents();
    }
}

void App::init_window() {
    if (!_window) {
        std::cerr << "Falha ao inicializar _window";
        return;
    }

    _window->init();
    std::cout << "_window inicializado com sucesso\n";
}

void App::init_renderer() {
    if (!_renderer) {
        std::cerr << "Falha ao inicializar _renderer";
        return;
    }

    _renderer->init();
    std::cout << "_renderer inicializado com sucesso\n";
}
