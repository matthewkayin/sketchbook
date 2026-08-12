#include "image.h"

#include "core/logger.h"
#include "renderer/buffer.h"
#include "renderer/command_buffer.h"
#include "renderer/util.h"
#include "vulkan/vulkan_core.h"
#include <SDL3/SDL.h>
#include <cmath>

bool vulkan_image_create(VulkanContext* context, VulkanImageCreateParams params, VulkanImage* out_image) {
    out_image->format = params.format;
    out_image->width = params.width;
    out_image->height = params.height;
    out_image->mip_levels = params.mip_levels;

    // Create image
    VkImageCreateInfo image_create_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = params.format,
        .extent = {
            .width = params.width,
            .height = params.height,
            .depth = 1
        },
        .mipLevels = params.mip_levels,
        .arrayLayers = 1,
        .samples = params.msaa_sample_count,
        .tiling = params.tiling,
        .usage = params.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    VkResult create_result = vkCreateImage(
        context->device.logical_device, &image_create_info, context->allocator, &out_image->handle);
    if (create_result != VK_SUCCESS) {
        log_error("vulkan_image_create - Image creation failed with error %s.", vulkan_result_str(create_result));
        return false;
    }

    // Get image memory
    VkMemoryRequirements memory_reqeuirements;
    vkGetImageMemoryRequirements(context->device.logical_device, out_image->handle, &memory_reqeuirements);
    uint32_t memory_type_index = vulkan_find_memory_index(
        context, memory_reqeuirements.memoryTypeBits, params.memory_properties);
    if (memory_type_index == VULKAN_MEMORY_TYPE_INDEX_NOT_FOUND) {
        log_error("vulkan_image_create - The required memory type index was not found.");
        return false;
    }

    // Allocate image memory
    VkMemoryAllocateInfo allocate_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_reqeuirements.size,
        .memoryTypeIndex = memory_type_index
    };
    VkResult allocate_result = vkAllocateMemory(
        context->device.logical_device, &allocate_info, context->allocator, &out_image->memory);
    if (allocate_result != VK_SUCCESS) {
        log_error("vulkan_image_create - Memory allocation failed with error %s.", vulkan_result_str(allocate_result));
        return false;
    }

    // Bind image memory
    VK_CHECK(vkBindImageMemory(context->device.logical_device, out_image->handle, out_image->memory, 0));

    // Create image view
    vulkan_image_view_create(context, {
        .image = out_image->handle,
        .format = params.format,
        .aspect = params.aspect,
        .mip_levels = params.mip_levels
    }, &out_image->view);

    return true;
}

bool vulkan_image_create_texture(VulkanContext* context, const char* path, VulkanImage* out_image) {
    // Load texture surface
    SDL_Surface* image_surface = SDL_LoadPNG(path);
    if (!image_surface) {
        log_error("Failed to load image at path %s.", path);
        return false;
    }
    if (image_surface->format != SDL_PIXELFORMAT_ABGR8888) {
        SDL_Surface* old_surface = image_surface;
        image_surface = SDL_ConvertSurface(old_surface, SDL_PIXELFORMAT_ABGR8888);
        SDL_DestroySurface(old_surface);
    }
    if (!image_surface) {
        log_error("Failed to convert image %s. %s", path, SDL_GetError());
        return false;
    }
    if (!SDL_FlipSurface(image_surface, SDL_FLIP_VERTICAL)) {
        log_error("Failed to flip image %s vertically: %s", path, SDL_GetError());
        return false;
    }
    const size_t image_size = image_surface->pitch * image_surface->h;

    // Copy image data to staging buffer
    VulkanBuffer staging_buffer;
    vulkan_buffer_create(context, {
        .size = image_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    }, &staging_buffer);
    vulkan_buffer_bind(context, &staging_buffer, 0);
    void* staging_buffer_data = vulkan_buffer_map_memory(context, &staging_buffer, {
        .offset = 0,
        .size = VK_WHOLE_SIZE
    });
    memcpy(staging_buffer_data, image_surface->pixels, image_size);
    vulkan_buffer_unmap_memory(context, &staging_buffer);

    // Create image
    bool image_create_succeeded = vulkan_image_create(context, {
        .width = (uint32_t)image_surface->w,
        .height = (uint32_t)image_surface->h,
        .mip_levels = (uint32_t)std::floor(std::log2(std::max(image_surface->w, image_surface->h))) + 1U,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .msaa_sample_count = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    }, out_image);
    if (!image_create_succeeded) {
        return false;
    }

    // Copy image data to the VulkanImage
    VkCommandBuffer temp_command_buffer;
    vulkan_command_buffer_begin_single_use(context, &temp_command_buffer);

    // Transition layout to TRANSFER_DST
    vulkan_image_transition_layout({
        .command_buffer = temp_command_buffer,
        .image = out_image,
        .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
        .new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    });

    // Copy from buffer into image
    VkBufferImageCopy copy_region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = { .x = 0, .y = 0, .z = 0 },
        .imageExtent = {
            .width = (uint32_t)image_surface->w,
            .height = (uint32_t)image_surface->h,
            .depth = 1
        }
    };

    vkCmdCopyBufferToImage(
        temp_command_buffer,
        staging_buffer.handle,
        out_image->handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy_region);

    if (!vulkan_image_generate_mipmaps(context, temp_command_buffer, out_image)) {
        return false;
    }

    vulkan_command_buffer_end_single_use(context, &temp_command_buffer);
    vulkan_buffer_destroy(context, &staging_buffer);
    SDL_DestroySurface(image_surface);

    return true;
}

