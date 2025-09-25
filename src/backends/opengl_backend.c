#ifdef SOKOL_GLCORE

#include "../renderer.h"
#include "sokol_gfx.h"
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

    // Set OpenGL attributes before creating context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Allocate OpenGL context
    opengl_context_t* gl_ctx = (opengl_context_t*)malloc(sizeof(opengl_context_t));
    if (!gl_ctx) {
        fprintf(stderr, "Failed to allocate OpenGL context\n");
        return false;
    }
    memset(gl_ctx, 0, sizeof(opengl_context_t));

    // Create OpenGL context
    gl_ctx->gl_context = SDL_GL_CreateContext(ctx->window);
    if (!gl_ctx->gl_context) {
        fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
        free(gl_ctx);
        return false;
    }

    // Make context current
    if (SDL_GL_MakeCurrent(ctx->window, gl_ctx->gl_context) != 0) {
        fprintf(stderr, "Failed to make OpenGL context current: %s\n", SDL_GetError());
        SDL_GL_DestroyContext(gl_ctx->gl_context);
        free(gl_ctx);
        return false;
    }

    ctx->native_context = gl_ctx;

    // Initialize Sokol GFX now that we have an OpenGL context
    sg_desc desc = {0};
    sg_setup(&desc);
    
    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to initialize Sokol GFX\n");
        SDL_GL_DestroyContext(gl_ctx->gl_context);
        free(gl_ctx);
        return false;
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

    if (gl_ctx->initialized) {
        sg_shutdown();
    }

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

    // Begin default pass with Sokol
    sg_begin_pass(&(sg_pass){ .action = gl_ctx->pass_action });
}

void opengl_backend_end_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    opengl_context_t* gl_ctx = (opengl_context_t*)ctx->native_context;
    
    if (!gl_ctx->initialized) {
        return;
    }

    // End the render pass
    sg_end_pass();
    
    // Commit the frame
    sg_commit();

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
    (void)ctx; // Unused parameter
    
    // For OpenGL, return a default swapchain - Sokol will use the default framebuffer
    sg_swapchain swapchain = {0};
    return swapchain;
}

#endif // SOKOL_GLCORE
