#pragma once

#include "core/math.h"
#include "renderer/uniform_types.h"
#include <SDL3/SDL.h>

struct RenderPacket {
    mat4 model;
    mat4 view;
    vec3 view_position;
};

bool renderer_init(SDL_Window* window);
void renderer_quit();
void renderer_on_resized();
void renderer_draw_frame(RenderPacket packet);
void renderer_set_light_data(const RendererLightData& data);