bool vulkan_image_generate_mipmaps(VulkanContext* context, VkCommandBuffer command_buffer, VulkanImage* image) {
    // Check if the image format supports linear blitting
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(context->device.physical_device, image->format, &format_properties);
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        log_error("Failed to generate mipmaps because image with format %u does not support linear blitting.");
        return false;
    }

    int mip_width = (int)image->width;
    int mip_height = (int)image->height;
    for (uint32_t level = 1; level < image->mip_levels; level++) {
        // Transition previous mip from TRANSFER_DST_OPTIMAL to TRANSFER_SRC_OPTIMAL (to be read from)
        VkImageMemoryBarrier2 mip_src_barrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image->handle,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = level - 1,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        VkDependencyInfo dependency_info {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &mip_src_barrier
        };
        vkCmdPipelineBarrier2(command_buffer, &dependency_info);

        // Blit previous mip onto current mip
        VkImageBlit2 image_blit {
            .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
            .pNext = nullptr,
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = level - 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets = {
                { .x = 0, .y = 0, .z = 0 },
                {
                    .x = mip_width,
                    .y = mip_height,
                    .z = 1
                },
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = level,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .dstOffsets = {
                { .x = 0, .y = 0, .z = 0 },
                {
                    .x = 1 < mip_width ? mip_width / 2 : 1,
                    .y = 1 < mip_height ? mip_height / 2 : 1,
                    .z = 1
                }
            },
        };
        VkBlitImageInfo2 blit_info {
            .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
            .pNext = nullptr,
            .srcImage = image->handle,
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = image->handle,
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = 1,
            .pRegions = &image_blit,
            .filter = VK_FILTER_LINEAR
        };
        vkCmdBlitImage2(command_buffer, &blit_info);

        // Reduce mip size
        if (1 < mip_width) {
            mip_width /= 2;
        }
        if (1 < mip_height) {
            mip_height /= 2;
        }
    }

    // Transition mips to SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier2 mip_read_only_barriers[] = {
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image->handle,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = image->mip_levels - 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        },
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image->handle,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = image->mip_levels - 1,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        }
    };
    VkDependencyInfo dependency_info {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = array_length(mip_read_only_barriers),
        .pImageMemoryBarriers = mip_read_only_barriers
    };
    vkCmdPipelineBarrier2(command_buffer, &dependency_info);

    return true;
}

void vulkan_image_destroy(VulkanContext* context, VulkanImage* image) {
    vkDestroyImageView(context->device.logical_device, image->view, context->allocator);
    vkFreeMemory(context->device.logical_device, image->memory, context->allocator);
    vkDestroyImage(context->device.logical_device, image->handle, context->allocator);
}

void vulkan_image_view_create(VulkanContext* context, VulkanImageViewCreateParams params, VkImageView* out_image_view) {
    VkImageViewCreateInfo view_create_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = params.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = params.format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = params.aspect,
            .baseMipLevel = 0,
            .levelCount = params.mip_levels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    VK_CHECK(vkCreateImageView(
        context->device.logical_device, &view_create_info, context->allocator, out_image_view));
}

void vulkan_image_view_destroy(VulkanContext* context, VkImageView view) {
    vkDestroyImageView(context->device.logical_device, view, context->allocator);
}

void vulkan_image_transition_layout(VulkanImageTransitionLayoutParams params) {
    VkAccessFlags2 src_access_mask;
    VkAccessFlags2 dst_access_mask;
    VkPipelineStageFlags2 src_stage_mask;
    VkPipelineStageFlags2 dst_stage_mask;

    if (params.old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        params.new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    ) {
        src_access_mask = 0;
        dst_access_mask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        src_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    } else if (params.old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        params.new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ) {
        src_access_mask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        dst_access_mask = VK_ACCESS_2_SHADER_READ_BIT;
        src_stage_mask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    } else {
        SBK_ASSERT_MESSAGE(false, "Unsupported layout transition.");
        return;
    }

    vulkan_image_transition_layout_ext({
        .command_buffer = params.command_buffer,
        .image = params.image->handle,
        .image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .base_mip_level = 0,
        .mip_levels = params.image->mip_levels,
        .old_layout = params.old_layout,
        .new_layout = params.new_layout,
        .src_access_mask = src_access_mask,
        .dst_access_mask = dst_access_mask,
        .src_stage_mask = src_stage_mask,
        .dst_stage_mask = dst_stage_mask
    });
}

void vulkan_image_transition_layout_ext(VulkanImageTransitionLayoutExtParams params) {
    VkImageMemoryBarrier2 image_memory_barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = params.src_stage_mask,
        .srcAccessMask = params.src_access_mask,
        .dstStageMask = params.dst_stage_mask,
        .dstAccessMask = params.dst_access_mask,
        .oldLayout = params.old_layout,
        .newLayout = params.new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = params.image,
        .subresourceRange = {
            .aspectMask = params.image_aspect,
            .baseMipLevel = params.base_mip_level,
            .levelCount = params.mip_levels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    VkDependencyInfo dependency_info {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_memory_barrier
    };
    vkCmdPipelineBarrier2(params.command_buffer, &dependency_info);
}
