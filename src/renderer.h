#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <stdbool.h>

// Include sokol for types
#include "sokol_gfx.h"

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
bool renderer_init(renderer_context_t* ctx, SDL_Window* window, int width, int height);
void renderer_shutdown(renderer_context_t* ctx);
void renderer_begin_frame(renderer_context_t* ctx);
void renderer_end_frame(renderer_context_t* ctx);
void renderer_resize(renderer_context_t* ctx, int width, int height);
void renderer_clear(float r, float g, float b, float a);

// Platform-specific backend functions (implemented in backend files)
#ifdef SOKOL_D3D11
bool d3d11_backend_init(renderer_context_t* ctx);
void d3d11_backend_shutdown(renderer_context_t* ctx);
void d3d11_backend_begin_frame(renderer_context_t* ctx);
void d3d11_backend_end_frame(renderer_context_t* ctx);
void d3d11_backend_resize(renderer_context_t* ctx, int width, int height);
sg_swapchain d3d11_get_swapchain(renderer_context_t* ctx);
#endif

#ifdef SOKOL_METAL
bool metal_backend_init(renderer_context_t* ctx);
void metal_backend_shutdown(renderer_context_t* ctx);
void metal_backend_begin_frame(renderer_context_t* ctx);
void metal_backend_end_frame(renderer_context_t* ctx);
void metal_backend_resize(renderer_context_t* ctx, int width, int height);
sg_swapchain metal_get_swapchain(renderer_context_t* ctx);
#endif

#ifdef SOKOL_GLCORE
bool opengl_backend_init(renderer_context_t* ctx);
void opengl_backend_shutdown(renderer_context_t* ctx);
void opengl_backend_begin_frame(renderer_context_t* ctx);
void opengl_backend_end_frame(renderer_context_t* ctx);
void opengl_backend_resize(renderer_context_t* ctx, int width, int height);
sg_swapchain opengl_get_swapchain(renderer_context_t* ctx);
#endif

#ifdef __cplusplus
}
#endif

#endif // RENDERER_H