#include "model.h"

#include "core/logger.h"
#include "core/math.h"
#include "renderer/image.h"
#include "renderer/buffer.h"
#include "renderer/uniform_types.h"
#include <tinygltf/tiny_gltf_v3.h>
#include <SDL3/SDL_image.h>
#include <cstdio>

struct RendererModelAttribute {
    const tg3_accessor* accessor;
    const tg3_buffer_view* buffer_view;
    const tg3_buffer* buffer;
};

LogLevel vulkan_model_tg3_error_severity_to_log_level(tg3_severity severity);
bool vulkan_model_tg3_parse_file(const char* path, tg3_model* out_model);
bool vulkan_model_get_attribute(const tg3_model& model, const tg3_primitive& primitive, const char* key, RendererModelAttribute* out_attribute);
SDL_Surface* vulkan_model_load_image_surface(const tg3_model* model, const tg3_texture_info* texture_info);
bool vulkan_model_load_textures(VulkanContext* context, const tg3_model& model, VulkanModel* out_model);
void vulkan_model_load_materials(VulkanContext* context, const tg3_model& model, VulkanModel* out_model);
bool vulkan_model_load_meshes(VulkanContext* context, const tg3_model& model, VulkanModel* out_model);
void vulkan_model_load_nodes(const tg3_model& model, VulkanModel* out_model);
void vulkan_model_render_node(VulkanContext* context, const VulkanModel&model, const VulkanNode& node, mat4 transform);

bool vulkan_model_load(VulkanContext* context, const char* path, VulkanModel* out_model) {
    bool success = true;
    tg3_model model;

    // Zero-out out_model
    out_model->vertex_buffer.handle = VK_NULL_HANDLE;
    out_model->vertex_buffer.memory = VK_NULL_HANDLE;
    out_model->index_buffer.handle = VK_NULL_HANDLE;
    out_model->index_buffer.handle = VK_NULL_HANDLE;

    if (!vulkan_model_tg3_parse_file(path, &model)) {
        success = false;
        goto end;
    }
    if (!vulkan_model_load_textures(context, model, out_model)) {
        success = false;
        goto end;
    }
    vulkan_model_load_materials(context, model, out_model);
    if (!vulkan_model_load_meshes(context, model, out_model)) {
        success = false;
        goto end;
    }
    vulkan_model_load_nodes(model, out_model);

end:
    if (!success) {
        vulkan_model_destroy(context, out_model);
    }
    tg3_model_free(&model);

    return success;
}

void vulkan_model_destroy(VulkanContext* context, VulkanModel* model) {
    vulkan_buffer_destroy(context, &model->vertex_buffer);
    vulkan_buffer_destroy(context, &model->index_buffer);

    // Destroy textures
    for (uint32_t index = 0; index < model->textures.size(); index++) {
        vulkan_image_destroy(context, &model->textures[index]);
    }
    model->textures.clear();

    // TODO: for model loading and unloading in a scene / between scenes, would need
    // to have a separate model descriptor pool that gets reset
    model->material_descriptor_sets.clear();

    model->meshes.clear();
    model->nodes.clear();
}

LogLevel vulkan_model_tg3_error_severity_to_log_level(tg3_severity severity) {
    switch (severity) {
        case TG3_SEVERITY_ERROR:
            return LOG_LEVEL_ERROR;
        case TG3_SEVERITY_WARNING:
            return LOG_LEVEL_WARN;
        case TG3_SEVERITY_INFO:
            return LOG_LEVEL_INFO;
    }
}

bool vulkan_model_tg3_parse_file(const char* path, tg3_model* out_model) {
    bool success = true;
    tg3_parse_options options;
    tg3_error_stack error_stack;

    tg3_parse_options_init(&options);
    tg3_error_stack_init(&error_stack);

    // Load model from file
    tg3_error_code error = tg3_parse_file(out_model, &error_stack, path, strlen(path), &options);
    if (error != TG3_OK) {
        for (uint32_t index = 0; index < error_stack.count; index++) {
            LogLevel log_level = vulkan_model_tg3_error_severity_to_log_level(error_stack.entries[index].severity);
            const char* error_message = error_stack.entries[index].message
                ? error_stack.entries[index].message
                : "(null)";
            logger_output(log_level, "TinyGLTF encountered error reading %s: %s", path, error_message);
        }

        success = false;
    }

    tg3_error_stack_free(&error_stack);

    return success;
}

