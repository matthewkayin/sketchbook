#pragma once

#include "renderer/types.h"

struct VulkanPipelineDescriptor {
    uint32_t set;
    uint32_t binding;
    VkDescriptorType type;
    VkShaderStageFlags stage_flags;
};

struct VulkanPipelineCreateParams {
    const char* shader_path;
    uint32_t vertex_input_stride;
    uint32_t attribute_count;
    VkVertexInputAttributeDescription* attributes;
    uint32_t descriptor_count;
    VulkanPipelineDescriptor* descriptors;
    uint32_t push_constant_count;
    VkPushConstantRange* push_constants;
    VkCullModeFlags cull_mode;
};

bool vulkan_pipeline_create_graphics(VulkanContext* context, VulkanPipeline* out_pipeline);
bool vulkan_pipeline_create(VulkanContext* context, VulkanPipelineCreateParams params, VulkanPipeline* out_pipeline);
void vulkan_pipeline_destroy(VulkanContext* context, VulkanPipeline* pipeline);
