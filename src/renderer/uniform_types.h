#pragma once

#include "core/math.h"

struct RendererUniformBufferObject {
    mat4 view;
    mat4 projection;
    mat4 normal;
    vec4 view_position;
};

struct RendererLightData {
    vec4 light_position;
    vec4 light_color;
};

struct RendererPushConstants {
    mat4 model;
};
