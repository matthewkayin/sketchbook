#pragma once

#include "renderer/types.h"

struct VulkanModelCreateGeometryParams {
    uint32_t vertex_count;
    Vertex3d* vertices;
    uint32_t index_count;
    uint32_t* indices;
};

bool vulkan_model_load(VulkanContext* context, const char* path, VulkanModel* out_model);
bool vulkan_model_create_geometry(VulkanContext* context, VulkanModelCreateGeometryParams params, VulkanModel* out_model);
void vulkan_model_destroy(VulkanContext* context, VulkanModel* model);
void vulkan_model_render(VulkanContext* context, const VulkanModel& model, mat4 transform, bool use_material);
