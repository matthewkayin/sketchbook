#include "image.h"

#include "core/logger.h"
#include "renderer/buffer.h"
#include "renderer/command_buffer.h"
#include "renderer/util.h"
#include "vulkan/vulkan_core.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include <cmath>

// SURFACE

SDL_Surface* vulkan_image_load_surface(SDL_IOStream* image_stream, uint32_t options) {
    // Load texture surface
    bool success = true;
    SDL_Surface* image_surface = IMG_Load_IO(image_stream, true);
    if (!image_surface) {
        log_error("Failed to load image: %s", SDL_GetError());
        success = false;
        goto end;
    }

    // Convert surface
    if (image_surface->format != SDL_PIXELFORMAT_ABGR8888) {
        SDL_Surface* old_surface = image_surface;
        image_surface = SDL_ConvertSurface(old_surface, SDL_PIXELFORMAT_ABGR8888);
        SDL_DestroySurface(old_surface);
    }
    if (!image_surface) {
        log_error("Failed to convert image: %s", SDL_GetError());
        success = false;
        goto end;
    }

    // Flip surface vertically
    if (options & VULKAN_LOAD_SURFACE_FLIP_V) {
        if (!SDL_FlipSurface(image_surface, SDL_FLIP_VERTICAL)) {
            log_error("Failed to flip image vertically: %s", SDL_GetError());
            success = false;
            goto end;
        }
    }
end:
    if (!success && image_surface != nullptr) {
        SDL_DestroySurface(image_surface);
        image_surface = nullptr;
    }

    return image_surface;
}

size_t vulkan_image_surface_size(const SDL_Surface* surface) {
    return surface->pitch * surface->h;
}

// CREATE

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

void vulkan_image_destroy(VulkanContext* context, VulkanImage* image) {
    vkDestroyImageView(context->device.logical_device, image->view, context->allocator);
    vkFreeMemory(context->device.logical_device, image->memory, context->allocator);
    vkDestroyImage(context->device.logical_device, image->handle, context->allocator);
    image->handle = VK_NULL_HANDLE;
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

// TEXTURES

bool vulkan_image_create_textures(VulkanContext* context, VulkanImageCreateTextureParams params, VulkanImage* out_images) {
    bool success = true;
    size_t image_copy_buffer_offset;
    VulkanBuffer staging_buffer;
    VkCommandBuffer temp_command_buffer;

    // Set handles to null for cleanup
    staging_buffer.handle = VK_NULL_HANDLE;
    temp_command_buffer = VK_NULL_HANDLE;
    for (uint32_t index = 0; index < params.surface_count; index++) {
        out_images[index].handle = VK_NULL_HANDLE;
    }

    // Sum up byte size of all images
    size_t size_of_all_images = 0;
    for (uint32_t index = 0; index < params.surface_count; index++) {
        size_of_all_images += vulkan_image_surface_size(params.surfaces[index]);
    }

    // Create staging buffer
    vulkan_buffer_create(context, {
        .size = size_of_all_images,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bind_on_create = true
    }, &staging_buffer);

    // Map staging buffer
    uint8_t* staging_buffer_data = (uint8_t*)vulkan_buffer_map_memory(context, &staging_buffer, {
        .offset = 0,
        .size = VK_WHOLE_SIZE
    });

    // Copy each image into staging buffer
    for (uint32_t index = 0; index < params.surface_count; index++) {
        const SDL_Surface* image_surface = params.surfaces[index];
        const size_t image_size = vulkan_image_surface_size(image_surface);
        memcpy(staging_buffer_data, image_surface->pixels, image_size);
        staging_buffer_data += image_size;
    }

    // Unmap staging buffer
    vulkan_buffer_unmap_memory(context, &staging_buffer);

    // Create images
    for (uint32_t index = 0; index < params.surface_count; index++) {
        const SDL_Surface* image_surface = params.surfaces[index];

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
        }, &out_images[index]);

        if (!image_create_succeeded) {
            success = false;
            goto end;
        }
    }

    // Setup command buffer for copying to image and creating mipmaps
    vulkan_command_buffer_begin_single_use(context, &temp_command_buffer);

    // Copy image datas to the out_images
    image_copy_buffer_offset = 0;
    for (uint32_t index = 0; index < params.surface_count; index++) {
        const SDL_Surface* image_surface = params.surfaces[index];

        // Transition layout to TRANSFER_DST
        vulkan_image_transition_layout({
            .command_buffer = temp_command_buffer,
            .image = &out_images[index],
            .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        });

        // Copy from buffer into image
        VkBufferImageCopy copy_region {
            .bufferOffset = image_copy_buffer_offset,
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
        image_copy_buffer_offset += vulkan_image_surface_size(image_surface);

        vkCmdCopyBufferToImage(
            temp_command_buffer,
            staging_buffer.handle,
            out_images[index].handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copy_region);

        if (!vulkan_image_generate_mipmaps(context, temp_command_buffer, params.mipmap_type, &out_images[index])) {
            success = false;
            goto end;
        }
    }

end:
    // CLEANUP

    // End command buffer
    if (temp_command_buffer != VK_NULL_HANDLE) {
        vulkan_command_buffer_end_single_use(context, &temp_command_buffer);
    }

    // Destroy staging buffer
    if (staging_buffer.handle != VK_NULL_HANDLE) {
        vulkan_buffer_destroy(context, &staging_buffer);
    }

    // On failure, destroy any non-null images
    if (!success) {
        for (uint32_t index = 0; index < params.surface_count; index++) {
            if (out_images[index].handle != VK_NULL_HANDLE) {
                vulkan_image_destroy(context, &out_images[index]);
            }
        }
    }

    return success;
}

