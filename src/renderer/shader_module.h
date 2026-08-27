#pragma once

#include "renderer/types.h"

bool vulkan_shader_module_create(VulkanContext* context, const char* path, VkShaderModule* out_shader_module);
