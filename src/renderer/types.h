#pragma once

#include "core/asserts.h"
#include "core/math.h"
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <vector>

const uint32_t VULKAN_MAX_FRAMES_IN_FLIGHT = 2U;

const uint32_t VULKAN_HATCH_TEXTURE_CHANNEL_COUNT = 6U;
const uint32_t VULKAN_HATCH_TEXTURE_IMAGE_COUNT = 2U;
const uint32_t VULKAN_HATCH_CHANNELS_PER_IMAGE = VULKAN_HATCH_TEXTURE_CHANNEL_COUNT / VULKAN_HATCH_TEXTURE_IMAGE_COUNT;

const uint32_t VULKAN_DESCRIPTOR_POOL_MAX_SETS = 64U;
const uint32_t VULKAN_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_COUNT = 48U;

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
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
};

struct VulkanBuffer {
    VkBuffer handle;
    VkDeviceMemory memory;
};

// MODEL

const uint32_t VULKAN_MESH_MATERIAL_NONE = UINT32_MAX;
const uint32_t VULKAN_NODE_MESH_NONE = UINT32_MAX;
const uint32_t VULKAN_NODE_PARENT_NONE = UINT32_MAX;

struct VulkanPrimitive {
    uint32_t first_index;
    uint32_t index_count;
    uint32_t material_index;
};

struct VulkanMesh {
    std::vector<VulkanPrimitive> primitives;
};

struct VulkanNode {
    mat4 local_transform;
    uint32_t mesh_index;
    uint32_t parent_index;
    std::vector<uint32_t> child_indices;
};

struct VulkanModel {
    VulkanBuffer vertex_buffer;
    VulkanBuffer index_buffer;

    std::vector<VulkanMesh> meshes;
    std::vector<VulkanNode> nodes;
    std::vector<VkDescriptorSet> material_descriptor_sets;
    std::vector<VulkanImage> textures;
};

struct VulkanModelRenderParams {
    uint32_t model_index;
    mat4 transform;
};

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
    VulkanPipeline outline_pipeline;
    VulkanPipeline* bound_pipeline;

    // Sync
    VkSemaphore acquire_semaphores[VULKAN_MAX_FRAMES_IN_FLIGHT];
    std::vector<VkSemaphore> submit_semaphores;
    VkFence frame_fences[VULKAN_MAX_FRAMES_IN_FLIGHT];

    // Command buffers
    VkCommandBuffer graphics_command_buffers[VULKAN_MAX_FRAMES_IN_FLIGHT];

    // Descriptors
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet global_descriptor_sets[VULKAN_MAX_FRAMES_IN_FLIGHT];

    // Shader resources
    VulkanBuffer uniform_buffers[VULKAN_MAX_FRAMES_IN_FLIGHT];
    VulkanBuffer light_data_buffer;
    VulkanImage hatch_textures[VULKAN_HATCH_TEXTURE_IMAGE_COUNT];
    VulkanImage hatch_textures2[VULKAN_HATCH_TEXTURE_IMAGE_COUNT];
    VkSampler texture_sampler;
    VulkanImage fallback_texture;

    // Model
    std::vector<VulkanModel> model_data;
    std::vector<VulkanModelRenderParams> model_render_queue;

    uint32_t frame_index;
    uint32_t image_index;
};
