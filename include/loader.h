#pragma once

#include <glm/glm.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstdint>

struct Point {
    float x;
    float y;
    float z;
};

struct Triangle {
    uint32_t p1;
    uint32_t p2;
    uint32_t p3;
};

#if defined(__GNUC__) || defined(__clang__)
    #define PACKED __attribute__((packed))
#elif defined(_MSC_VER)
    #define PACKED
    #pragma pack(push, 1)
#else
    #define PACKED
#endif

struct PACKED SimulationHeader {
    char type;
    float x_extent;
    float y_extent;
    uint32_t row;
    uint32_t col;
    uint32_t qtd_frames;
};

#if defined(_MSC_VER)
    #pragma pack(pop)
#endif

#undef PACKED

struct Frame {
    std::vector<float> z_data;
    float zmin;
    float zmax;
};

class FrameBuffer {
private:
    std::queue<Frame> queue;
    std::mutex mutex;
    std::condition_variable cv_not_full;
    std::condition_variable cv_not_empty;
    size_t capacity;
    bool finished = false;

public:
    FrameBuffer(size_t c);

    void push(Frame frame);
    bool pop(Frame& out);
    void set_finished();
};

class Loader {
public:
    Loader(const std::filesystem::path& filename);
    ~Loader();

    bool load_frame();

    float get_zmin() const;
    float get_zmax() const;
    std::uint32_t get_qtd_points() const;
    const std::vector<Triangle>& get_triangles() const;
    const std::vector<glm::vec2>& get_grid_xy() const;
    const std::vector<float>& get_frame_z() const;
    const std::vector<float>& get_bathymetry_z() const;
private:
    std::ifstream file;
    int current_frame{};
    int qtd_frames{};
    char type;
    uint32_t row;
    uint32_t col;
    std::vector<Triangle> triangles;
    std::vector<glm::vec2> grid_xy;
    std::vector<float> bathymetry_z;
    std::vector<float> frame_z;

    float zmin{};
    float zmax{};
};
