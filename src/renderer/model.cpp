#include "model.h"

#include "core/logger.h"
#include <tinygltf/tiny_gltf_v3.h>

struct RendererModelAttribute {
    const tg3_accessor* accessor;
    const tg3_buffer_view* buffer_view;
    const tg3_buffer* buffer;
};

LogLevel renderer_tg3_error_severity_to_log_level(tg3_severity severity);
bool renderer_tg3_get_model_attribute(const tg3_model* model, const tg3_primitive* primitive, const char* key, RendererModelAttribute* out_attribute);

bool renderer_load_model(const char* path, std::vector<Vertex3d>* out_vertices, std::vector<uint32_t>* out_indices) {
    tg3_parse_options options;
    tg3_error_stack error_stack;
    tg3_model model;

    uint32_t mesh_index;
    const tg3_mesh* mesh;

    bool success = true;

    tg3_parse_options_init(&options);
    tg3_error_stack_init(&error_stack);

    // Load model from file
    tg3_error_code error = tg3_parse_file(&model, &error_stack, path, strlen(path), &options);
    if (error != TG3_OK) {
        for (uint32_t index = 0; index < error_stack.count; index++) {
            LogLevel log_level = renderer_tg3_error_severity_to_log_level(error_stack.entries[index].severity);
            const char* error_message = error_stack.entries[index].message
                ? error_stack.entries[index].message
                : "(null)";
            logger_output(log_level, "TinyGLTF encountered error reading %s: %s", path, error_message);
        }

        success = false;
        goto end;
    }

    for (mesh_index = 0; mesh_index < model.meshes_count; mesh_index++) {
        mesh = &model.meshes[mesh_index];
        for (uint32_t primitive_index = 0; primitive_index < mesh->primitives_count; primitive_index++) {
            const tg3_primitive& primitive = mesh->primitives[primitive_index];

            // Get position attribute
            RendererModelAttribute position_attribute;
            if (!renderer_tg3_get_model_attribute(&model, &primitive, "POSITION", &position_attribute)) {
                log_error("Error loading model %s. Mesh %u primitive %u has no attribute POSITION.", path, mesh_index, primitive_index);
                success = false;
                goto end;
            }

            // Get normal attribute
            RendererModelAttribute normal_attribute;
            if (!renderer_tg3_get_model_attribute(&model, &primitive, "NORMAL", &normal_attribute)) {
                log_error("Error loading model %s. Mesh %u primitive %u has no attribute POSITION.", path, mesh_index, primitive_index);
                success = false;
                goto end;
            }

            // Get tex coord attribute
            RendererModelAttribute tex_coord_attribute;
            bool has_tex_coords = renderer_tg3_get_model_attribute(&model, &primitive, "TEXCOORD_0", &tex_coord_attribute);
            if (!has_tex_coords) {
                log_warn("Model %s has no attribute TEXCOORD_0.", path);
            }

            // Store vertices
            for (uint32_t index = 0; index < position_attribute.accessor->count; index++) {
                const float* position_data = (float*)(position_attribute.buffer->data.data + position_attribute.buffer_view->byte_offset + position_attribute.accessor->byte_offset + (index * sizeof(vec3)));
                const float* normal_data = (float*)(normal_attribute.buffer->data.data + normal_attribute.buffer_view->byte_offset + normal_attribute.accessor->byte_offset + (index * sizeof(vec3)));
                const float* tex_coord_data = has_tex_coords
                    ? (float*)(tex_coord_attribute.buffer->data.data + tex_coord_attribute.buffer_view->byte_offset + tex_coord_attribute.accessor->byte_offset + (index * sizeof(vec2)))
                    : nullptr;

                out_vertices->push_back({
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
                        out_indices->push_back((uint32_t)(*index_data_ptr));
                        index_data_ptr += sizeof(uint8_t);
                        break;
                    }
                    case TG3_COMPONENT_TYPE_UNSIGNED_SHORT: {
                        out_indices->push_back((uint32_t)(*((uint16_t*)index_data_ptr)));
                        index_data_ptr += sizeof(uint16_t);
                        break;
                    }
                    case TG3_COMPONENT_TYPE_UNSIGNED_INT: {
                        out_indices->push_back(*((uint32_t*)index_data_ptr));
                        index_data_ptr += sizeof(uint32_t);
                        break;
                    }
                    default: {
                        log_error("Failed to load model %s. Unhandled index component type %u.", index_accessor.component_type);
                        success = false;
                        goto end;
                        break;
                    }
                }
            }
        }
    }

    end:
        tg3_model_free(&model);
        tg3_error_stack_free(&error_stack);

        return success;
}

LogLevel renderer_tg3_error_severity_to_log_level(tg3_severity severity) {
    switch (severity) {
        case TG3_SEVERITY_ERROR:
            return LOG_LEVEL_ERROR;
        case TG3_SEVERITY_WARNING:
            return LOG_LEVEL_WARN;
        case TG3_SEVERITY_INFO:
            return LOG_LEVEL_INFO;
    }
}

bool renderer_tg3_get_model_attribute(const tg3_model* model, const tg3_primitive* primitive, const char* key, RendererModelAttribute* out_attribute) {
    // Find the attribute index that matches the key
    uint32_t attribute_index;
    for (attribute_index = 0; attribute_index < primitive->attributes_count; attribute_index++) {
        const tg3_str_int_pair* attribute = &primitive->attributes[attribute_index];
        if (strcmp(attribute->key.data, key) == 0) {
            break;
        }
    }

    if (attribute_index == primitive->attributes_count) {
        return false;
    }

    const tg3_str_int_pair* attribute = &primitive->attributes[attribute_index];
    out_attribute->accessor = &model->accessors[attribute->value];
    out_attribute->buffer_view = &model->buffer_views[out_attribute->accessor->buffer_view];
    out_attribute->buffer = &model->buffers[out_attribute->buffer_view->buffer];

    return true;
}
