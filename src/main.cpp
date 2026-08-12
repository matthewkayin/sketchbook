#include "core/logger.h"
#include "core/input.h"
#include "renderer/renderer.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

struct AppState {
    SDL_Window* window;

    uint64_t last_time;
    uint64_t last_second;
    uint32_t frames;
    uint32_t updates;
    uint32_t fps;
    uint32_t ups;
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

    while (app_is_running()) {
        input_poll_events();
        renderer_draw_frame();
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
