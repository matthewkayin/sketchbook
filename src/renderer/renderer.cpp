#include "renderer.h"

#include "defines.h"
#include "core/logger.h"
#include "core/math.h"
#include "renderer/types.h"
#include "renderer/util.h"
#include "renderer/device.h"
#include "renderer/swapchain.h"
#include "renderer/pipeline.h"
#include "renderer/image.h"
#include "renderer/buffer.h"
#include "renderer/model.h"
#include "renderer/descriptor.h"
#include <SDL3/SDL_vulkan.h>
#include <vector>

// Debug

bool renderer_get_debug_extension_names(
    std::vector<const char*>& extension_names,
    std::vector<const char*>& layer_names);
VKAPI_ATTR VkBool32 VKAPI_CALL renderer_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT /*message_types*/,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* /*user_data*/);
void renderer_create_debugger();

// Internal
void renderer_create_sync_objects();
void renderer_destroy_sync_objects();
void renderer_recreate_swapchain();

// Context
static VulkanContext context;

bool renderer_init(SDL_Window* window) {
    context.window = window;
    context.allocator = nullptr;

    VkApplicationInfo app_info {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = SBK_APP_NAME,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Sketchbook Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };

    // Extension and layer names
    std::vector<const char*> extension_names;
    std::vector<const char*> layer_names;

    // Get platform extensions from SDL
    uint32_t instance_extension_count;
    const char* const* instance_extensions = SDL_Vulkan_GetInstanceExtensions(&instance_extension_count);
    if (instance_extensions == nullptr) {
        log_error("Failed to get platform-specific Vulkan extensions: %s", SDL_GetError());
        return false;
    }
    for (uint32_t extension_index = 0; extension_index < instance_extension_count; extension_index++) {
        extension_names.push_back(instance_extensions[extension_index]);
    }

    // Get debug extensions
    if (!renderer_get_debug_extension_names(extension_names, layer_names)) {
        return false;
    }

    VkInstanceCreateInfo instance_create_info {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = (uint32_t)layer_names.size(),
        .ppEnabledLayerNames = layer_names.data(),
        .enabledExtensionCount = (uint32_t)extension_names.size(),
        .ppEnabledExtensionNames = extension_names.data()
    };

    // Create instance
    VkResult result = vkCreateInstance(&instance_create_info, context.allocator, &context.instance);
    if (result != VK_SUCCESS) {
        log_error("vkCreateInstance failed with result %s.", vulkan_result_str(result));
        return false;
    }

    // Create debugger
    renderer_create_debugger();

    // Create surface
    if (!SDL_Vulkan_CreateSurface(context.window, context.instance, context.allocator, &context.surface)) {
        log_error("Failed to create surface %s.", SDL_GetError());
        return false;
    }

    // Create vulkan objects
    if (!vulkan_device_create(&context)) {
        return false;
    }
    vulkan_swapchain_create(&context);

    // Create uniform objects
    if (!vulkan_uniform_objects_create(&context)) {
        return false;
    }

    // Create descriptors
    vulkan_descriptor_sets_create(&context);

    // Create pipelines
    if (!vulkan_pipeline_create_graphics(&context, &context.graphics_pipeline)) {
        return false;
    }
    if (!vulkan_pipeline_create_outline(&context, &context.outline_pipeline)) {
        return false;
    }
    if (!vulkan_pipeline_create_shadow(&context, &context.shadow_pipeline)) {
        return false;
    }

    // Create graphics command buffer
    VkCommandBufferAllocateInfo graphics_command_buffer_allocate_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = context.device.graphics_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = array_length(context.graphics_command_buffers)
    };
    VK_CHECK(vkAllocateCommandBuffers(
        context.device.logical_device,
        &graphics_command_buffer_allocate_info,
        context.graphics_command_buffers));

    renderer_create_sync_objects();

    // Create floor
    Vertex3d floor_vertices[] = {
        { .position = vec3(25.0f, 0.0f, 25.0f), .normal = vec3::up(), .tex_coord = vec2(1.0f, 0.0f) },
        { .position = vec3(-25.0f, 0.0f, 25.0f), .normal = vec3::up(), .tex_coord = vec2(0.0f, 0.0f) },
        { .position = vec3(-25.0f, 0.0f, -25.0f), .normal = vec3::up(), .tex_coord = vec2(0.0f, 1.0f) },

        { .position = vec3(25.0f, 0.0f, 25.0f), .normal = vec3::up(), .tex_coord = vec2(1.0f, 0.0f) },
        { .position = vec3(-25.0f, 0.0f, -25.0f), .normal = vec3::up(), .tex_coord = vec2(0.0f, 1.0f) },
        { .position = vec3(25.0f, 0.0f, -25.0f), .normal = vec3::up(), .tex_coord = vec2(1.0f, 1.0f) },
    };
    uint32_t floor_indices[] = {
        0, 2, 1,
        3, 5, 4,
    };
    if (!vulkan_model_create_geometry(&context, {
        .vertex_count = array_length(floor_vertices),
        .vertices = floor_vertices,
        .index_count = array_length(floor_indices),
        .indices = floor_indices
    }, &context.model_floor)) {
        return false;
    }

    context.frame_index = 0;

    log_info("Renderer initialized.");
    return true;
}

