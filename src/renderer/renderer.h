#pragma once

#include "core/math.h"
#include "renderer/uniform_types.h"
#include <SDL3/SDL.h>

struct RenderPacket {
    mat4 view;
    vec3 view_position;
    uint32_t mode;
};

bool renderer_init(SDL_Window* window);
void renderer_quit();
void renderer_on_resized();

void renderer_set_light_data(const RendererLightData& data);

bool renderer_begin_frame(RenderPacket packet);
void renderer_end_frame();

void renderer_draw_model(uint32_t index, mat4 transform);
