#pragma once

#include <filesystem>
#include "loader.h"
#include "renderer.h"
#include "window.h"
#include <thread>

class App {
public:
    App(IWindow *window, IRenderer *renderer, std::filesystem::path filename) : loader(filename), fb(10) {
        _window = window;
        _renderer = renderer;
    
        init_window();
        init_renderer();

       _producer_thread = std::thread(&App::frame_producer, this);
    }
    
    ~App() {
        if (_producer_thread.joinable()) {
            _producer_thread.join();
        }
    }

    void run();
private:
    void init_window();
    void init_renderer();
    void init_loader();
    void frame_producer();

    std::thread _producer_thread;
    Loader loader;
    FrameBuffer fb;
    IWindow *_window = nullptr;
    IRenderer *_renderer = nullptr;
};