void renderer_quit() {
    vkDeviceWaitIdle(context.device.logical_device);

    for (uint32_t index = 0; index < context.model_data.size(); index++) {
        vulkan_model_destroy(&context, &context.model_data[index]);
    }
    vulkan_model_destroy(&context, &context.model_floor);

    vulkan_descriptor_sets_destroy(&context);
    vulkan_uniform_objects_destroy(&context);
    renderer_destroy_sync_objects();
    vkFreeCommandBuffers(
        context.device.logical_device,
        context.device.graphics_command_pool,
        array_length(context.graphics_command_buffers),
        context.graphics_command_buffers);
    vulkan_pipeline_destroy(&context, &context.graphics_pipeline);
    vulkan_pipeline_destroy(&context, &context.outline_pipeline);
    vulkan_pipeline_destroy(&context, &context.shadow_pipeline);
    vulkan_swapchain_destroy(&context);
    vulkan_device_destroy(&context);

    log_debug("Destroying vulkan surface...");
    SDL_Vulkan_DestroySurface(context.instance, context.surface, context.allocator);

    // Destroy debug messenger
    log_debug("Destroying debug messenger...");
    if (context.debug_messenger != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        destroyDebugUtilsMessenger(context.instance, context.debug_messenger, context.allocator);
    }

    log_debug("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);

    log_info("Successfully quit renderer.");
}

void renderer_on_resized() {
    renderer_recreate_swapchain();
}

void renderer_draw_scene(const RenderPacket& packet, bool use_material) {
    vulkan_model_render(&context, context.model_floor, mat4::identity(), use_material);
    vulkan_model_render(&context, context.model_data[packet.model_index], packet.model_transform, false);
}

void renderer_draw_frame(RenderPacket packet) {
    // Wait for current frame fence
    VkResult fence_result = vkWaitForFences(
        context.device.logical_device,
        1, &context.frame_fences[context.frame_index], VK_TRUE, UINT64_MAX);
    if (fence_result != VK_SUCCESS) {
        log_error("Error waiting for fence: %s.", vulkan_result_str(fence_result));
        return;
    }

    // Acquire next image
    VkResult acquire_result = vkAcquireNextImageKHR(
        context.device.logical_device,
        context.swapchain.handle,
        UINT64_MAX,
        context.acquire_semaphores[context.frame_index],
        VK_NULL_HANDLE,
        &context.image_index);
    if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
        log_info("vkAcquireNextImageKHR - Swapchain is out of date. Recreating swapchain...");
        renderer_recreate_swapchain();
        return;
    } else if (acquire_result != VK_SUCCESS) {
        log_error("Error acquiring next image: %s.", vulkan_result_str(acquire_result));
        return;
    }

    // Reset current frame fence (only done after we have successfully acquired image to avoid deadlock)
    vkResetFences(context.device.logical_device, 1, &context.frame_fences[context.frame_index]);

    // Begin command buffer
    vkResetCommandBuffer(context.graphics_command_buffers[context.frame_index], 0);
    VkCommandBufferBeginInfo command_buffer_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };
    vkBeginCommandBuffer(context.graphics_command_buffers[context.frame_index], &command_buffer_begin_info);

    // Bind global descriptor set
    vkCmdBindDescriptorSets(
        context.graphics_command_buffers[context.frame_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, context.graphics_pipeline.layout,
        0, 1, &context.global_descriptor_sets[context.frame_index], 0, nullptr);

    // SET UBO

    RendererUniformBufferObject ubo {
        .view = packet.view,
        .projection = mat4::perspective(45.0f * SBK_DEG_TO_RAD, (float)context.swapchain.extent.width / (float)context.swapchain.extent.height, 0.1f, 1000.0f),
        .depth_view_projection =
            mat4::ortho(-10.0f, 10.0f, 10.0f, -10.0f, 1.0f, 7.5f) *
            mat4::look_at(packet.light_position, vec3(0.0f, 0.0f, 0.0f), vec3::up()),
        .view_position = vec4(packet.view_position, 0.0f),
        .light_position = vec4(packet.light_position, 0.0f)
    };

    vulkan_buffer_load_data(&context, &context.uniform_buffers[context.frame_index], {
        .offset = 0,
        .size = sizeof(ubo),
        .data = &ubo
    });

    // SHADOW PASS
    {
        // Transition shadow map to DEPTH_ATTACHMENT_OPTIMAL
        vulkan_image_transition_layout_ext({
            .command_buffer = context.graphics_command_buffers[context.frame_index],
            .image = context.shadow_maps[context.frame_index].handle,
            .image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
            .base_mip_level = 0,
            .mip_levels = 1,
            .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .new_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .src_access_mask = VK_ACCESS_2_NONE,
            .dst_access_mask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .src_stage_mask = VK_PIPELINE_STAGE_2_NONE,
            .dst_stage_mask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        });

        // Rendering info
        VkRenderingAttachmentInfo shadow_pass_depth_attachment {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = context.shadow_maps[context.frame_index].view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .depthStencil = {
                    .depth = 1.0f,
                    .stencil = 0
                }
            }
        };
        VkRenderingInfo shadow_pass_rendering_info {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = {
                .offset = { .x = 0, .y = 0 },
                .extent = {
                    .width = VULKAN_SHADOW_MAP_WIDTH,
                    .height = VULKAN_SHADOW_MAP_HEIGHT
                }
            },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 0,
            .pColorAttachments = nullptr,
            .pDepthAttachment = &shadow_pass_depth_attachment,
            .pStencilAttachment = nullptr
        };
        vkCmdBeginRendering(context.graphics_command_buffers[context.frame_index], &shadow_pass_rendering_info);

        vulkan_pipeline_bind(&context, &context.shadow_pipeline);

        VkViewport viewport {
            .x = 0,
            .y = 0,
            .width = (float)VULKAN_SHADOW_MAP_WIDTH,
            .height = (float)VULKAN_SHADOW_MAP_HEIGHT,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        VkRect2D scissor = {
            .offset = { .x = 0, .y = 0 },
            .extent = {
                .width = VULKAN_SHADOW_MAP_WIDTH,
                .height = VULKAN_SHADOW_MAP_HEIGHT,
            }
        };
        vkCmdSetViewport(context.graphics_command_buffers[context.frame_index], 0, 1, &viewport);
        vkCmdSetScissor(context.graphics_command_buffers[context.frame_index], 0, 1, &scissor);

        renderer_draw_scene(packet, false);

        vkCmdEndRendering(context.graphics_command_buffers[context.frame_index]);
    }

    // GRAPHICS PASS
    {
        // Transition shadow map to DEPTH_STENCIL_READ_ONLY_OPTIMAL
        vulkan_image_transition_layout_ext({
            .command_buffer = context.graphics_command_buffers[context.frame_index],
            .image = context.shadow_maps[context.frame_index].handle,
            .image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
            .base_mip_level = 0,
            .mip_levels = 1,
            .old_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            .src_access_mask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dst_access_mask = VK_ACCESS_2_SHADER_READ_BIT,
            .src_stage_mask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .dst_stage_mask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
        });

        // Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL
        vulkan_image_transition_layout_ext({
            .command_buffer = context.graphics_command_buffers[context.frame_index],
            .image = context.swapchain.images[context.image_index],
            .image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .base_mip_level = 0,
            .mip_levels = 1,
            .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .src_access_mask = VK_ACCESS_2_NONE,
            .dst_access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .src_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dst_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
        });

        // Transition multisampled color attachment to COLOR_ATTACHMENT_OPTIMAL
        vulkan_image_transition_layout_ext({
            .command_buffer = context.graphics_command_buffers[context.frame_index],
            .image = context.swapchain.color_attachment.handle,
            .image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .base_mip_level = 0,
            .mip_levels = 1,
            .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .src_access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dst_access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .src_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dst_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
        });

        // Transition depth attachment to DEPTH_ATTACHMENT_OPTIMAL
        vulkan_image_transition_layout_ext({
            .command_buffer = context.graphics_command_buffers[context.frame_index],
            .image = context.swapchain.depth_attachment.handle,
            .image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
            .base_mip_level = 0,
            .mip_levels = 1,
            .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .new_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .src_access_mask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dst_access_mask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .src_stage_mask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .dst_stage_mask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        });

        VkRenderingAttachmentInfo color_attachment {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = context.swapchain.color_attachment.view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
            .resolveImageView = context.swapchain.image_views[context.image_index],
            .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .color = { .float32 = { 0.9f, 0.9f, 0.9f, 1.0f }}
            }
        };
        VkRenderingAttachmentInfo depth_attachment {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = context.swapchain.depth_attachment.view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = {
                .depthStencil = {
                    .depth = 1.0f,
                    .stencil = 0
                }
            }
        };

        VkRenderingInfo rendering_info {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = {
                .offset = { .x = 0, .y = 0 },
                .extent = context.swapchain.extent
            },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment,
            .pDepthAttachment = &depth_attachment,
            .pStencilAttachment = nullptr
        };
        vkCmdBeginRendering(
            context.graphics_command_buffers[context.frame_index],
            &rendering_info);

        VkViewport viewport {
            .x = 0,
            .y = 0,
            .width = (float)context.swapchain.extent.width,
            .height = (float)context.swapchain.extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        VkRect2D scissor = {
            .offset = { .x = 0, .y = 0 },
            .extent = context.swapchain.extent
        };

        // Bind pipeline
        vulkan_pipeline_bind(&context, &context.graphics_pipeline);
        vkCmdSetViewport(context.graphics_command_buffers[context.frame_index], 0, 1, &viewport);
        vkCmdSetScissor(context.graphics_command_buffers[context.frame_index], 0, 1, &scissor);

        // Bind graphics descriptor sets
        vkCmdBindDescriptorSets(
            context.graphics_command_buffers[context.frame_index],
            VK_PIPELINE_BIND_POINT_GRAPHICS, context.bound_pipeline->layout,
            1, 1, &context.graphics_descriptor_sets[context.frame_index],
            0, nullptr);

        renderer_draw_scene(packet, true);

        // RENDER PASS 2 - OUTLINE
        if (packet.show_outline) {
            vulkan_pipeline_bind(&context, &context.outline_pipeline);
            renderer_draw_scene(packet, false);
        }

        vkCmdEndRendering(context.graphics_command_buffers[context.frame_index]);
    }

    // SUBMIT / PRESENT
    {
        // Transition the swapchain image to PRESENT
        vulkan_image_transition_layout_ext({
            .command_buffer = context.graphics_command_buffers[context.frame_index],
            .image = context.swapchain.images[context.image_index],
            .image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .base_mip_level = 0,
            .mip_levels = 1,
            .old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .new_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .src_access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dst_access_mask = VK_ACCESS_2_NONE,
            .src_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dst_stage_mask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
        });

        vkEndCommandBuffer(context.graphics_command_buffers[context.frame_index]);

        VkPipelineStageFlags pipeline_stage_flags[] = {
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        VkSubmitInfo submit_info {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &context.acquire_semaphores[context.frame_index],
            .pWaitDstStageMask = pipeline_stage_flags,
            .commandBufferCount = 1,
            .pCommandBuffers = &context.graphics_command_buffers[context.frame_index],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &context.submit_semaphores[context.image_index]
        };
        VkResult submit_result = vkQueueSubmit(context.device.graphics_queue,
            1, &submit_info, context.frame_fences[context.frame_index]);
        if (submit_result != VK_SUCCESS) {
            log_error("renderer_draw_frame() - vkQueueSubmit failed with result %s.", vulkan_result_str(submit_result));
            return;
        }

        VkPresentInfoKHR present_info {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &context.submit_semaphores[context.image_index],
            .swapchainCount = 1,
            .pSwapchains = &context.swapchain.handle,
            .pImageIndices = &context.image_index,
            .pResults = nullptr
        };
        VkResult present_result = vkQueuePresentKHR(context.device.present_queue, &present_info);
        if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
            log_info("vkQueuePresentKHR returned result %s. Recreating swapchain...", vulkan_result_str(present_result));
            renderer_recreate_swapchain();
        } else if (present_result != VK_SUCCESS) {
            log_error("Failed to present swapchain image: %s.", vulkan_result_str(present_result));
        }

    }

    context.frame_index = (context.frame_index + 1) % VULKAN_MAX_FRAMES_IN_FLIGHT;
}

