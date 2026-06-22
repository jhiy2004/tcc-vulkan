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

    int get_frame_count() const {
        return frame_count;
    }

    float get_frame_duration() const {
        return frame_duration;
    }

    void set_frame_duration(float duration) {
        frame_duration = duration;
    }

    void set_last_dt(float dt) {
        last_dt = dt;
    }

    void increment_frame_count() {
        frame_count++;
    }
private:
    float last_dt{};
    int frame_count{1};
    float frame_duration{1.0f/60.0f};
};