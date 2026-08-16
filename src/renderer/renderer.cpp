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
void renderer_create_uniform_objects();
void renderer_destroy_uniform_objects();
void renderer_recreate_swapchain();
void renderer_create_texture_sampler();
void renderer_destroy_texture_sampler();
bool renderer_create_hatch_textures();
void renderer_destroy_hatch_textures();

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
    if (!vulkan_pipeline_create_graphics(&context, &context.graphics_pipeline)) {
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

    // Load hatch textures
    if (!renderer_create_hatch_textures()) {
        return false;
    }

    renderer_create_texture_sampler();
    renderer_create_uniform_objects();

    if (!renderer_load_model("../res/model/teacup.glb", &context.model_vertices, &context.model_indices)) {
        return false;
    }

    // Create vertex buffer
    vulkan_buffer_create(&context, {
        .size = context.model_vertices.size() * sizeof(Vertex3d),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bind_on_create = true
    }, &context.vertex_buffer);
    vulkan_buffer_upload_data(&context, &context.vertex_buffer, {
        .offset = 0,
        .size = context.model_vertices.size() * sizeof(Vertex3d),
        .data = context.model_vertices.data()
    });

    // Create index buffer
    vulkan_buffer_create(&context, {
        .size = context.model_indices.size() * sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bind_on_create = true
    }, &context.index_buffer);
    vulkan_buffer_upload_data(&context, &context.index_buffer, {
        .offset = 0,
        .size = context.model_indices.size() * sizeof(uint32_t),
        .data = context.model_indices.data()
    });

    context.frame_index = 0;

    log_info("Renderer initialized.");
    return true;
}

void renderer_quit() {
    vkDeviceWaitIdle(context.device.logical_device);

    vulkan_buffer_destroy(&context, &context.vertex_buffer);
    vulkan_buffer_destroy(&context, &context.index_buffer);
    renderer_destroy_uniform_objects();
    renderer_destroy_texture_sampler();
    renderer_destroy_hatch_textures();
    renderer_destroy_sync_objects();
    vkFreeCommandBuffers(
        context.device.logical_device,
        context.device.graphics_command_pool,
        array_length(context.graphics_command_buffers),
        context.graphics_command_buffers);
    vulkan_pipeline_destroy(&context, &context.graphics_pipeline);
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
            .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f }}
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
    vkCmdBindPipeline(
        context.graphics_command_buffers[context.frame_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        context.graphics_pipeline.handle);

    VkViewport viewport {
        .x = 0,
        .y = 0,
        .width = (float)context.swapchain.extent.width,
        .height = (float)context.swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(context.graphics_command_buffers[context.frame_index], 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { .x = 0, .y = 0 },
        .extent = context.swapchain.extent
    };
    vkCmdSetScissor(context.graphics_command_buffers[context.frame_index], 0, 1, &scissor);

    // DRAW MODEL

    RendererUniformBufferObject ubo {
        .model = packet.model,
        .view = packet.view,
        .projection = mat4::perspective(
            45.0f * SBK_DEG_TO_RAD,
            (float)context.swapchain.extent.width / (float)context.swapchain.extent.height,
            0.1f, 1000.0f),
        .normal = packet.model.inversed().transposed(),
        .view_position = vec4(packet.view_position, 0.0f)
    };

    // This accounts for the fact that our math library is GL-style (Y coordinate inverted)
    // should probably change the math library or the coordinate system creation
    ubo.projection.data[5] *= -1;

    vulkan_buffer_load_data(&context, &context.uniform_buffers[context.frame_index], {
        .offset = 0,
        .size = sizeof(ubo),
        .data = &ubo
    });

    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(
        context.graphics_command_buffers[context.frame_index],
        0, 1, &context.vertex_buffer.handle, &offsets);
    vkCmdBindIndexBuffer(
        context.graphics_command_buffers[context.frame_index],
        context.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(
        context.graphics_command_buffers[context.frame_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, context.graphics_pipeline.layout,
        0, 1, &context.descriptor_sets[context.frame_index], 0, nullptr);

    vkCmdDrawIndexed(context.graphics_command_buffers[context.frame_index], context.model_indices.size(), 1, 0, 0, 0);

    // END FRAME

    vkCmdEndRendering(context.graphics_command_buffers[context.frame_index]);

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

    context.frame_index = (context.frame_index + 1) % VULKAN_MAX_FRAMES_IN_FLIGHT;
}

void renderer_set_light_data(const RendererLightData& data) {
    vulkan_buffer_upload_data(&context, &context.light_data_buffer, {
        .offset = 0,
        .size = sizeof(data),
        .data = &data
    });
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

void renderer_create_uniform_objects() {
    // Create uniform buffers
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        vulkan_buffer_create(&context, {
            .size = VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(RendererUniformBufferObject),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .bind_on_create = true
        }, &context.uniform_buffers[index]);
    }

    // Create light data buffer
    vulkan_buffer_create(&context, {
        .size = sizeof(RendererLightData),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bind_on_create = true
    }, &context.light_data_buffer);

    // Create descriptor pool
    VkDescriptorPoolSize descriptor_pool_sizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = VULKAN_MAX_FRAMES_IN_FLIGHT
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = VULKAN_MAX_FRAMES_IN_FLIGHT
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = VULKAN_MAX_FRAMES_IN_FLIGHT * VULKAN_HATCH_TEXTURE_IMAGE_COUNT
        }
    };
    VkDescriptorPoolCreateInfo descriptor_pool_create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = VULKAN_MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = array_length(descriptor_pool_sizes),
        .pPoolSizes = descriptor_pool_sizes
    };
    VK_CHECK(vkCreateDescriptorPool(
        context.device.logical_device,
        &descriptor_pool_create_info,
        context.allocator,
        &context.descriptor_pool));

    // Create descriptor sets
    VkDescriptorSetLayout layouts[VULKAN_MAX_FRAMES_IN_FLIGHT] = {
        context.graphics_pipeline.descriptor_set_layout,
        context.graphics_pipeline.descriptor_set_layout
    };
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = context.descriptor_pool,
        .descriptorSetCount = array_length(layouts),
        .pSetLayouts = layouts
    };
    VK_CHECK(vkAllocateDescriptorSets(
        context.device.logical_device, &descriptor_set_allocate_info, context.descriptor_sets));

    // Write descriptor sets
    std::vector<VkWriteDescriptorSet> descriptor_writes;

    // Uniform buffer descriptors
    VkDescriptorBufferInfo uniform_buffer_infos[VULKAN_MAX_FRAMES_IN_FLIGHT];
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        uniform_buffer_infos[index] = {
            .buffer = context.uniform_buffers[index].handle,
            .offset = 0,
            .range = sizeof(RendererUniformBufferObject)
        };

        descriptor_writes.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = context.descriptor_sets[index],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &uniform_buffer_infos[index],
            .pTexelBufferView = nullptr
        });
    }

    // Light data storage buffer descriptors
    VkDescriptorBufferInfo light_data_buffer_info {
        .buffer = context.light_data_buffer.handle,
        .offset = 0,
        .range = sizeof(RendererLightData)
    };
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        descriptor_writes.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = context.descriptor_sets[index],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &light_data_buffer_info,
            .pTexelBufferView = nullptr
        });
    }

    // Hatch texture combined image sampler descriptors
    VkDescriptorImageInfo image_infos[VULKAN_MAX_FRAMES_IN_FLIGHT];
    for (uint32_t image_index = 0; image_index < VULKAN_HATCH_TEXTURE_IMAGE_COUNT; image_index++) {
        image_infos[image_index] = {
            .sampler = context.texture_sampler,
            .imageView = context.hatch_textures[image_index].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
            descriptor_writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = context.descriptor_sets[index],
                .dstBinding = 2 + image_index,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_infos[image_index],
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            });
        }
    }

    vkUpdateDescriptorSets(context.device.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
}

