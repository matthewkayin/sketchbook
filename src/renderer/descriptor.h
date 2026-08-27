#pragma once

#include "renderer/types.h"

void vulkan_descriptor_sets_create(VulkanContext* context);
void vulkan_descriptor_sets_destroy(VulkanContext* context);

bool vulkan_uniform_objects_create(VulkanContext* context);
void vulkan_uniform_objects_destroy(VulkanContext* context);
