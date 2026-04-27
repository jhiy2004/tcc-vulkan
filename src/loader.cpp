#include "loader.h"

#include <iostream>

Loader::Loader(const std::filesystem::path& filename) : file(filename, std::ios::binary)
{
    if (file.is_open()) {
        SimulationHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(SimulationHeader));

        type = header.type;
        qtd_frames = header.qtd_frames;
        row = header.row;
        col = header.col;

        std::cout << "Type: " << static_cast<int>(type) << std::endl;
        std::cout << "Row: " << row << std::endl;
        std::cout << "Col: " << row << std::endl;
        std::cout << "X extent: " << header.x_extent << std::endl;
        std::cout << "Y extent: " << header.y_extent << std::endl;

        uint32_t qtd_triangles = 2 * (row-1) * (col-1);
        frame_points = std::vector<Point>(row * col);
        triangles = std::vector<Triangle>(qtd_triangles);

        float dx = static_cast<float>(header.x_extent) / (col - 1);
        float dy = static_cast<float>(header.y_extent) / (row - 1);
        for (int i=0; i < row; i++) {
            for (int j=0; j < col; j++) {
                uint32_t pos = i*col + j;
                frame_points[pos].x = j * dx;
                frame_points[pos].y = i * dy;
            }
        }

        uint32_t t = 0;
        for (uint32_t i = 0; i < row - 1; i++) {
            for (uint32_t j = 0; j < col - 1; j++) {
                uint32_t i0 = i * col + j;
                uint32_t i1 = i * col + (j + 1);
                uint32_t i2 = (i + 1) * col + j;
                uint32_t i3 = (i + 1) * col + (j + 1);

                triangles[t++] = { i0, i1, i2 };
                triangles[t++] = { i1, i3, i2 };
            }
        }

        bathymetry_points = std::vector<Point>(frame_points);

        std::vector<float> z_values(row * col);
        file.read(
            reinterpret_cast<char*>(z_values.data()),
            row * col * sizeof(float)
        );

        for (int i=0; i < row; i++) {
            for (int j=0; j < col; j++) {
                uint32_t pos = i*col + j;
                bathymetry_points[pos].z = z_values[pos];
            }
        }
    }
}

Loader::~Loader() {
    file.close();
}

bool Loader::load_frame() {
    if (current_frame >= qtd_frames) {
        return false;
    }

    if (file.is_open()) {
        std::vector<float> z_values(row * col);
        file.read(
            reinterpret_cast<char*>(z_values.data()),
            row * col * sizeof(float)
        );

        for (int i=0; i < row; i++) {
            for (int j=0; j < col; j++) {
                uint32_t pos = i*col + j;
                frame_points[pos].z = z_values[pos];
            }
        }
        current_frame++;

        return true;
    }
    return false;
}

std::vector<Triangle> Loader::get_triangles() const {
    return triangles;
}

std::vector<Point> Loader::get_bathymetry() const {
    return bathymetry_points;
}

std::vector<Point> Loader::get_frame_points() const {
    return frame_points;
}

FrameBuffer::FrameBuffer(size_t c) : capacity(c) {}

void FrameBuffer::push(Frame frame) {
    std::unique_lock<std::mutex> lock(mutex);

    cv_not_full.wait(lock, [this]() {
        return queue.size() < capacity || finished;
    });

    if (finished)
        return;

    queue.push(std::move(frame));

    lock.unlock();
    cv_not_empty.notify_one();
}

bool FrameBuffer::pop(Frame& out) {
    std::unique_lock<std::mutex> lock(mutex);

    cv_not_empty.wait(lock, [this]() {
        return !queue.empty() || finished;
    });

    if (queue.empty())
        return false;

    out = std::move(queue.front());
    queue.pop();

    lock.unlock();
    cv_not_full.notify_one();

    return true;
}

void FrameBuffer::set_finished() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        finished = true;
    }

    cv_not_empty.notify_all();
    cv_not_full.notify_all();
}
