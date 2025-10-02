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
void renderer_set_clear_color(float r, float g, float b, float a);
sg_swapchain renderer_get_swapchain(AppState* state);
sg_swapchain get_swapchain(AppState* state);
void load_spritesheet(void* appstate, char* file); // loads a spritesheet

// Platform-specific backend functions (implemented in backend files)
#ifdef SOKOL_D3D11
bool d3d11_backend_init(void* appstate);
void d3d11_backend_shutdown(void* appstate);
void d3d11_backend_begin_frame(void* appstate);
void d3d11_backend_end_frame(void* appstate);
void d3d11_backend_resize(void* appstate, int width, int height);
sg_swapchain d3d11_get_swapchain(void* appstate);
#endif

#ifdef SOKOL_METAL
bool metal_backend_init(void* appstate);
void metal_backend_shutdown(void* appstate);
void metal_backend_begin_frame(void* appstate);
void metal_backend_end_frame(void* appstate);
void metal_backend_resize(void* appstate, int width, int height);
sg_swapchain metal_get_swapchain(void* appstate);
#endif

#ifdef SOKOL_GLCORE
bool opengl_backend_init(void* appstate);
void opengl_backend_shutdown(void* appstate);
void opengl_backend_begin_frame(void* appstate);
void opengl_backend_end_frame(void* appstate);
void opengl_backend_resize(void* appstate, int width, int height);
sg_swapchain opengl_get_swapchain(void* appstate);
#endif

#ifdef __cplusplus
}
#endif

#endif // RENDERER_H
