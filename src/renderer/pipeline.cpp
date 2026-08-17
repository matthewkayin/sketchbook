#include "pipeline.h"

#include "core/logger.h"
#include "renderer/util.h"
#include "util/file.h"
#include <algorithm>

bool vulkan_pipeline_create(VulkanContext* context, VulkanPipelineCreateParams params, VulkanPipeline* out_pipeline) {
    // Read shader file
    std::vector<uint8_t> shader_contents;
    if (!file_read_blob(params.shader_path, &shader_contents)) {
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
    VkShaderModule shader_module;
    VK_CHECK(vkCreateShaderModule(
        context->device.logical_device, &shader_module_create_info, context->allocator, &shader_module));

    // Shader stages
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shader_module,
            .pName = "vertex_main",
            .pSpecializationInfo = nullptr
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shader_module,
            .pName = "fragment_main",
            .pSpecializationInfo = nullptr
        }
    };

    // Vertex input
    VkVertexInputBindingDescription vertex_binding_description {
        .binding = 0,
        .stride = params.vertex_input_stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkPipelineVertexInputStateCreateInfo vertex_input_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_description,
        .vertexAttributeDescriptionCount = params.attribute_count,
        .pVertexAttributeDescriptions = params.attributes
    };

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Viewport state
    VkPipelineViewportStateCreateInfo viewport_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr
    };

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterization_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    // Multisample
    VkPipelineMultisampleStateCreateInfo multisample_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = context->device.msaa_sample_count,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    // Depth stencil state
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_NEVER,
            .compareMask = 0,
            .writeMask = 0,
            .reference = 0
        },
        .back = {
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_NEVER,
            .compareMask = 0,
            .writeMask = 0,
            .reference = 0
        },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 0.0f
    };

    // Color blend state
    VkPipelineColorBlendAttachmentState color_blend_attachment {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    VkPipelineColorBlendStateCreateInfo color_blend_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = {}
    };

    // Dynamic state
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = array_length(dynamic_states),
        .pDynamicStates = dynamic_states
    };

    // Count descriptor sets
    uint32_t descriptor_set_count = 0;
    for (uint32_t index = 0; index < params.descriptor_count; index++) {
        descriptor_set_count = std::max(descriptor_set_count, params.descriptors[index].set + 1);
    }

    out_pipeline->descriptor_set_layouts = std::vector<VkDescriptorSetLayout>(descriptor_set_count);
    for (uint32_t descriptor_set_index = 0; descriptor_set_index < descriptor_set_count; descriptor_set_index++) {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (uint32_t index = 0; index < params.descriptor_count; index++) {
            if (params.descriptors[index].set == descriptor_set_index) {
                bindings.push_back({
                    .binding = params.descriptors[index].binding,
                    .descriptorType = params.descriptors[index].type,
                    .descriptorCount = 1,
                    .stageFlags = params.descriptors[index].stage_flags,
                    .pImmutableSamplers = nullptr
                });
            }
        }
        if (bindings.empty()) {
            // TODO: clean things up
            log_error("Pipeline descriptor set %u has no bindings.", descriptor_set_index);
            return false;
        }

        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = (uint32_t)bindings.size(),
            .pBindings = bindings.data()
        };
        VK_CHECK(vkCreateDescriptorSetLayout(
            context->device.logical_device,
            &descriptor_set_layout_create_info,
            context->allocator,
            &out_pipeline->descriptor_set_layouts[descriptor_set_index]));
    }

    VkPipelineLayoutCreateInfo layout_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = (uint32_t)out_pipeline->descriptor_set_layouts.size(),
        .pSetLayouts = out_pipeline->descriptor_set_layouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };
    VK_CHECK(vkCreatePipelineLayout(
        context->device.logical_device,
        &layout_create_info,
        context->allocator,
        &out_pipeline->layout));

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &context->swapchain.image_format.format,
        .depthAttachmentFormat = context->device.depth_format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
    };
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_create_info,
        .flags = 0,
        .stageCount = array_length(shader_stages),
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = out_pipeline->layout,
        .renderPass = nullptr,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    VkResult result = vkCreateGraphicsPipelines(
        context->device.logical_device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info,
        context->allocator, &out_pipeline->handle);
    if (vulkan_result_is_error(result)) {
        log_error("Failed to create graphics pipeline: %s.", vulkan_result_str(result));
        return false;
    }

    // Destroy shader module
    vkDestroyShaderModule(context->device.logical_device, shader_module, context->allocator);

    log_debug("Vulkan graphics pipeline created.");
    return true;
}

void vulkan_pipeline_destroy(VulkanContext* context, VulkanPipeline* pipeline) {
    vkDestroyPipeline(context->device.logical_device, pipeline->handle, context->allocator);
    vkDestroyPipelineLayout(context->device.logical_device, pipeline->layout, context->allocator);

    for (uint32_t index = 0; index < pipeline->descriptor_set_layouts.size(); index++) {
        vkDestroyDescriptorSetLayout(context->device.logical_device, pipeline->descriptor_set_layouts[index], context->allocator);
    }
}
