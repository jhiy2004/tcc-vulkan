#pragma once

#include <cstdint>

struct PlaybackState {
    uint32_t currentFrame{1};
    bool isPaused{true};
    float frameDuration{1.0f/60.0f};
};