#pragma once

#include "core/math.h"
#include <SDL3/SDL.h>

enum InputMouseButton {
    INPUT_MOUSE_BUTTON_LEFT,
    INPUT_MOUSE_BUTTON_MIDDLE,
    INPUT_MOUSE_BUTTON_RIGHT,
    INPUT_MOUSE_BUTTON_COUNT
};

void input_init(SDL_Window* window);

void input_poll_events();
bool input_user_requests_exit();

// Key
bool input_is_key_just_pressed(SDL_Scancode scancode);
bool input_is_key_just_released(SDL_Scancode scancode);
bool input_is_key_pressed(SDL_Scancode scancode);

// Mouse button
bool input_is_mouse_button_just_pressed(InputMouseButton button);
bool input_is_mouse_button_just_released(InputMouseButton button);
bool input_is_mouse_button_pressed(InputMouseButton button);

// Mouse movement
ivec2 input_get_mouse_position();
ivec2 input_get_mouse_motion();

// Mouse scroll
int input_get_mouse_scroll();
