#pragma once

#include "core/math.h"

struct RendererUniformBufferObject {
    mat4 view;
    mat4 projection;
    mat4 depth_view_projection;
    vec4 view_position;
    vec4 light_position;
    vec4 mode;
};

struct RendererLightData {
    vec4 light_position;
    vec4 light_color;
};

struct RendererModelPushConstants {
    mat4 model;
    mat4 normal;
};
