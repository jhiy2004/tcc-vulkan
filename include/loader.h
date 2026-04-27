#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

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

struct __attribute__((packed)) SimulationHeader {
    char type;
    float x_extent;
    float y_extent;
    uint32_t row;
    uint32_t col;
    uint32_t qtd_frames;
};

struct Frame {
    std::vector<Point> points;  
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

    std::vector<Triangle> get_triangles() const;
    std::vector<Point> get_bathymetry() const;
    std::vector<Point> get_frame_points() const;
private:
    std::ifstream file;
    int current_frame{};
    int qtd_frames{};
    char type;
    uint32_t row;
    uint32_t col;
    std::vector<Triangle> triangles;
    std::vector<Point> frame_points;
    std::vector<Point> bathymetry_points;
};
