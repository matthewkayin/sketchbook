#pragma once

#include "renderer/types.h"

struct VulkanBufferCreateParams {
    uint64_t size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory_properties;
    bool bind_on_create;
};

struct VulkanBufferMapMemoryParams {
    uint64_t offset;
    uint64_t size;
};

struct VulkanBufferLoadDataParams {
    uint64_t offset;
    uint64_t size;
    const void* data;
};

struct VulkanBufferCopyParams {
    VkBuffer src_buffer;
    uint64_t src_offset;
    VkBuffer dst_buffer;
    uint64_t dst_offset;
    uint64_t size;
};

struct VulkanBufferUploadDataParams {
    uint64_t offset;
    uint64_t size;
    const void* data;
};

bool vulkan_buffer_create(VulkanContext* context, VulkanBufferCreateParams params, VulkanBuffer* out_buffer);
void vulkan_buffer_destroy(VulkanContext* context, VulkanBuffer* buffer);
void vulkan_buffer_bind(VulkanContext* context, VulkanBuffer* buffer, uint64_t offset);

void* vulkan_buffer_map_memory(VulkanContext* context, VulkanBuffer* buffer, VulkanBufferMapMemoryParams params);
void vulkan_buffer_unmap_memory(VulkanContext* context, VulkanBuffer* buffer);

void vulkan_buffer_load_data(VulkanContext* context, VulkanBuffer* buffer, VulkanBufferLoadDataParams params);
void vulkan_buffer_copy(VulkanContext* context, VulkanBufferCopyParams params);
void vulkan_buffer_upload_data(VulkanContext* context, VulkanBuffer* buffer, VulkanBufferUploadDataParams params);
