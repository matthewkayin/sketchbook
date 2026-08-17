#pragma once

#include "renderer/types.h"

bool vulkan_model_load(VulkanContext* context, const char* path, VulkanModel* out_model);
void vulkan_model_destroy(VulkanContext* context, VulkanModel* model);
void vulkan_model_render(VulkanContext* context, VulkanModel* model);