bool renderer_load_model(const char* path, uint32_t* out_model_index) {
    VulkanModel model;
    if (!vulkan_model_load(&context, path, &model)) {
        return false;
    }
    context.model_data.push_back(model);
    *out_model_index = (uint32_t)context.model_data.size() - 1U;
    return true;
}

// DEBUG

#ifdef SBK_DEBUG

bool renderer_get_debug_extension_names(
    std::vector<const char*>& extension_names,
    std::vector<const char*>& layer_names
) {
    extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Print list of extensions
    log_debug("Required Vulkan extensions:");
    for (uint32_t index = 0; index < extension_names.size(); index++) {
        log_debug("%s", extension_names[index]);
    }

    // Debug layers
    layer_names.push_back("VK_LAYER_KHRONOS_validation");

    // Get a list of available validation layers
    uint32_t available_layer_count;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));
    std::vector<VkLayerProperties> available_layers(available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data()));

    // Verify that all required layers are available
    for (uint32_t index = 0; index < (uint32_t)layer_names.size(); index++) {
        uint32_t layer_index;
        for (layer_index = 0; layer_index < available_layer_count; layer_index++) {
            if (strcmp(layer_names[index], available_layers[layer_index].layerName) == 0) {
                break;
            }
        }

        if (layer_index < available_layer_count) {
            log_info("Found layer %s.", layer_names[index]);
        } else {
            log_error("Missing required layer %s.", layer_names[index]);
            return false;
        }
    }
    log_info("All required layers are available.");

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL renderer_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT /*message_types*/,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* /*user_data*/
) {
    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
            log_error(callback_data->pMessage);
            SBK_ASSERT(false);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
            log_warn(callback_data->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
            log_info(callback_data->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
            log_debug(callback_data->pMessage);
            break;
        }
    }

    return VK_FALSE;
}

