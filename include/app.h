#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>
#include "loader.h"
#include "renderer.h"
#include "window.h"

class App {
public:
    App(IWindow *window, IRenderer *renderer, std::filesystem::path filename) : loader(filename) {
        _window = window;
        _renderer = renderer;
    
        init_window();
        init_renderer();

        // TODO: Remove this later
        loader.load_frame();
    }

    void run();
private:
    void init_window();
    void init_renderer();
    void init_loader();

    Loader loader;
    IWindow *_window = nullptr;
    IRenderer *_renderer = nullptr;
};
