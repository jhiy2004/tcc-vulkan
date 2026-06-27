#include "loader.h"

#include <limits>
#include <iostream>

Loader::Loader(const std::filesystem::path& filename) : file(filename, std::ios::binary),
                                                        zmin{std::numeric_limits<float>::max()},
                                                        zmax{std::numeric_limits<float>::lowest()}
{
    if (file.is_open()) {
        SimulationHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(SimulationHeader));

        type = header.type;
        qtd_frames = header.qtd_frames;
        row = header.row;
        col = header.col;
        x_extent = header.x_extent;
        y_extent = header.y_extent;

        std::cout << "Type: " << static_cast<int>(type) << std::endl;
        std::cout << "Row: " << row << std::endl;
        std::cout << "Col: " << row << std::endl;
        std::cout << "X extent: " << x_extent << std::endl;
        std::cout << "Y extent: " << y_extent << std::endl;

        uint32_t qtd_triangles = 2 * (row-1) * (col-1);
        frame_z = std::vector<float>(row * col);
        bathymetry_z = std::vector<float>(row * col);
        grid_xy = std::vector<glm::vec2>(row * col);

        triangles = std::vector<Triangle>(qtd_triangles);

        float dx = static_cast<float>(x_extent) / (col - 1);
        float dy = static_cast<float>(y_extent) / (row - 1);
        for (uint32_t i=0; i < row; i++) {
            for (uint32_t j=0; j < col; j++) {
                uint32_t pos = i*col + j;
                grid_xy[pos].x = j * dx;
                grid_xy[pos].y = i * dy;
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

        bathymetryZMin = std::numeric_limits<float>::max();
        bathymetryZMax = std::numeric_limits<float>::lowest();

        std::vector<float> z_values(row * col);
        file.read(
            reinterpret_cast<char*>(z_values.data()),
            row * col * sizeof(float)
        );

        for (uint32_t i=0; i < row; i++) {
            for (uint32_t j=0; j < col; j++) {
                uint32_t pos = i*col + j;
                bathymetry_z[pos] = -z_values[pos];

                float value{bathymetry_z[pos]};
                
                if (value > bathymetryZMax) {
                    bathymetryZMax = value;
                }

                if (value < bathymetryZMin) {
                    bathymetryZMin = value;
                }
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

    zmin = std::numeric_limits<float>::max();
    zmax = std::numeric_limits<float>::lowest();

    if (file.is_open()) {
        std::vector<float> z_values(row * col);
        file.read(
            reinterpret_cast<char*>(z_values.data()),
            row * col * sizeof(float)
        );

        for (uint32_t i=0; i < row; i++) {
            for (uint32_t j=0; j < col; j++) {
                uint32_t pos = i*col + j;

                float value{z_values[pos]};
                
                if (value > zmax) {
                    zmax = value;
                }

                if (value < zmin) {
                    zmin = value;
                }
                
                frame_z[pos] = value;
            }
        }
        current_frame++;

        return true;
    }
    return false;
}

int Loader::get_qtd_frames() const {
    return qtd_frames;
}

const std::vector<Triangle>& Loader::get_triangles() const {
    return triangles;
}

const std::vector<glm::vec2>& Loader::get_grid_xy() const {
    return grid_xy;
}

const std::vector<float>& Loader::get_frame_z() const {
    return frame_z;
}

const std::vector<float>& Loader::get_bathymetry_z() const {
    return bathymetry_z;
}

std::uint32_t Loader::get_qtd_points() const {
    return static_cast<uint32_t>(grid_xy.size());
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


float Loader::get_zmin() const {
    return zmin;
}

float Loader::get_zmax() const {
    return zmax;
}

float Loader::get_x_extent() const {
    return x_extent;
}
float Loader::get_y_extent() const {
    return y_extent;
}

uint32_t Loader::get_row() const {
    return row;
}

uint32_t Loader::get_col() const {
    return col;
}

float Loader::get_bathymetry_zmin() const {
    return bathymetryZMin;
}

float Loader::get_bathymetry_zmax() const {
    return bathymetryZMax;
}