#include "shader_module.h"

#include "util/file.h"

bool vulkan_shader_module_create(VulkanContext* context, const char* path, VkShaderModule* out_shader_module) {
    // Read shader file
    std::vector<uint8_t> shader_contents;
    if (!file_read_blob(path, &shader_contents)) {
        return false;
    }

    // Create shader module
    VkShaderModuleCreateInfo shader_module_create_info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = shader_contents.size(),
        .pCode = (uint32_t*)shader_contents.data()
    };
    VK_CHECK(vkCreateShaderModule(
        context->device.logical_device, &shader_module_create_info, context->allocator, out_shader_module));

    return true;
}
