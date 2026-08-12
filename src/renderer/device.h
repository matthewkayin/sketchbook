#pragma once

#include "renderer/types.h"

bool vulkan_device_create(VulkanContext* context);
void vulkan_device_destroy(VulkanContext* context);

bool vulkan_device_get_supported_depth_format(VulkanDevice* device, VkImageTiling tiling, VkFormatFeatureFlags features);
