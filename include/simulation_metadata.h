#pragma once

#include <cstdint>

struct SimulationMetadata {
    float xExtent;
    float yExtent;

    float bathymetryZMin;
    float bathymetryZMax;

    uint32_t rows;
    uint32_t cols;
};