void renderer_create_debugger() {
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = renderer_debug_callback,
        .pUserData = nullptr
    };

    PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessenger =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    SBK_ASSERT_MESSAGE(createDebugUtilsMessenger, "Failed to load createDebugUtilsMessenger function pointer.");

    VK_CHECK(createDebugUtilsMessenger(
        context.instance, &debug_create_info, context.allocator, &context.debug_messenger));

    log_info("Vulkan debugger created.");
}

#else

bool renderer_get_debug_extension_names(std::vector<const char*>&, std::vector<const char*>&) {
    return true;
}

void renderer_create_debugger() {
    context.debug_messenger = VK_NULL_HANDLE;
}

#endif

// INTERNAL

void renderer_create_sync_objects() {
    // Acquire semaphores
    for (uint32_t index = 0; index < array_length(context.acquire_semaphores); index++) {
        VkSemaphoreCreateInfo semaphore_create_info {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };
        VK_CHECK(vkCreateSemaphore(
            context.device.logical_device,
            &semaphore_create_info,
            context.allocator,
            &context.acquire_semaphores[index]));
    }

    // Submit semaphores
    context.submit_semaphores = std::vector<VkSemaphore>(context.swapchain.images.size());
    for (uint32_t index = 0; index < (uint32_t)context.swapchain.images.size(); index++) {
        VkSemaphoreCreateInfo semaphore_create_info {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };
        VK_CHECK(vkCreateSemaphore(
            context.device.logical_device,
            &semaphore_create_info,
            context.allocator,
            &context.submit_semaphores[index]));
    }

    // Frame fences
    for (uint32_t index = 0; index < array_length(context.frame_fences); index++) {
        VkFenceCreateInfo fence_create_info {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        VK_CHECK(vkCreateFence(
            context.device.logical_device,
            &fence_create_info,
            context.allocator,
            &context.frame_fences[index]));
    }
}

void renderer_destroy_sync_objects() {
    // Acquire semaphores
    for (uint32_t index = 0; index < array_length(context.acquire_semaphores); index++) {
        vkDestroySemaphore(
            context.device.logical_device,
            context.acquire_semaphores[index],
            context.allocator);
    }

    // Submit semaphores
    for (uint32_t index = 0; index < (uint32_t)context.submit_semaphores.size(); index++) {
        vkDestroySemaphore(
            context.device.logical_device,
            context.submit_semaphores[index],
            context.allocator);
    }

    // Frame fences
    for (uint32_t index = 0; index < array_length(context.frame_fences); index++) {
        vkDestroyFence(
            context.device.logical_device,
            context.frame_fences[index],
            context.allocator);
    }
}

void renderer_recreate_swapchain() {
    vkDeviceWaitIdle(context.device.logical_device);

    renderer_destroy_sync_objects();
    vulkan_swapchain_destroy(&context);
    vulkan_swapchain_create(&context);
    renderer_create_sync_objects();

    log_info("Swapchain recreated successfully.");
}
