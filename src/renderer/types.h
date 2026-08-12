#pragma once

#include "core/asserts.h"
#include "core/math.h"
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <vector>

const uint32_t VULKAN_MAX_FRAMES_IN_FLIGHT = 2U;

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

struct VulkanImage {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;

    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
};

struct VulkanSwapchain {
    VkSwapchainKHR handle;
    VkSurfaceFormatKHR image_format;
    VkExtent2D extent;
    VulkanImage color_attachment;
    VulkanImage depth_attachment;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
};

struct VulkanPipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VkDescriptorSetLayout descriptor_set_layout;
};

struct VulkanBuffer {
    VkBuffer handle;
    VkDeviceMemory memory;
};

// TODO: move this into the frontend or something?
struct VulkanUniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    uint8_t padding[64];
};
static_assert(sizeof(VulkanUniformBufferObject) == 256ULL, "Some Nvidia cards require this to be exactly 256 bytes.");

struct VulkanContext {
    SDL_Window* window;

    // Vulkan objects
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    VulkanDevice device;
    VulkanSwapchain swapchain;
    VulkanPipeline graphics_pipeline;

    // Sync
    VkSemaphore acquire_semaphores[VULKAN_MAX_FRAMES_IN_FLIGHT];
    std::vector<VkSemaphore> submit_semaphores;
    VkFence frame_fences[VULKAN_MAX_FRAMES_IN_FLIGHT];

    // Command buffers
    VkCommandBuffer graphics_command_buffers[VULKAN_MAX_FRAMES_IN_FLIGHT];

    // Descriptors
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_sets[VULKAN_MAX_FRAMES_IN_FLIGHT];

    // Vertex and index buffers
    VulkanBuffer vertex_buffer;
    VulkanBuffer index_buffer;
    VulkanBuffer uniform_buffer;

    uint32_t frame_index;
    uint32_t image_index;
};
