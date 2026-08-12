#pragma once

#include <cstdint>
#include <vector>

bool file_read_blob(const char* path, std::vector<uint8_t>* out_data);
