#ifdef SOKOL_GLCORE

#include "sokol_gfx.h"
#include "sokol_gp.h"
#include "sokol_log.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

typedef struct {
    SDL_GLContext gl_context;
    sg_pass_action pass_action;
    bool initialized;
} opengl_context_t;

static opengl_context_t g_gl_ctx = {0};

bool renderer_init(AppState* state) {
    if (!state || !state->window) {
        fprintf(stderr, "Invalid state or window for OpenGL backend\n");
        return false;
    }
    
    g_gl_ctx.gl_context = SDL_GL_CreateContext(state->window);
    if (!g_gl_ctx.gl_context) {
        fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
        return false;
    }
    
    if (!SDL_GL_MakeCurrent(state->window, g_gl_ctx.gl_context)) {
        fprintf(stderr, "Failed to make OpenGL context current: %s\n", SDL_GetError());
        SDL_GL_DestroyContext(g_gl_ctx.gl_context);
        return false;
    }

    // Initialize Sokol GFX
    sg_setup(&(sg_desc){
        .logger.func = slog_func,
    });
    
    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to initialize Sokol GFX\n");
        SDL_GL_DestroyContext(g_gl_ctx.gl_context);
        return false;
    }

    // Initialize Sokol GP
    sgp_desc sgpdesc = {0};
    sgp_setup(&sgpdesc);
    if (!sgp_is_valid()) {
        fprintf(stderr, "Failed to create Sokol GP context: %s\n", 
                sgp_get_error_message(sgp_get_last_error()));
        sg_shutdown();
        SDL_GL_DestroyContext(g_gl_ctx.gl_context);
        return false;
    }
    
    // Setup pass action for clearing
    g_gl_ctx.pass_action = (sg_pass_action) {
        .colors[0] = { 
            .load_action = SG_LOADACTION_CLEAR, 
            .clear_value = {0.0f, 0.0f, 0.0f, 1.0f}
        }
    };

    g_gl_ctx.initialized = true;
    printf("OpenGL backend initialized successfully\n");
    return true;
}

void renderer_shutdown(AppState* state) {
    (void)state;
    
    if (g_gl_ctx.initialized) {
        sgp_shutdown();
        sg_shutdown();
    }

    if (g_gl_ctx.gl_context) {
        SDL_GL_DestroyContext(g_gl_ctx.gl_context);
        g_gl_ctx.gl_context = NULL;
    }

    g_gl_ctx.initialized = false;
}

void renderer_begin_frame(AppState* state) {
    if (!g_gl_ctx.initialized) {
        return;
    }

    // Make context current (in case it was lost)
    SDL_GL_MakeCurrent(state->window, g_gl_ctx.gl_context);
}

void renderer_end_frame(AppState* state) {
    if (!g_gl_ctx.initialized) {
        return;
    }    
    // Swap buffers
    SDL_GL_SwapWindow(state->window);
}

void renderer_resize(AppState* state, int width, int height) {
    if (!g_gl_ctx.initialized) {
        return;
    }

    // Make context current
    SDL_GL_MakeCurrent(state->window, g_gl_ctx.gl_context);

    // Update state dimensions
    state->width = width;
    state->height = height;
}

sg_swapchain get_swapchain(AppState* state) {
    if (!state || !state->window) {
        fprintf(stderr, "Invalid state in renderer_get_swapchain\n");
        sg_swapchain empty = {0};
        return empty;
    }
    
    int width = state->width;
    int height = state->height;
    
    // Handle minimized window case
    if (width <= 0 || height <= 0) {
        width = 1;
        height = 1;
    }
    
    sg_swapchain swapchain = {0};
    swapchain.width = width;
    swapchain.height = height;
    swapchain.sample_count = 1;
    swapchain.color_format = SG_PIXELFORMAT_RGBA8;
    swapchain.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    swapchain.gl.framebuffer = 0;  // Default framebuffer
    
    return swapchain;
}

#endif // SOKOL_GLCORE