#pragma once

#include "core/math.h"
#include <SDL3/SDL.h>

struct RendererUniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    uint8_t padding[64];
};

// Some Nvidia cards require this to be exactly 256 bytes
static_assert(sizeof(RendererUniformBufferObject) == 256ULL);

bool renderer_init(SDL_Window* window);
void renderer_quit();
void renderer_on_resized();
void renderer_draw_frame(mat4 model, mat4 view);
