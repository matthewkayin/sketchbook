#pragma once

#include "renderer/types.h"

const uint32_t VULKAN_MEMORY_TYPE_INDEX_NOT_FOUND = UINT32_MAX;

uint32_t vulkan_find_memory_index(
    VulkanContext* context,
    uint32_t memory_type_filter,
    VkMemoryPropertyFlags memory_property_flags);

bool vulkan_result_is_error(VkResult result);
const char* vulkan_result_str(VkResult result);
