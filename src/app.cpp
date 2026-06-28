#include "app.h"

#include <iostream>

void App::run()
{
    using clock = std::chrono::high_resolution_clock;

    auto last = clock::now();

    float playbackTime = 0.0f;

    playback_state.frameDuration = 0.008f;
    info.set_qtd_frames(loader.get_qtd_frames());

    Frame currentFrame;

    fb.pop(currentFrame);

    while (!_window->shouldClose()) {
        if (_window->consume_framebuffer_resized()) {
            _renderer->recreate_swap_chain();
        }
        

        uint32_t prevFrame = playback_state.currentFrame;

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

        float frameDuration{playback_state.frameDuration};
        if (playbackTime >= frameDuration) {
            if (!playback_state.isPaused && fb.pop(currentFrame)) {
                _renderer->update_frame_z_data(currentFrame);
                
                playback_state.currentFrame++;
            }

            playbackTime -= frameDuration;
        }

        _renderer->update_scene(
            currentFrame.zmin,
            currentFrame.zmax,
            dt
        );
        _renderer->draw(info, playback_state);

        if (playback_state.isPaused && prevFrame != playback_state.currentFrame) {
        #ifndef NDEBUG
            std::cerr << "Changed frame\n";
        #endif

            {
                std::scoped_lock lock(_loaderMutex);

                loader.set_current_frame(playback_state.currentFrame);
                fb.clear();
                fb.reset_cancel();
            }

            {
                std::lock_guard lock(_producerMutex);
                _restartProducer = true;
            }

            _producerCv.notify_one();

            // Gambiarra
            fb.pop(currentFrame);

            // Load user chosen frame
            if (fb.pop(currentFrame)) {
                std::cerr << "Loaded user chosen frame with zmin=" << currentFrame.zmin << ", zmax=" << currentFrame.zmax << "\n";

                _renderer->update_frame_z_data(currentFrame);
            }
        }
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
    while (_running) {
        {
            std::unique_lock lock(_producerMutex);

            _producerCv.wait(lock, [&] {
                return !_running || _restartProducer;
            });

            if (!_running)
                break;

            _restartProducer = false;
        }

        while (_running) {
            Frame frame;
            bool res;
            {
                std::scoped_lock lock(_loaderMutex);

                res = loader.load_frame();

                if (!res) {
                    fb.set_finished();
                    break;
                }

                frame.z_data = loader.get_frame_z();
                frame.zmin   = loader.get_zmin();
                frame.zmax   = loader.get_zmax();
            }

            fb.push(std::move(frame));

            // Usuário mudou o frame enquanto produzíamos?
            if (_restartProducer) {
                std::cerr << "frame_producer: reiniciando produção de frames\n";
                break;
            };
        }
    }
}