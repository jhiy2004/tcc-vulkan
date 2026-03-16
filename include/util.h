#pragma once
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <vector>

std::vector<char> read_file(std::filesystem::path filename);
