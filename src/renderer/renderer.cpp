#include "renderer.h"

#include "defines.h"
#include "core/logger.h"
#include "renderer/types.h"
#include "renderer/util.h"
#include "renderer/device.h"
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

    // Create device
    if (!vulkan_device_create(&context)) {
        return false;
    }

    log_info("Renderer initialized.");
    return true;
}

void renderer_quit() {
    vkDeviceWaitIdle(context.device.logical_device);

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

}

void renderer_draw_frame() {

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
