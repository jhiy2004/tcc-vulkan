#pragma once

#include <filesystem>
#include "loader.h"
#include "renderer.h"
#include "window.h"
#include <thread>
#include <atomic>
#include <mutex>
#include "app_info.h"
#include "simulation_metadata.h"
#include "playback_state.h"

#include <iostream>

class App {
public:
    App(IWindow *window, IRenderer *renderer, std::filesystem::path filename) : loader(filename), fb(10) {
        _window = window;
        _renderer = renderer;

        metadata.rows = loader.get_row();
        metadata.cols = loader.get_col();
        metadata.xExtent = loader.get_x_extent();
        metadata.yExtent = loader.get_y_extent();
        metadata.bathymetryZMax = loader.get_bathymetry_zmax();
        metadata.bathymetryZMin = loader.get_bathymetry_zmin();

        // Send basic simulation data to the renderer
        _renderer->set_metadata(metadata);

        init_window();
        init_renderer();

       _producer_thread = std::thread(&App::frame_producer, this);
        {
            std::lock_guard lock(_producerMutex);
            _restartProducer = true;
        }

        _producerCv.notify_one();
    }

    ~App() {
        std::cout << "App Destructor start\n";

        _running = false;
        fb.set_finished();

        _producerCv.notify_one();

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

    std::atomic<bool> _running = true;
    std::thread _producer_thread;
    std::atomic_bool _restartProducer{false};
    std::mutex _loaderMutex;

    std::mutex _producerMutex;
    std::condition_variable _producerCv;

    Loader loader;
    FrameBuffer fb;
    IWindow *_window = nullptr;
    IRenderer *_renderer = nullptr;
    AppInfo info;
    PlaybackState playback_state;
    SimulationMetadata metadata;
};
