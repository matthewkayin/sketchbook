#pragma once

#include <SDL3/SDL.h>

bool renderer_init(SDL_Window* window);
void renderer_quit();
void renderer_on_resized();
void renderer_draw_frame();
