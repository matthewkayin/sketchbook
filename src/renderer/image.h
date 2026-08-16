#pragma once

#include "renderer/types.h"

const uint32_t VULKAN_LOAD_SURFACE_FLIP_V = 1U;

enum VulkanImageMipmapType {
    VULKAN_IMAGE_MIPMAP_SCALED,
    VULKAN_IMAGE_MIPMAP_SUBSET
};

struct VulkanImageCreateTextureParams {
    VulkanImageMipmapType mipmap_type;
    uint32_t surface_count;
    SDL_Surface** surfaces;
};

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

// Surface
SDL_Surface* vulkan_image_load_surface(SDL_IOStream* image_stream, uint32_t options);
size_t vulkan_image_surface_size(const SDL_Surface* surface);

// Create
bool vulkan_image_create(VulkanContext* context, VulkanImageCreateParams params, VulkanImage* out_image);
void vulkan_image_destroy(VulkanContext* context, VulkanImage* image);
void vulkan_image_view_create(VulkanContext* context, VulkanImageViewCreateParams params, VkImageView* out_image_view);

// Textures
bool vulkan_image_create_textures(VulkanContext* context, VulkanImageCreateTextureParams params, VulkanImage* out_images);
bool vulkan_image_generate_mipmaps(VulkanContext* context, VkCommandBuffer command_buffer, VulkanImageMipmapType type, VulkanImage* image);
bool vulkan_image_create_hatch_textures(VulkanContext* context, SDL_Surface* hatch_surfaces[VULKAN_HATCH_TEXTURE_CHANNEL_COUNT], VulkanImage* out_images);

// Layout
void vulkan_image_transition_layout(VulkanImageTransitionLayoutParams params);
void vulkan_image_transition_layout_ext(VulkanImageTransitionLayoutExtParams params);
