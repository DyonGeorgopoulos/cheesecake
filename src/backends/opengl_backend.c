#ifdef SOKOL_GLCORE

#include "../renderer.h"
#include "sokol_gfx.h"
#include "sokol_gp.h"
#include "sokol_log.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    SDL_GLContext gl_context;
    sg_pass_action pass_action;
    bool initialized;
} opengl_context_t;

bool opengl_backend_init(renderer_context_t* ctx) {
    if (!ctx || !ctx->window) {
        fprintf(stderr, "Invalid context or window for OpenGL backend\n");
        return false;
    }
    
    // In SDL3, attributes should already be set during window creation
    // But let's verify they match what was set in window.c
    
    opengl_context_t* gl_ctx = (opengl_context_t*)malloc(sizeof(opengl_context_t));
    if (!gl_ctx) {
        fprintf(stderr, "Failed to allocate OpenGL context\n");
        return false;
    }
    memset(gl_ctx, 0, sizeof(opengl_context_t));
    
    gl_ctx->gl_context = SDL_GL_CreateContext(ctx->window);
    if (!gl_ctx->gl_context) {
        fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
        free(gl_ctx);
        return false;
    }
    
    // SDL3: SDL_GL_MakeCurrent returns bool (true = success)
    if (!SDL_GL_MakeCurrent(ctx->window, gl_ctx->gl_context)) {
        fprintf(stderr, "Failed to make OpenGL context current: %s\n", SDL_GetError());
        SDL_GL_DestroyContext(gl_ctx->gl_context);
        free(gl_ctx);
        return false;
    }

    ctx->native_context = gl_ctx;

    // Initialize Sokol GFX now that we have an OpenGL context
    sg_setup(&(sg_desc){
        .logger.func = slog_func,
    });
    
    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to initialize Sokol GFX\n");
        SDL_GL_DestroyContext(gl_ctx->gl_context);
        free(gl_ctx);
        return false;
    }

    sgp_desc sgpdesc = {0};
    sgp_setup(&sgpdesc);
    if (!sgp_is_valid())
    {
        fprintf(stderr, "Failed to create Sokol GP context: %s\n", sgp_get_error_message(sgp_get_last_error()));
        exit(-1);
    }
    
    // Setup pass action for clearing
    gl_ctx->pass_action = (sg_pass_action) {
        .colors[0] = { 
            .load_action = SG_LOADACTION_CLEAR, 
            .clear_value = {0.0f, 0.0f, 0.0f, 1.0f} 
        }
    };

    gl_ctx->initialized = true;
    printf("OpenGL backend initialized successfully\n");
    return true;
}

void opengl_backend_shutdown(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    opengl_context_t* gl_ctx = (opengl_context_t*)ctx->native_context;

    // Destroy OpenGL context
    if (gl_ctx->gl_context) {
        SDL_GL_DestroyContext(gl_ctx->gl_context);
    }

    free(gl_ctx);
    ctx->native_context = NULL;
}

void opengl_backend_begin_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    opengl_context_t* gl_ctx = (opengl_context_t*)ctx->native_context;
    
    if (!gl_ctx->initialized) {
        return;
    }

    // Make context current (in case it was lost)
    SDL_GL_MakeCurrent(ctx->window, gl_ctx->gl_context);
}

void opengl_backend_end_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    opengl_context_t* gl_ctx = (opengl_context_t*)ctx->native_context;
    
    if (!gl_ctx->initialized) {
        return;
    }

    // Swap buffers
    SDL_GL_SwapWindow(ctx->window);
}

void opengl_backend_resize(renderer_context_t* ctx, int width, int height) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    // Make context current
    opengl_context_t* gl_ctx = (opengl_context_t*)ctx->native_context;
    SDL_GL_MakeCurrent(ctx->window, gl_ctx->gl_context);

    // Update context dimensions
    ctx->width = width;
    ctx->height = height;
}

void opengl_backend_clear(renderer_context_t* ctx, float r, float g, float b, float a) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    opengl_context_t* gl_ctx = (opengl_context_t*)ctx->native_context;
    
    // Update clear color
    gl_ctx->pass_action.colors[0].clear_value.r = r;
    gl_ctx->pass_action.colors[0].clear_value.g = g;
    gl_ctx->pass_action.colors[0].clear_value.b = b;
    gl_ctx->pass_action.colors[0].clear_value.a = a;
}

sg_swapchain opengl_get_swapchain(renderer_context_t* ctx) {
    if (!ctx || !ctx->window) {
        fprintf(stderr, "Invalid context in opengl_get_swapchain\n");
        sg_swapchain empty = {0};
        return empty;
    }
    
    // Get current window dimensions
    int width, height;
    SDL_GetWindowSize(ctx->window, &width, &height);
    
    // Handle minimized window case
    if (width <= 0 || height <= 0) {
        width = 1;  // Minimum valid size
        height = 1;
    }
    
    sg_swapchain swapchain = {0};
    swapchain.width = width;
    swapchain.height = height;
    swapchain.sample_count = 1;
    swapchain.color_format = SG_PIXELFORMAT_RGBA8;
    swapchain.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    swapchain.gl.framebuffer = 0;  // Default framebuffer (0) for OpenGL
    
    return swapchain;
}

#endif // SOKOL_GLCORE
