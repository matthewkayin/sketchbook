#pragma once

#include "renderer/types.h"

void vulkan_command_buffer_begin_single_use(VulkanContext* context, VkCommandBuffer* out_command_buffer);
void vulkan_command_buffer_end_single_use(VulkanContext* context, VkCommandBuffer* command_buffer);