bool vulkan_model_get_attribute(const tg3_model& model, const tg3_primitive& primitive, const char* key, RendererModelAttribute* out_attribute) {
    // Find the attribute index that matches the key
    uint32_t attribute_index;
    for (attribute_index = 0; attribute_index < primitive.attributes_count; attribute_index++) {
        const tg3_str_int_pair* attribute = &primitive.attributes[attribute_index];
        if (strcmp(attribute->key.data, key) == 0) {
            break;
        }
    }

    if (attribute_index == primitive.attributes_count) {
        return false;
    }

    const tg3_str_int_pair* attribute = &primitive.attributes[attribute_index];
    out_attribute->accessor = &model.accessors[attribute->value];
    out_attribute->buffer_view = &model.buffer_views[out_attribute->accessor->buffer_view];
    out_attribute->buffer = &model.buffers[out_attribute->buffer_view->buffer];

    return true;
}

SDL_Surface* vulkan_model_load_texture_surface(const tg3_model& model, const tg3_texture& texture) {
    SDL_IOStream* image_stream;

    const tg3_image& image = model.images[texture.source];
    log_debug("Loading image with mime type %s.", image.mime_type.data);

    if (image.buffer_view != -1) {
        log_debug("Loading model image from buffer view %i", image.buffer_view);

        const tg3_buffer_view& buffer_view = model.buffer_views[image.buffer_view];
        const tg3_buffer& buffer = model.buffers[buffer_view.buffer];

        image_stream = SDL_IOFromConstMem(buffer.data.data + buffer_view.byte_offset, buffer_view.byte_length);
        if (!image_stream) {
            log_error("Failed to create IO stream for model image surface: %s.", SDL_GetError());
            return nullptr;
        }
    } else if (image.uri.len != 0) {
        log_debug("Loading model image with uri %s.", image.uri.data);

        char image_path[256];
        sprintf(image_path, "../res/model/%s", image.uri.data);
        image_stream = SDL_IOFromFile(image_path, "rb");
    } else {
        log_error("No way to load image.");
        return nullptr;
    }

    return vulkan_image_load_surface(image_stream, 0);
}

bool vulkan_model_load_textures(VulkanContext* context, const tg3_model& model, VulkanModel* out_model) {
    bool success;

    // Load surfaces for each texture
    std::vector<SDL_Surface*> texture_surfaces;
    texture_surfaces.reserve(model.textures_count);
    for (uint32_t texture_index = 0; texture_index < model.textures_count; texture_index++) {
        const tg3_texture& texture = model.textures[texture_index];
        SDL_Surface* surface = vulkan_model_load_texture_surface(model, texture);
        if (!surface) {
            success = false;
            goto end;
        }

        texture_surfaces.push_back(surface);
    }

    // Create vulkan images based on the surfaces
    out_model->textures = std::vector<VulkanImage>(model.textures_count);
    success = vulkan_image_create_textures(context, {
        .mipmap_type = VULKAN_IMAGE_MIPMAP_SCALED,
        .surface_count = (uint32_t)texture_surfaces.size(),
        .surfaces = texture_surfaces.data()
    }, out_model->textures.data());
end:
    for (uint32_t index = 0; index < texture_surfaces.size(); index++) {
        SDL_DestroySurface(texture_surfaces[index]);
    }

    return success;
}

void vulkan_model_load_materials(VulkanContext* context, const tg3_model& model, VulkanModel* out_model) {
    std::vector<VkDescriptorImageInfo> image_infos;
    std::vector<VkWriteDescriptorSet> descriptor_writes;
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;

    // Allocate descriptors for the materials
    out_model->material_descriptor_sets = std::vector<VkDescriptorSet>(model.materials_count);
    std::vector<VkDescriptorSetLayout> set_layouts(model.materials_count, context->graphics_pipeline.descriptor_set_layouts[1]);
    descriptor_set_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = context->descriptor_pool,
        .descriptorSetCount = model.materials_count,
        .pSetLayouts = set_layouts.data()
    };
    VK_CHECK(vkAllocateDescriptorSets(
        context->device.logical_device,
        &descriptor_set_allocate_info,
        out_model->material_descriptor_sets.data()));

    // Image info for each texture
    image_infos.reserve(model.textures_count);
    for (uint32_t index = 0; index < model.textures_count; index++) {
        image_infos.push_back({
            .sampler = context->texture_sampler,
            .imageView = out_model->textures[index].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        });
    }

    // Image info for the fallback texture
    VkDescriptorImageInfo fallback_texture_image_info {
        .sampler = context->texture_sampler,
        .imageView = context->fallback_texture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    // Descriptor writes for each material
    descriptor_writes.reserve(model.textures_count);
    for (uint32_t index = 0; index < model.materials_count; index++) {
        const tg3_material& material = model.materials[index];
        const tg3_pbr_metallic_roughness& pbr_material = material.pbr_metallic_roughness;
        const int color_texture_index = pbr_material.base_color_texture.index;

        descriptor_writes.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = out_model->material_descriptor_sets[index],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = color_texture_index == -1
                ? &fallback_texture_image_info
                : &image_infos[color_texture_index],
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        });
    }
    vkUpdateDescriptorSets(
        context->device.logical_device,
        descriptor_writes.size(), descriptor_writes.data(),
        0, nullptr);
}

