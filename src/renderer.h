#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

// Include sokol for types
#include "sokol_gfx.h"
#include "sokol_gp.h"
#include "util/sokol_color.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SDL_Window* window;
    void* native_context;
    int width;
    int height;
    bool is_initialized;
} renderer_context_t;

// Common renderer interface
bool renderer_init(void* appstate);
void renderer_shutdown(void* appstate);
void renderer_begin_frame(void* appstate);
void renderer_end_frame(void* appstate);
void renderer_resize(void* appstate, int width, int height);


sg_swapchain renderer_get_swapchain(AppState* state);
sg_swapchain get_swapchain(AppState* state);
void load_spritesheet(void* appstate, char* file); // loads a spritesheet

#ifdef __cplusplus
}
#endif

#endif // RENDERER_H
