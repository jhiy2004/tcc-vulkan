#pragma once

class AppInfo {
public:
    AppInfo() = default;

    int get_fps() const {
        if (last_dt == 0) {
            return 0;
        }

        return static_cast<int>(1.0f / last_dt);
    }

    int get_qtd_frames() const {
        return qtd_frames;
    }

    void set_last_dt(float dt) {
        last_dt = dt;
    }

    void set_qtd_frames(int value) {
        qtd_frames = value;
    }
private:
    float last_dt{};
    int qtd_frames{};
};