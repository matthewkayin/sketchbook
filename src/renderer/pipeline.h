#pragma once

#include "renderer/types.h"

bool vulkan_pipeline_create_graphics(VulkanContext* context, const char* shader_path, VulkanPipeline* out_pipeline);
bool vulkan_pipeline_create_outline(VulkanContext* context, VulkanPipeline* out_pipeline);
bool vulkan_pipeline_create_shadow(VulkanContext* context, VulkanPipeline* out_pipeline);
void vulkan_pipeline_destroy(VulkanContext* context, VulkanPipeline* pipeline);
void vulkan_pipeline_bind(VulkanContext* context, VulkanPipeline* pipeline);
