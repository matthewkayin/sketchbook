#pragma once

#include "renderer/types.h"

struct VulkanImageCreateParams {
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
    VkFormat format;
    VkSampleCountFlagBits msaa_sample_count;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkImageAspectFlags aspect;
    VkMemoryPropertyFlags memory_properties;
};

struct VulkanImageViewCreateParams {
    VkImage image;
    VkFormat format;
    VkImageAspectFlags aspect;
    uint32_t mip_levels;
};

struct VulkanImageTransitionLayoutParams {
    VkCommandBuffer command_buffer;
    VulkanImage* image;
    VkImageLayout old_layout;
    VkImageLayout new_layout;
};

struct VulkanImageTransitionLayoutExtParams {
    VkCommandBuffer command_buffer;
    VkImage image;
    VkImageAspectFlags image_aspect;
    uint32_t base_mip_level;
    uint32_t mip_levels;
    VkImageLayout old_layout;
    VkImageLayout new_layout;
    VkAccessFlags2 src_access_mask;
    VkAccessFlags2 dst_access_mask;
    VkPipelineStageFlags2 src_stage_mask;
    VkPipelineStageFlags2 dst_stage_mask;
};

bool vulkan_image_create(VulkanContext* context, VulkanImageCreateParams params, VulkanImage* out_image);
bool vulkan_image_create_texture(VulkanContext* context, const char* path, VulkanImage* out_image);
bool vulkan_image_generate_mipmaps(VulkanContext* context, VkCommandBuffer command_buffer, VulkanImage* image);
bool vulkan_image_create_hatch_texture(VulkanContext* context, const char* const* paths, VulkanImage* out_images);
void vulkan_image_generate_hatch_mipmaps(VkCommandBuffer command_buffer, VulkanImage* image);
void vulkan_image_destroy(VulkanContext* context, VulkanImage* image);
void vulkan_image_view_create(VulkanContext* context, VulkanImageViewCreateParams params, VkImageView* out_image_view);
void vulkan_image_transition_layout(VulkanImageTransitionLayoutParams params);
void vulkan_image_transition_layout_ext(VulkanImageTransitionLayoutExtParams params);
