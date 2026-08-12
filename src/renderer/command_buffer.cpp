#include "command_buffer.h"

void vulkan_command_buffer_begin_single_use(VulkanContext* context, VkCommandBuffer* out_command_buffer) {
    VK_CHECK(vkQueueWaitIdle(context->device.graphics_queue));

    // Alloc temp command buffer
    VkCommandBufferAllocateInfo temp_command_buffer_alloc_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = context->device.graphics_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VK_CHECK(vkAllocateCommandBuffers(
        context->device.logical_device, &temp_command_buffer_alloc_info, out_command_buffer));

    // Begin command buffer
    VkCommandBufferBeginInfo temp_command_buffer_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    VK_CHECK(vkBeginCommandBuffer(*out_command_buffer, &temp_command_buffer_begin_info));
}

void vulkan_command_buffer_end_single_use(VulkanContext* context, VkCommandBuffer* command_buffer) {
    // End command buffer
    VK_CHECK(vkEndCommandBuffer(*command_buffer));

    // Submit command buffer to queue
    VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = command_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    VK_CHECK(vkQueueSubmit(context->device.graphics_queue, 1, &submit_info, nullptr));
    VK_CHECK(vkQueueWaitIdle(context->device.graphics_queue));
}