bool vulkan_model_load_meshes(VulkanContext* context, const tg3_model& model, VulkanModel* out_model) {
    bool success = true;
    std::vector<Vertex3d> vertices;
    std::vector<uint32_t> indices;

    out_model->meshes.reserve(model.meshes_count);
    for (uint32_t mesh_index = 0; mesh_index < model.meshes_count; mesh_index++) {
        const tg3_mesh& mesh = model.meshes[mesh_index];

        VulkanMesh vulkan_mesh;
        vulkan_mesh.primitives.reserve(mesh.primitives_count);

        for (uint32_t primitive_index = 0; primitive_index < mesh.primitives_count; primitive_index++) {
            const tg3_primitive& primitive = mesh.primitives[primitive_index];
            const uint32_t base_index = (uint32_t)vertices.size();

            // Get position attribute
            RendererModelAttribute position_attribute;
            if (!vulkan_model_get_attribute(model, primitive, "POSITION", &position_attribute)) {
                log_error("Error loading model. Mesh %u primitive %u has no attribute POSITION.", mesh_index, primitive_index);
                success = false;
                goto end;
            }

            // Get normal attribute
            RendererModelAttribute normal_attribute;
            if (!vulkan_model_get_attribute(model, primitive, "NORMAL", &normal_attribute)) {
                log_error("Error loading model. Mesh %u primitive %u has no attribute POSITION.", mesh_index, primitive_index);
                success = false;
                goto end;
            }

            // Get tex coord attribute
            RendererModelAttribute tex_coord_attribute;
            bool has_tex_coords = vulkan_model_get_attribute(model, primitive, "TEXCOORD_0", &tex_coord_attribute);
            if (!has_tex_coords) {
                log_warn("Model has no attribute TEXCOORD_0.");
            }

            // Store vertices
            for (uint32_t index = 0; index < position_attribute.accessor->count; index++) {
                const float* position_data = (float*)(position_attribute.buffer->data.data + position_attribute.buffer_view->byte_offset + position_attribute.accessor->byte_offset + (index * sizeof(vec3)));
                const float* normal_data = (float*)(normal_attribute.buffer->data.data + normal_attribute.buffer_view->byte_offset + normal_attribute.accessor->byte_offset + (index * sizeof(vec3)));
                const float* tex_coord_data = has_tex_coords
                    ? (float*)(tex_coord_attribute.buffer->data.data + tex_coord_attribute.buffer_view->byte_offset + tex_coord_attribute.accessor->byte_offset + (index * sizeof(vec2)))
                    : nullptr;

                vertices.push_back({
                    .position = vec3(position_data[0], position_data[1], position_data[2]),
                    .normal = vec3(normal_data[0], normal_data[1], normal_data[2]),
                    .tex_coord = has_tex_coords
                        ? vec2(tex_coord_data[0], tex_coord_data[1])
                        : vec2(0.0f, 0.0f)
                });
            }

            // Get indices
            const tg3_accessor& index_accessor = model.accessors[primitive.indices];
            const tg3_buffer_view& index_buffer_view = model.buffer_views[index_accessor.buffer_view];
            const tg3_buffer& index_buffer = model.buffers[index_buffer_view.buffer];

            // Store indices
            const uint8_t* index_data_ptr = index_buffer.data.data + index_buffer_view.byte_offset + index_accessor.byte_offset;
            for (uint32_t index = 0; index < index_accessor.count; index++) {
                switch (index_accessor.component_type) {
                    case TG3_COMPONENT_TYPE_UNSIGNED_BYTE: {
                        indices.push_back(base_index + (uint32_t)(*index_data_ptr));
                        index_data_ptr += sizeof(uint8_t);
                        break;
                    }
                    case TG3_COMPONENT_TYPE_UNSIGNED_SHORT: {
                        uint16_t* index_data_ptr_uint16 = (uint16_t*)index_data_ptr;
                        indices.push_back(base_index + (uint32_t)(*index_data_ptr_uint16));
                        index_data_ptr += sizeof(uint16_t);
                        break;
                    }
                    case TG3_COMPONENT_TYPE_UNSIGNED_INT: {
                        indices.push_back(base_index + *((uint32_t*)index_data_ptr));
                        index_data_ptr += sizeof(uint32_t);
                        break;
                    }
                    default: {
                        log_error("Failed to load model %s. Unhandled index component type %u.", index_accessor.component_type);
                        success = false;
                        goto end;
                    }
                }
            }

            vulkan_mesh.primitives.push_back({
                .first_index = (uint32_t)(indices.size() - index_accessor.count),
                .index_count = (uint32_t)index_accessor.count,
                .material_index = primitive.material == -1
                    ? VULKAN_MESH_MATERIAL_NONE
                    : (uint32_t)primitive.material
            });
        } // End for each primitive

        out_model->meshes.push_back(vulkan_mesh);
    } // End for each mesh

    // Create vertex buffer
    success = vulkan_buffer_create(context, {
        .size = vertices.size() * sizeof(Vertex3d),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bind_on_create = true
    }, &out_model->vertex_buffer);
    if (!success) {
        goto end;
    }

    // Upload vertices to vertex buffer
    vulkan_buffer_upload_data(context, &out_model->vertex_buffer, {
        .offset = 0,
        .size = vertices.size() * sizeof(Vertex3d),
        .data = vertices.data()
    });

    // Create index buffer
    success = vulkan_buffer_create(context, {
        .size = indices.size() * sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bind_on_create = true
    }, &out_model->index_buffer);
    if (!success) {
        goto end;
    }

    // Upload indices to index buffer
    vulkan_buffer_upload_data(context, &out_model->index_buffer, {
        .offset = 0,
        .size = indices.size() * sizeof(uint32_t),
        .data = indices.data()
    });

end:
    return success;
}

