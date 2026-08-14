#pragma once

#include "core/math.h"
#include <vector>
#include <cstdint>

bool renderer_load_model(const char* path, std::vector<Vertex3d>* out_vertices, std::vector<uint32_t>* out_indices);
