#include "descriptor.h"

#include "core/logger.h"
#include "renderer/uniform_types.h"
#include "renderer/image.h"
#include "renderer/buffer.h"

bool vulkan_uniform_create_hatch_textures(VulkanContext* context);
bool vulkan_uniform_create_fallback_texture(VulkanContext* context);

void vulkan_descriptor_sets_create(VulkanContext* context) {
    // GLOBAL DESCRIPTOR SET LAYOUT
    VkDescriptorSetLayoutBinding global_layout_bindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        }
    };
    VkDescriptorSetLayoutCreateInfo global_layout_create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = array_length(global_layout_bindings),
        .pBindings = global_layout_bindings
    };
    VK_CHECK(vkCreateDescriptorSetLayout(
        context->device.logical_device,
        &global_layout_create_info,
        context->allocator,
        &context->global_descriptor_set_layout
    ));

    // GRAPHICS DESCRIPTOR SET LAYOUT
    VkDescriptorSetLayoutBinding graphics_layout_bindings[] = {
        // Hatch 1
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        },
        // Hatch 2
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        },
        // Shadow Map
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        },
    };
    VkDescriptorSetLayoutCreateInfo graphics_layout_create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = array_length(graphics_layout_bindings),
        .pBindings = graphics_layout_bindings
    };
    VK_CHECK(vkCreateDescriptorSetLayout(
        context->device.logical_device,
        &graphics_layout_create_info,
        context->allocator,
        &context->graphics_descriptor_set_layout
    ));

    // MODEL DESCRIPTOR SET LAYOUT
    VkDescriptorSetLayoutBinding model_layout_bindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        }
    };
    VkDescriptorSetLayoutCreateInfo model_layout_create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = array_length(model_layout_bindings),
        .pBindings = model_layout_bindings
    };
    VK_CHECK(vkCreateDescriptorSetLayout(
        context->device.logical_device,
        &model_layout_create_info,
        context->allocator,
        &context->model_descriptor_set_layout
    ));

    // DESCRIPTOR POOL
    VkDescriptorPoolSize descriptor_pool_sizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = VULKAN_MAX_FRAMES_IN_FLIGHT
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = VULKAN_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_COUNT
        }
    };
    VkDescriptorPoolCreateInfo descriptor_pool_create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = VULKAN_DESCRIPTOR_POOL_MAX_SETS,
        .poolSizeCount = array_length(descriptor_pool_sizes),
        .pPoolSizes = descriptor_pool_sizes
    };
    VK_CHECK(vkCreateDescriptorPool(
        context->device.logical_device,
        &descriptor_pool_create_info,
        context->allocator,
        &context->descriptor_pool));

    // ALLOC GLOBAL SET
    VkDescriptorSetLayout global_alloc_set_layouts[] = {
        context->global_descriptor_set_layout,
        context->global_descriptor_set_layout
    };
    VkDescriptorSetAllocateInfo global_set_alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = context->descriptor_pool,
        .descriptorSetCount = array_length(context->global_descriptor_sets),
        .pSetLayouts = global_alloc_set_layouts
    };
    VK_CHECK(vkAllocateDescriptorSets(
        context->device.logical_device,
        &global_set_alloc_info,
        context->global_descriptor_sets));

    // ALLOC GRAPHICS SET
    VkDescriptorSetLayout graphics_alloc_set_layouts[] = {
        context->graphics_descriptor_set_layout,
        context->graphics_descriptor_set_layout
    };
    VkDescriptorSetAllocateInfo graphics_set_alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = context->descriptor_pool,
        .descriptorSetCount = array_length(context->graphics_descriptor_sets),
        .pSetLayouts = graphics_alloc_set_layouts
    };
    VK_CHECK(vkAllocateDescriptorSets(
        context->device.logical_device,
        &graphics_set_alloc_info,
        context->graphics_descriptor_sets));

    // DESCRIPTOR WRITES
    std::vector<VkWriteDescriptorSet> descriptor_writes;

    // UNIFORM BUFFER DESCRIPTORS
    VkDescriptorBufferInfo uniform_buffer_infos[VULKAN_MAX_FRAMES_IN_FLIGHT];
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        uniform_buffer_infos[index] = {
            .buffer = context->uniform_buffers[index].handle,
            .offset = 0,
            .range = sizeof(RendererUniformBufferObject)
        };

        descriptor_writes.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = context->global_descriptor_sets[index],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &uniform_buffer_infos[index],
            .pTexelBufferView = nullptr
        });
    }

    // HATCH TEXTURE DESCRIPTORS
    VkDescriptorImageInfo image_infos[VULKAN_MAX_FRAMES_IN_FLIGHT];
    for (uint32_t image_index = 0; image_index < VULKAN_HATCH_TEXTURE_IMAGE_COUNT; image_index++) {
        image_infos[image_index] = {
            .sampler = context->texture_sampler,
            .imageView = context->hatch_textures[image_index].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
            descriptor_writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = context->graphics_descriptor_sets[index],
                .dstBinding = 0 + image_index,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_infos[image_index],
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            });
        }
    }

    // SHADOW MAP SAMPLER DESCRIPTORS
    VkDescriptorImageInfo shadow_map_image_infos[VULKAN_MAX_FRAMES_IN_FLIGHT];
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        shadow_map_image_infos[index] = {
            .sampler = context->texture_sampler,
            .imageView = context->shadow_maps[index].view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };
        descriptor_writes.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = context->graphics_descriptor_sets[index],
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &shadow_map_image_infos[index],
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        });
    }


    vkUpdateDescriptorSets(
        context->device.logical_device,
        descriptor_writes.size(), descriptor_writes.data(),
        0, nullptr);
}

