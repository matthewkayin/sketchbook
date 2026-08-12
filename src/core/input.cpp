#include "input.h"
#include "renderer/renderer.h"

struct InputState {
    SDL_Window* window;

    ivec2 mouse_position;
    ivec2 mouse_motion;
    bool key_pressed_current[SDL_SCANCODE_COUNT];
    bool key_pressed_previous[SDL_SCANCODE_COUNT];
    bool mouse_pressed_current[INPUT_MOUSE_BUTTON_COUNT];
    bool mouse_pressed_previous[INPUT_MOUSE_BUTTON_COUNT];
    bool user_requests_exit;
};
static InputState state;

void input_init(SDL_Window* window) {
    state.window = window;

    memset(state.key_pressed_current, 0, sizeof(state.key_pressed_current));
    memset(state.key_pressed_previous, 0, sizeof(state.key_pressed_previous));
    memset(state.mouse_pressed_current, 0, sizeof(state.mouse_pressed_current));
    memset(state.mouse_pressed_previous, 0, sizeof(state.mouse_pressed_previous));

    state.user_requests_exit = false;
}

void input_poll_events() {
    memcpy(state.key_pressed_previous, state.key_pressed_current, sizeof(state.key_pressed_previous));
    memcpy(state.mouse_pressed_previous, state.mouse_pressed_current, sizeof(state.mouse_pressed_previous));
    state.mouse_motion = ivec2(0, 0);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                state.user_requests_exit = false;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                state.key_pressed_current[event.key.scancode] = event.type == SDL_EVENT_KEY_DOWN;
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                state.mouse_pressed_current[event.button.button - 1] = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED: {
                int width, height;
                SDL_GetWindowSize(state.window, &width, &height);
                renderer_on_resized();
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                // Note, may need to change this to scale with screen size
                state.mouse_position = ivec2(event.motion.x, event.motion.y);
                state.mouse_motion = ivec2(event.motion.xrel, event.motion.yrel);
                break;
            }
        }
    }
}

bool input_user_requests_exit() {
    return state.user_requests_exit;
}

// KEY

bool input_is_key_just_pressed(SDL_Scancode scancode) {
    return state.key_pressed_current[scancode] &&
           !state.key_pressed_previous[scancode];
}

bool input_is_key_just_released(SDL_Scancode scancode) {
    return !state.key_pressed_current[scancode] &&
           state.key_pressed_previous[scancode];
}

bool input_is_key_pressed(SDL_Scancode scancode) {
    return state.key_pressed_current[scancode];
}

// MOUSE BUTTON

bool input_is_mouse_button_just_pressed(InputMouseButton button) {
    return state.mouse_pressed_current[button] &&
           !state.mouse_pressed_previous[button];
}

bool input_is_mouse_button_just_released(InputMouseButton button) {
    return !state.mouse_pressed_current[button] &&
           state.mouse_pressed_previous[button];
}

bool input_is_mouse_button_pressed(InputMouseButton button) {
    return state.mouse_pressed_current[button];
}

// MOUSE MOTION

ivec2 input_get_mouse_position() {
    return state.mouse_position;
}

ivec2 input_get_mouse_motion() {
    return state.mouse_motion;
}
