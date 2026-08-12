#pragma once

#include "core/asserts.h"
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

#define VK_CHECK(expr)                  \
    {                                   \
        SBK_ASSERT(expr == VK_SUCCESS); \
    }

struct VulkanDevice {
    VkPhysicalDevice physical_device;
    VkDevice logical_device;

    VkPhysicalDeviceProperties properties;
    VkFormat depth_format;
    VkSampleCountFlagBits msaa_sample_count;

    uint32_t graphics_queue_index;
    uint32_t present_queue_index;
    uint32_t transfer_queue_index;
    uint32_t compute_queue_index;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkCommandPool graphics_command_pool;
};

struct VulkanContext {
    SDL_Window* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkAllocationCallbacks* allocator;

    VkSurfaceKHR surface;

    VulkanDevice device;
};