void vulkan_descriptor_sets_destroy(VulkanContext* context) {
    vkDestroyDescriptorPool(context->device.logical_device, context->descriptor_pool, context->allocator);
    vkDestroyDescriptorSetLayout(context->device.logical_device, context->global_descriptor_set_layout, context->allocator);
    vkDestroyDescriptorSetLayout(context->device.logical_device, context->graphics_descriptor_set_layout, context->allocator);
    vkDestroyDescriptorSetLayout(context->device.logical_device, context->model_descriptor_set_layout, context->allocator);
}

bool vulkan_uniform_objects_create(VulkanContext* context) {
    // Create uniform buffers
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        vulkan_buffer_create(context, {
            .size = VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(RendererUniformBufferObject),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .bind_on_create = true
        }, &context->uniform_buffers[index]);
    }

    // Create texture sampler
    VkSamplerCreateInfo sampler_create_info {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = context->device.properties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    VK_CHECK(vkCreateSampler(
        context->device.logical_device,
        &sampler_create_info,
        context->allocator,
        &context->texture_sampler));

    // Create depth sampler
    VkSamplerCreateInfo depth_sampler_create_info {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 1.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 1.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    };
    VK_CHECK(vkCreateSampler(
        context->device.logical_device,
        &depth_sampler_create_info,
        context->allocator,
        &context->depth_sampler));

    if (!vulkan_uniform_create_hatch_textures(context)) {
        return false;
    }
    if (!vulkan_uniform_create_fallback_texture(context)) {
        return false;
    }

    // Create shadow maps
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        bool success = vulkan_image_create(context, {
            .width = VULKAN_SHADOW_MAP_WIDTH,
            .height = VULKAN_SHADOW_MAP_HEIGHT,
            .mip_levels = 1,
            .format = context->device.depth_format,
            .msaa_sample_count = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        }, &context->shadow_maps[index]);
        if (!success) {
            return false;
        }
    }

    return true;
}

bool vulkan_uniform_create_hatch_textures(VulkanContext* context) {
    bool success;

    const char* hatch_texture_paths[VULKAN_HATCH_TEXTURE_CHANNEL_COUNT] = {
        "../res/texture/hatch0.jpg",
        "../res/texture/hatch1.jpg",
        "../res/texture/hatch2.jpg",
        "../res/texture/hatch3.jpg",
        "../res/texture/hatch4.jpg",
        "../res/texture/hatch5.jpg"
    };

    // Init surfaces to nullptr
    SDL_Surface* hatch_surfaces[VULKAN_HATCH_TEXTURE_CHANNEL_COUNT];
    for (uint32_t index = 0; index < VULKAN_HATCH_TEXTURE_CHANNEL_COUNT; index++) {
        hatch_surfaces[index] = nullptr;
    }

    // Load each surface
    for (uint32_t index = 0; index < VULKAN_HATCH_TEXTURE_CHANNEL_COUNT; index++) {
        SDL_IOStream* image_stream = SDL_IOFromFile(hatch_texture_paths[index], "rb");
        if (!image_stream) {
            log_error("Failed to load hatch image at path %s: %s.", hatch_texture_paths[index], SDL_GetError());
            success = false;
            goto end;
        }

        hatch_surfaces[index] = vulkan_image_load_surface(image_stream, VULKAN_LOAD_SURFACE_FLIP_V);
        if (!hatch_surfaces[index]) {
            success = false;
            goto end;
        }
    }

    success = vulkan_image_create_hatch_textures(context, hatch_surfaces, VULKAN_IMAGE_MIPMAP_SCALED, context->hatch_textures);

end:
    for (uint32_t index = 0; index < VULKAN_HATCH_TEXTURE_CHANNEL_COUNT; index++) {
        if (hatch_surfaces[index]) {
            SDL_DestroySurface(hatch_surfaces[index]);
        }
    }

    return success;
}

bool vulkan_uniform_create_fallback_texture(VulkanContext* context) {
    SDL_Surface* surface = SDL_CreateSurface(2, 2, SDL_PIXELFORMAT_ABGR8888);
    if (!surface) {
        log_error("Error creating surface for fallback texture: %s", SDL_GetError());
        return false;
    }

    SDL_UnlockSurface(surface);
    uint32_t* surface_pixels = (uint32_t*)surface->pixels;
    for (int y = 0; y < surface->h; y++) {
        for (int x = 0; x < surface->w; x++) {
            surface_pixels[x + (y * surface->w)] = 0xFFFFFFFF;
        }
    }
    SDL_LockSurface(surface);

    bool success = vulkan_image_create_textures(context, {
        .mipmap_type = VULKAN_IMAGE_MIPMAP_SCALED,
        .surface_count = 1,
        .surfaces = &surface
    }, &context->fallback_texture);

    SDL_DestroySurface(surface);

    return success;
}

void vulkan_uniform_objects_destroy(VulkanContext* context) {
    // Destroy hatch textures
    for (uint32_t index = 0; index < VULKAN_HATCH_TEXTURE_IMAGE_COUNT; index++) {
        vulkan_image_destroy(context, &context->hatch_textures[index]);
    }

    // Destroy samplers
    vkDestroySampler(context->device.logical_device, context->texture_sampler, context->allocator);
    vkDestroySampler(context->device.logical_device, context->depth_sampler, context->allocator);

    // Destroy uniform buffers
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        vulkan_buffer_destroy(context, &context->uniform_buffers[index]);
    }

    // Destroy fallback texture
    vulkan_image_destroy(context, &context->fallback_texture);

    // Destroy shadow maps
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        vulkan_image_destroy(context, &context->shadow_maps[index]);
    }
}
