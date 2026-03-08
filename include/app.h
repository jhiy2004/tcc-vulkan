#pragma once

#include <cstdint>
#include <string_view>
#include "renderer.h"
#include "window.h"

class App {
public:
    App(IWindow *window, IRenderer *renderer) {
        _window = window;
        _renderer = renderer;
    
        init_window();
        init_renderer();
    }

    void run();
private:
    void init_window();
    void init_renderer();

    IWindow *_window = nullptr;
    IRenderer *_renderer = nullptr;
};