void vulkan_model_load_nodes(const tg3_model& model, VulkanModel* out_model) {
    out_model->nodes = std::vector<VulkanNode>(model.nodes_count);
    for (uint32_t node_index = 0; node_index < model.nodes_count; node_index++) {
        out_model->nodes[node_index].parent_index = VULKAN_NODE_PARENT_NONE;
    }

    for (uint32_t node_index = 0; node_index < model.nodes_count; node_index++) {
        const tg3_node& node = model.nodes[node_index];

        const quat rotation = quat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
        const vec3 translation = vec3(node.translation[0], node.translation[1], node.translation[2]);
        const vec3 scale = vec3(node.scale[0], node.scale[1], node.scale[2]);
        out_model->nodes[node_index].local_transform =  mat4::scale(scale) * rotation.to_mat4() * mat4::translation(translation);

        // Mesh
        out_model->nodes[node_index].mesh_index = node.mesh == -1
            ? VULKAN_NODE_MESH_NONE
            : (uint32_t)node.mesh;

        // Children
        out_model->nodes[node_index].child_indices.reserve(node.children_count);
        for (uint32_t child_index = 0; child_index < node.children_count; child_index++) {
            out_model->nodes[node_index].child_indices.push_back(node.children[child_index]);
            out_model->nodes[node.children[child_index]].parent_index = node_index;
        }
    }
}

void vulkan_model_render(VulkanContext* context, const VulkanModel& model, mat4 transform) {
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(
        context->graphics_command_buffers[context->frame_index],
        0, 1, &model.vertex_buffer.handle, &offsets);
    vkCmdBindIndexBuffer(
        context->graphics_command_buffers[context->frame_index],
        model.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    for (uint32_t node_index = 0; node_index < model.nodes.size(); node_index++) {
        const VulkanNode& node = model.nodes[node_index];
        if (node.parent_index != VULKAN_NODE_PARENT_NONE) {
            continue;
        }

        vulkan_model_render_node(context, model, node, transform);
    }
}

void vulkan_model_render_node(VulkanContext* context, const VulkanModel&model, const VulkanNode& node, mat4 transform) {
    if (node.mesh_index == VULKAN_NODE_MESH_NONE) {
        return;
    }

    mat4 model_matrix = node.local_transform * transform;
    RendererPushConstants constants {
        .model = model_matrix,
        .normal = model_matrix.inversed().transposed()
    };

    // Render each primitive
    const VulkanMesh& mesh = model.meshes[node.mesh_index];
    for (uint32_t primitive_index = 0; primitive_index < mesh.primitives.size(); primitive_index++) {
        const VulkanPrimitive& primitive = mesh.primitives[primitive_index];

        // Bind material
        if (primitive.material_index != VULKAN_MESH_MATERIAL_NONE) {
            vkCmdBindDescriptorSets(
                context->graphics_command_buffers[context->frame_index],
                VK_PIPELINE_BIND_POINT_GRAPHICS, context->bound_pipeline->layout,
                1, 1, &model.material_descriptor_sets[primitive.material_index],
                0, nullptr);
        }

        // Push model matrix
        vkCmdPushConstants(context->graphics_command_buffers[context->frame_index], context->bound_pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(RendererPushConstants), &constants);

        vkCmdDrawIndexed(
            context->graphics_command_buffers[context->frame_index],
            primitive.index_count, 1, primitive.first_index, 0, 0);
    }

    // Render each child
    for (uint32_t child_index = 0; child_index < node.child_indices.size(); child_index++) {
        vulkan_model_render_node(context, model, model.nodes[node.child_indices[child_index]], constants.model);
    }
}
