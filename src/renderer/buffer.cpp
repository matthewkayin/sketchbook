#include "buffer.h"

#include "core/logger.h"
#include "renderer/util.h"
#include "renderer/command_buffer.h"

bool vulkan_buffer_create(VulkanContext* context, VulkanBufferCreateParams params, VulkanBuffer* out_buffer) {
    // Create the buffer
    VkBufferCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = params.size,
        .usage = params.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };
    VK_CHECK(vkCreateBuffer(context->device.logical_device, &create_info, context->allocator, &out_buffer->handle));

    // Gather memory requirements
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(context->device.logical_device, out_buffer->handle, &memory_requirements);
    uint32_t memory_type_index = vulkan_find_memory_index(
        context, memory_requirements.memoryTypeBits, params.memory_properties);
    if (memory_type_index == VULKAN_MEMORY_TYPE_INDEX_NOT_FOUND) {
        log_error("vulkan_buffer_create - The required memory type index was not found.");
        return false;
    }

    // Allocate memory for the buffer
    VkMemoryAllocateInfo allocate_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = memory_type_index
    };
    VkResult allocate_result = vkAllocateMemory(
        context->device.logical_device, &allocate_info, context->allocator, &out_buffer->memory);
    if (allocate_result != VK_SUCCESS) {
        log_error("vulkan_buffer_create - Memory allocation failed with error %s.", vulkan_result_str(allocate_result));
        return false;
    }

    if (params.bind_on_create) {
        vulkan_buffer_bind(context, out_buffer, 0);
    }

    return true;
}

void vulkan_buffer_destroy(VulkanContext* context, VulkanBuffer* buffer) {
    vkFreeMemory(context->device.logical_device, buffer->memory, context->allocator);
    vkDestroyBuffer(context->device.logical_device, buffer->handle, context->allocator);
}

void vulkan_buffer_bind(VulkanContext* context, VulkanBuffer* buffer, uint64_t offset) {
    VK_CHECK(vkBindBufferMemory(context->device.logical_device, buffer->handle, buffer->memory, offset));
}

void* vulkan_buffer_map_memory(VulkanContext* context, VulkanBuffer* buffer, VulkanBufferMapMemoryParams params) {
    void* buffer_data;
    VK_CHECK(vkMapMemory(
        context->device.logical_device, buffer->memory, params.offset, params.size, 0, &buffer_data));
    return buffer_data;
}

void vulkan_buffer_unmap_memory(VulkanContext* context, VulkanBuffer* buffer) {
    vkUnmapMemory(context->device.logical_device, buffer->memory);
}

void vulkan_buffer_load_data(VulkanContext* context, VulkanBuffer* buffer, VulkanBufferLoadDataParams params) {
    void* buffer_data = vulkan_buffer_map_memory(context, buffer, {
        .offset = params.offset,
        .size = params.size
    });
    memcpy(buffer_data, params.data, params.size);
    vulkan_buffer_unmap_memory(context, buffer);
}

void vulkan_buffer_copy(VulkanContext* context, VulkanBufferCopyParams params) {
    VkCommandBuffer temp_command_buffer;
    vulkan_command_buffer_begin_single_use(context, &temp_command_buffer);

    // Send copy command to the buffer
    VkBufferCopy copy_region {
        .srcOffset = params.src_offset,
        .dstOffset = params.dst_offset,
        .size = params.size
    };
    vkCmdCopyBuffer(temp_command_buffer, params.src_buffer, params.dst_buffer, 1, &copy_region);

    vulkan_command_buffer_end_single_use(context, &temp_command_buffer);
}

void vulkan_buffer_upload_data(VulkanContext* context, VulkanBuffer* buffer, VulkanBufferUploadDataParams params) {
    // Create staging buffer
    VulkanBuffer staging_buffer;
    SBK_ASSERT(vulkan_buffer_create(context, {
        .size = params.size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .bind_on_create = true
    }, &staging_buffer));

    // Load data into staging buffer
    vulkan_buffer_load_data(context, &staging_buffer, {
        .offset = 0,
        .size = params.size,
        .data = params.data
    });

    // Copy from the staging buffer to the device-local buffer
    vulkan_buffer_copy(context, {
        .src_buffer = staging_buffer.handle,
        .src_offset = 0,
        .dst_buffer = buffer->handle,
        .dst_offset = params.offset,
        .size = params.size
    });

    vulkan_buffer_destroy(context, &staging_buffer);
}