void renderer_destroy_uniform_objects() {
    vkDestroyDescriptorPool(context.device.logical_device, context.descriptor_pool, context.allocator);
    for (uint32_t index = 0; index < VULKAN_MAX_FRAMES_IN_FLIGHT; index++) {
        vulkan_buffer_destroy(&context, &context.uniform_buffers[index]);
    }
    vulkan_buffer_destroy(&context, &context.light_data_buffer);
}

void renderer_recreate_swapchain() {
    vkDeviceWaitIdle(context.device.logical_device);

    renderer_destroy_sync_objects();
    vulkan_swapchain_destroy(&context);
    vulkan_swapchain_create(&context);
    renderer_create_sync_objects();

    log_info("Swapchain recreated successfully.");
}

void renderer_create_texture_sampler() {
    // Create texture sample
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
        .maxAnisotropy = context.device.properties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    VK_CHECK(vkCreateSampler(
        context.device.logical_device,
        &sampler_create_info,
        context.allocator,
        &context.texture_sampler));
}

void renderer_destroy_texture_sampler() {
    vkDestroySampler(context.device.logical_device, context.texture_sampler, context.allocator);
}

bool renderer_create_hash_textures() {
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

        hatch_surfaces[index] = vulkan_image_load_surface(image_stream);
        if (!hatch_surfaces[index]) {
            success = false;
            goto end;
        }
    }

    success = vulkan_image_create_hatch_textures(&context, hatch_surfaces, context.hatch_textures);

end:
    for (uint32_t index = 0; index < VULKAN_HATCH_TEXTURE_CHANNEL_COUNT; index++) {
        if (hatch_surfaces[index]) {
            SDL_DestroySurface(hatch_surfaces[index]);
        }
    }

    return success;
}

void renderer_destroy_hatch_textures() {
    for (uint32_t index = 0; index < VULKAN_HATCH_TEXTURE_IMAGE_COUNT; index++) {
        vulkan_image_destroy(&context, &context.hatch_textures[index]);
    }
}
