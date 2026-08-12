#pragma once

#include "renderer/types.h"

bool vulkan_pipeline_create_graphics(VulkanContext* context, VulkanPipeline* out_pipeline);
void vulkan_pipeline_destroy(VulkanContext* context, VulkanPipeline* pipeline);
