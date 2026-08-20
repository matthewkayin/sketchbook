#pragma once

#include "core/math.h"
#include "renderer/uniform_types.h"
#include <SDL3/SDL.h>

struct RenderPacket {
    mat4 view;
    vec3 view_position;
    uint32_t mode;

    // TODO: allow multiple model renders
    uint32_t model_index;
    mat4 model_transform;
    bool show_outline;
};

bool renderer_init(SDL_Window* window);
void renderer_quit();
void renderer_on_resized();

void renderer_set_light_data(const RendererLightData& data);

void renderer_draw_frame(RenderPacket packet);
bool renderer_load_model(const char* path, uint32_t* out_model_index);
