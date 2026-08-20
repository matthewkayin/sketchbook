#include "defines.h"
#include "core/logger.h"
#include "core/input.h"
#include "renderer/renderer.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <vector>

static const float CAMERA_SCROLL_SPEED = 100.0f;
static const float CAMERA_YAW_SPEED = -75.0f * SBK_DEG_TO_RAD;
static const float CAMERA_PITCH_SPEED = 75.0f * SBK_DEG_TO_RAD;
static const float MODEL_ROTATE_SPEED = 75.0f * SBK_DEG_TO_RAD;

struct AppState {
    SDL_Window* window;

    uint64_t last_time;
    uint64_t last_second;
    uint32_t frames;
    uint32_t updates;
    uint32_t fps;
    uint32_t ups;

    float camera_pitch;
    float camera_yaw;
    float camera_distance;

    float model_angle;
    uint32_t mode;

    uint32_t current_model;
    std::vector<uint32_t> models;

    bool show_outline;
};
static AppState state;

bool app_init();
void app_quit();
bool app_is_running();
double app_timekeep();

int main() {
    if (!app_init()) {
        return 1;
    }

    state.camera_pitch = 45.0f * SBK_DEG_TO_RAD;
    state.camera_yaw = 45.0f * SBK_DEG_TO_RAD;
    state.camera_distance = 10.0f;
    state.model_angle = 0.0f;
    state.mode = 0;
    state.show_outline = true;

    renderer_set_light_data({
        .light_position = vec4(2.0f, 2.0f, -2.0f, 0.0f),
        .light_color = vec4(0.5f, 0.5, 0.5f, 1.0f)
    });

    while (app_is_running()) {
        input_poll_events();

        double delta = app_timekeep();

        state.camera_distance += input_get_mouse_scroll() * -CAMERA_SCROLL_SPEED * delta;
        state.camera_distance = std::clamp(state.camera_distance, 0.0f, 1000.0f);

        vec3 camera_position = vec3(
            state.camera_distance * cos(state.camera_pitch) * sin(state.camera_yaw),
            state.camera_distance * sin(state.camera_pitch),
            state.camera_distance * cos(state.camera_pitch) * cos(state.camera_yaw)
        );

        if (input_is_mouse_button_pressed(INPUT_MOUSE_BUTTON_LEFT)) {
            ivec2 mouse_motion = input_get_mouse_motion();
            state.camera_yaw += mouse_motion.x * CAMERA_YAW_SPEED * delta;
            state.camera_pitch += mouse_motion.y * CAMERA_PITCH_SPEED * delta;
            state.camera_pitch = std::clamp(state.camera_pitch, -89.0f * SBK_DEG_TO_RAD, 89.0f * SBK_DEG_TO_RAD);
        }

        for (uint32_t mode = 0; mode < 5; mode++) {
            if (input_is_key_just_pressed((SDL_Scancode)(SDL_SCANCODE_1 + mode))) {
                state.mode = mode;
            }
        }

        float model_rotation_direction = 0.0f;
        if (input_is_key_pressed(SDL_SCANCODE_A)) {
            model_rotation_direction = -1.0f;
        }
        if (input_is_key_pressed(SDL_SCANCODE_D)) {
            model_rotation_direction = 1.0f;
        }
        state.model_angle += model_rotation_direction * MODEL_ROTATE_SPEED * delta;

        if (input_is_key_just_pressed(SDL_SCANCODE_LEFT)) {
            if (state.current_model > 0) {
                state.current_model--;
            }
        }
        if (input_is_key_just_pressed(SDL_SCANCODE_RIGHT)) {
            if (state.current_model < state.models.size() - 1) {
                state.current_model++;
            }
        }

        if (input_is_key_pressed(SDL_SCANCODE_O)) {
            state.show_outline = !state.show_outline;
        }

        vec3 model_position = vec3(0.0f, 0.0f, 0.0f);
        quat model_rotation = quat::from_axis_angle(vec3::up(), state.model_angle, true);

        renderer_draw_frame({
            .view = mat4::look_at(camera_position, vec3(0.0f, 0.0f, 0.0f), vec3::up()),
            .view_position = camera_position,
            .mode = state.mode,
            .model_index = state.current_model,
            .model_transform = mat4::scale(vec3(4.0f, 4.0f, 4.0f)) * model_rotation.to_rotation_matrix(model_position),
            .show_outline = state.show_outline
        });
    }

    app_quit();
    return 0;
}

bool app_init() {
    logger_init();

    // Log initialization messages
    log_info("Initializing %s.", SBK_APP_NAME);
    log_info("Detected platform %s.", SBK_PLATFORM_STR);
    log_info("%s build.", SBK_BUILD_STR);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        log_error("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }

    // Check if Vulkan is supported
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        log_error("Failed to load Vulkan library: %s", SDL_GetError());
    }

    // Create window
    state.window = SDL_CreateWindow(SBK_APP_NAME, SBK_SCREEN_WIDTH, SBK_SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    if (state.window == nullptr) {
        log_error("Error creating window: %s", SDL_GetError());
        return false;
    }

    // Init input system
    input_init(state.window);

    // Init renderer
    if (!renderer_init(state.window)) {
        return false;
    }

    const char* model_paths[] = {
        "../res/model/plant.glb",
        "../res/model/teacup.glb"
    };
    for (uint32_t index = 0; index < array_length(model_paths); index++) {
        uint32_t model_index;
        if (!renderer_load_model(model_paths[index], &model_index)) {
            return false;
        }
        state.models.push_back(model_index);
    }

    log_info("%s initialized.", SBK_APP_NAME);
    return true;
}

void app_quit() {
    renderer_quit();

    SDL_DestroyWindow(state.window);
    SDL_Quit();

    log_info("%s quit gracefully.", SBK_APP_NAME);
    logger_quit();
}

bool app_is_running() {
    return !input_user_requests_exit();
}

double app_timekeep() {
    uint64_t current_time = SDL_GetTicksNS();
    double elapsed_time = (double)(current_time - state.last_time);
    state.last_time = current_time;

    double delta = elapsed_time / (double)SDL_NS_PER_SECOND;

    if (current_time - state.last_second >= SDL_NS_PER_SECOND) {
        state.fps = state.frames;
        state.ups = state.updates;
        state.frames = 0;
        state.updates = 0;
        state.last_second += SDL_NS_PER_SECOND;
    }

    return delta;
}