bool vulkan_image_generate_mipmaps(VulkanContext* context, VkCommandBuffer command_buffer, VulkanImageMipmapType mipmap_type, VulkanImage* image) {
    // Check if the image format supports linear blitting
    if (mipmap_type == VULKAN_IMAGE_MIPMAP_SCALED) {
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(context->device.physical_device, image->format, &format_properties);
        if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            log_error("Failed to generate mipmaps because image with format %u does not support linear blitting.");
            return false;
        }
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

        // Determine half mip size
        int half_mip_width = 1 < mip_width ? mip_width / 2 : 1;
        int half_mip_height = 1 < mip_height ? mip_height / 2 : 1;

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
                    .x = mipmap_type == VULKAN_IMAGE_MIPMAP_SCALED ? mip_width : half_mip_width,
                    .y = mipmap_type == VULKAN_IMAGE_MIPMAP_SCALED ? mip_height : half_mip_height,
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
                    .x = half_mip_width,
                    .y = half_mip_height,
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
            .filter = mipmap_type == VULKAN_IMAGE_MIPMAP_SCALED
                ? VK_FILTER_LINEAR
                : VK_FILTER_NEAREST
        };
        vkCmdBlitImage2(command_buffer, &blit_info);

        // Reduce mip size
        mip_width = half_mip_width;
        mip_height = half_mip_height;
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

bool vulkan_image_create_hatch_textures(VulkanContext* context, SDL_Surface* hatch_surfaces[VULKAN_HATCH_TEXTURE_CHANNEL_COUNT], VulkanImage* out_images) {
    bool success;

    SDL_Surface* packed_hatch_surfaces[VULKAN_HATCH_TEXTURE_IMAGE_COUNT];
    for (uint32_t index = 0; index < VULKAN_HATCH_TEXTURE_IMAGE_COUNT; index++) {
        packed_hatch_surfaces[index] = nullptr;
    }

    // Pack the hatch surfaces into two
    for (uint32_t index = 0; index < VULKAN_HATCH_TEXTURE_IMAGE_COUNT; index++) {
        packed_hatch_surfaces[index] = SDL_CreateSurface(hatch_surfaces[0]->w, hatch_surfaces[0]->h, hatch_surfaces[0]->format);
        if (!packed_hatch_surfaces[index]) {
            log_error("Failed to create packed hatch surface: %s.", SDL_GetError());
            success = false;
            goto end;
        }

        const SDL_PixelFormatDetails* format_details = SDL_GetPixelFormatDetails(packed_hatch_surfaces[index]->format);
        SDL_LockSurface(packed_hatch_surfaces[index]);

        uint32_t channel_base_index = index * VULKAN_HATCH_CHANNELS_PER_IMAGE;
        uint32_t* packed_pixels = (uint32_t*)packed_hatch_surfaces[index]->pixels;

        for (int y = 0; y < packed_hatch_surfaces[index]->h; y++) {
            for (int x = 0; x < packed_hatch_surfaces[index]->w; x++) {
                // Since each image is grayscale, extract the red value to use as the "blackness" of each stroke
                uint8_t channel_colors[VULKAN_HATCH_CHANNELS_PER_IMAGE];
                for (uint32_t channel_index = 0; channel_index < VULKAN_HATCH_CHANNELS_PER_IMAGE; channel_index++) {
                    uint32_t* channel_pixels = (uint32_t*)hatch_surfaces[channel_base_index + channel_index]->pixels;
                    uint32_t channel_pixel = channel_pixels[x + (y * packed_hatch_surfaces[index]->w)];
                    SDL_GetRGBA(channel_pixel, format_details, nullptr, &channel_colors[channel_index], nullptr, nullptr, nullptr);
                }

                packed_pixels[x + (y * packed_hatch_surfaces[index]->w)] = SDL_MapRGBA(format_details, NULL, channel_colors[0], channel_colors[1], channel_colors[2], 255);
            }
        }

        SDL_UnlockSurface(packed_hatch_surfaces[index]);
    }

    // Create a Vulkan image for each packed surface
    success = vulkan_image_create_textures(context, {
        .mipmap_type = VULKAN_IMAGE_MIPMAP_SUBSET,
        .surface_count = VULKAN_HATCH_TEXTURE_IMAGE_COUNT,
        .surfaces = packed_hatch_surfaces
    }, out_images);

end:
    // CLEANUP

    // Destroy packed hatch surfaces
    for (uint32_t index = 0; index < VULKAN_HATCH_TEXTURE_IMAGE_COUNT; index++) {
        if (packed_hatch_surfaces[index]) {
            SDL_DestroySurface(packed_hatch_surfaces[index]);
        }
    }

    return success;
}

// LAYOUT

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

// INTERNAL
