#include "pipeline.h"

#include "core/logger.h"
#include "renderer/util.h"
#include "renderer/uniform_types.h"
#include "renderer/shader_module.h"

bool vulkan_pipeline_create_graphics(VulkanContext* context, const char* shader_path, VulkanPipeline* out_pipeline) {
    bool success;

    // Load shader module
    VkShaderModule shader_module;
    if (!vulkan_shader_module_create(context, shader_path, &shader_module)) {
        return false;
    }

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
        .stride = sizeof(Vertex3d),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex3d, position)
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex3d, normal)
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex3d, tex_coord)
        }
    };
    VkPipelineVertexInputStateCreateInfo vertex_input_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_description,
        .vertexAttributeDescriptionCount = array_length(vertex_attribute_descriptions),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions
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

    VkDescriptorSetLayout descriptor_set_layouts[] = {
        context->global_descriptor_set_layout,
        context->graphics_descriptor_set_layout,
        context->model_descriptor_set_layout
    };

    // Push constants
    VkPushConstantRange push_constants[] = {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(RendererModelPushConstants)
        }
    };

    VkPipelineLayoutCreateInfo layout_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = array_length(descriptor_set_layouts),
        .pSetLayouts = descriptor_set_layouts,
        .pushConstantRangeCount = array_length(push_constants),
        .pPushConstantRanges = push_constants
    };
    VK_CHECK(vkCreatePipelineLayout(
        context->device.logical_device,
        &layout_create_info,
        context->allocator,
        &out_pipeline->layout
    ));

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &context->swapchain.image_format.format,
        .depthAttachmentFormat = context->device.depth_format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
    };
    VkGraphicsPipelineCreateInfo pipeline_create_info {
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
        context->device.logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info,
        context->allocator, &out_pipeline->handle);
    if (vulkan_result_is_error(result)) {
        log_error("Failed to create graphics pipeline: %s.", vulkan_result_str(result));
        success = false;
        goto end;
    }

    log_debug("Graphics pipeline created.");
    success = true;
end:
    vkDestroyShaderModule(context->device.logical_device, shader_module, context->allocator);
    return success;
}

bool vulkan_pipeline_create_outline(VulkanContext* context, VulkanPipeline* out_pipeline) {
    bool success;

    // Load shader module
    VkShaderModule shader_module;
    if (!vulkan_shader_module_create(context, "shader/outline.spv", &shader_module)) {
        return false;
    }

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
        .stride = sizeof(Vertex3d),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex3d, position)
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex3d, normal)
        }
    };
    VkPipelineVertexInputStateCreateInfo vertex_input_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_description,
        .vertexAttributeDescriptionCount = array_length(vertex_attribute_descriptions),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions
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
        .cullMode = VK_CULL_MODE_FRONT_BIT,
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

    // Descriptor set layouts
    VkDescriptorSetLayout descriptor_set_layouts[] = {
        context->global_descriptor_set_layout
    };

    // Push constants
    VkPushConstantRange push_constants[] = {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(RendererModelPushConstants)
        }
    };

    VkPipelineLayoutCreateInfo layout_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = array_length(descriptor_set_layouts),
        .pSetLayouts = descriptor_set_layouts,
        .pushConstantRangeCount = array_length(push_constants),
        .pPushConstantRanges = push_constants
    };
    VK_CHECK(vkCreatePipelineLayout(
        context->device.logical_device,
        &layout_create_info,
        context->allocator,
        &out_pipeline->layout
    ));

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &context->swapchain.image_format.format,
        .depthAttachmentFormat = context->device.depth_format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
    };
    VkGraphicsPipelineCreateInfo pipeline_create_info {
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
        context->device.logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info,
        context->allocator, &out_pipeline->handle);
    if (vulkan_result_is_error(result)) {
        log_error("Failed to create outline pipeline: %s.", vulkan_result_str(result));
        success = false;
        goto end;
    }

    log_debug("Outline pipeline created.");
    success = true;
end:
    vkDestroyShaderModule(context->device.logical_device, shader_module, context->allocator);
    return success;
}

bool vulkan_pipeline_create_shadow(VulkanContext* context, VulkanPipeline* out_pipeline) {
    bool success;

    // Load shader module
    VkShaderModule shader_module;
    if (!vulkan_shader_module_create(context, "shader/shadow.spv", &shader_module)) {
        return false;
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shader_module,
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
    };

    // Vertex input
    VkVertexInputBindingDescription vertex_binding_description {
        .binding = 0,
        .stride = sizeof(Vertex3d),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex3d, position)
        },
    };
    VkPipelineVertexInputStateCreateInfo vertex_input_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_description,
        .vertexAttributeDescriptionCount = array_length(vertex_attribute_descriptions),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions
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
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_TRUE,
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
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
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
    VkPipelineColorBlendStateCreateInfo color_blend_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 0,
        .pAttachments = nullptr,
        .blendConstants = {}
    };

    // Dynamic state
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_BIAS
    };
    VkPipelineDynamicStateCreateInfo dynamic_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = array_length(dynamic_states),
        .pDynamicStates = dynamic_states
    };

    VkDescriptorSetLayout descriptor_set_layouts[] = {
        context->global_descriptor_set_layout
    };

    // Push constants
    VkPushConstantRange push_constants[] = {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(RendererModelPushConstants)
        }
    };

    VkPipelineLayoutCreateInfo layout_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = array_length(descriptor_set_layouts),
        .pSetLayouts = descriptor_set_layouts,
        .pushConstantRangeCount = array_length(push_constants),
        .pPushConstantRanges = push_constants
    };
    VK_CHECK(vkCreatePipelineLayout(
        context->device.logical_device,
        &layout_create_info,
        context->allocator,
        &out_pipeline->layout
    ));

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 0,
        .pColorAttachmentFormats = nullptr,
        .depthAttachmentFormat = context->device.depth_format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
    };
    VkGraphicsPipelineCreateInfo pipeline_create_info {
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
        context->device.logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info,
        context->allocator, &out_pipeline->handle);
    if (vulkan_result_is_error(result)) {
        log_error("Failed to create shadow pipeline: %s.", vulkan_result_str(result));
        success = false;
        goto end;
    }

    log_debug("Graphics pipeline created.");
    success = true;
end:
    vkDestroyShaderModule(context->device.logical_device, shader_module, context->allocator);
    return success;
}


void vulkan_pipeline_destroy(VulkanContext* context, VulkanPipeline* pipeline) {
    vkDestroyPipeline(context->device.logical_device, pipeline->handle, context->allocator);
    vkDestroyPipelineLayout(context->device.logical_device, pipeline->layout, context->allocator);
}

void vulkan_pipeline_bind(VulkanContext* context, VulkanPipeline* pipeline) {
    vkCmdBindPipeline(
        context->graphics_command_buffers[context->frame_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle);
    context->bound_pipeline = pipeline;
}
