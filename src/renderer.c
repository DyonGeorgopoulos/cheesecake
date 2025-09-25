#include "renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sokol implementation
#define SOKOL_IMPL
#include "sokol_gfx.h"
#include "sokol_log.h"

static renderer_context_t* g_renderer_ctx = NULL;
static sg_pass_action g_pass_action;

bool renderer_init(renderer_context_t* ctx, SDL_Window* window, int width, int height) {
    if (!ctx || !window) {
        fprintf(stderr, "Invalid parameters passed to renderer_init\n");
        return false;
    }

    ctx->window = window;
    ctx->width = width;
    ctx->height = height;
    ctx->native_context = NULL;
    ctx->is_initialized = false;

    g_renderer_ctx = ctx;

    // Platform-specific initialization
    bool backend_success = false;
    
#ifdef SOKOL_D3D11
    backend_success = d3d11_backend_init(ctx);
#elif defined(SOKOL_METAL)
    backend_success = metal_backend_init(ctx);
#elif defined(SOKOL_GLCORE)
    backend_success = opengl_backend_init(ctx);
#endif

    if (!backend_success) {
        fprintf(stderr, "Failed to initialize rendering backend\n");
        return false;
    }

    // Initialize pass action for clearing
    g_pass_action = (sg_pass_action){
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR, 
            .clear_value = {0.45f, 0.55f, 0.60f, 1.0f}
        }
    };

    ctx->is_initialized = true;
    printf("Renderer initialized successfully\n");
    return true;
}

void renderer_shutdown(renderer_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) {
        return;
    }

    sg_shutdown();

    // Platform-specific shutdown
#ifdef SOKOL_D3D11
    d3d11_backend_shutdown(ctx);
#elif defined(SOKOL_METAL)
    metal_backend_shutdown(ctx);
#elif defined(SOKOL_GLCORE)
    opengl_backend_shutdown(ctx);
#endif

    ctx->is_initialized = false;
    g_renderer_ctx = NULL;
}

void renderer_begin_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) {
        return;
    }

    // Platform-specific frame begin
#ifdef SOKOL_D3D11
    d3d11_backend_begin_frame(ctx);
#elif defined(SOKOL_METAL)
    metal_backend_begin_frame(ctx);
#elif defined(SOKOL_GLCORE)
    opengl_backend_begin_frame(ctx);
#endif

    // Get swapchain for modern sokol
    sg_swapchain swapchain = {0};
    
#ifdef SOKOL_D3D11
    swapchain = d3d11_get_swapchain(ctx);
#elif defined(SOKOL_METAL)
    swapchain = metal_get_swapchain(ctx);
#elif defined(SOKOL_GLCORE)
    swapchain = opengl_get_swapchain(ctx);
#endif

    // Begin sokol pass
    sg_pass pass = {
        .action = g_pass_action,
        .swapchain = swapchain
    };
    
    sg_begin_pass(&pass);
}

void renderer_end_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) {
        return;
    }

    sg_end_pass();
    sg_commit();

    // Platform-specific frame end
#ifdef SOKOL_D3D11
    d3d11_backend_end_frame(ctx);
#elif defined(SOKOL_METAL)
    metal_backend_end_frame(ctx);
#elif defined(SOKOL_GLCORE)
    opengl_backend_end_frame(ctx);
#endif
}

void renderer_resize(renderer_context_t* ctx, int width, int height) {
    if (!ctx || !ctx->is_initialized) {
        return;
    }

    ctx->width = width;
    ctx->height = height;

    // Platform-specific resize
#ifdef SOKOL_D3D11
    d3d11_backend_resize(ctx, width, height);
#elif defined(SOKOL_METAL)
    metal_backend_resize(ctx, width, height);
#elif defined(SOKOL_GLCORE)
    opengl_backend_resize(ctx, width, height);
#endif
}

void renderer_clear(float r, float g, float b, float a) {
    // Update the global pass action clear color
    g_pass_action.colors[0].clear_value.r = r;
    g_pass_action.colors[0].clear_value.g = g;
    g_pass_action.colors[0].clear_value.b = b;
    g_pass_action.colors[0].clear_value.a = a;
}
