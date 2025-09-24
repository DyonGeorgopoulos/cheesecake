#ifdef SOKOL_GLCORE

#include "../renderer.h"
#include "sokol_gfx.h"
#include "sokol_gl.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <stdio.h>

typedef struct {
    SDL_GLContext gl_context;
    GLuint framebuffer;
    GLuint color_texture;
    GLuint depth_texture;
    bool has_framebuffer;
} opengl_context_t;

bool opengl_backend_init(renderer_context_t* ctx) {
    if (!ctx || !ctx->window) {
        fprintf(stderr, "Invalid context or window for OpenGL backend\n");
        return false;
    }

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

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        SDL_GL_DestroyContext(gl_ctx->gl_context);
        free(gl_ctx);
        return false;
    }

    // Print OpenGL info
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));

    // Enable VSync
    SDL_GL_SetSwapInterval(1);

    // Set initial viewport
    glViewport(0, 0, ctx->width, ctx->height);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ctx->native_context = gl_ctx;

    printf("OpenGL backend initialized successfully\n");
    return true;
}

void opengl_backend_shutdown(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    opengl_context_t* gl_ctx = (opengl_context_t*)ctx->native_context;

    // Delete framebuffer objects if they exist
    if (gl_ctx->has_framebuffer) {
        if (gl_ctx->color_texture) {
            glDeleteTextures(1, &gl_ctx->color_texture);
        }
        if (gl_ctx->depth_texture) {
            glDeleteTextures(1, &gl_ctx->depth_texture);
        }
        if (gl_ctx->framebuffer) {
            glDeleteFramebuffers(1, &gl_ctx->framebuffer);
        }
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

    // Make context current (in case it was lost)
    SDL_GL_MakeCurrent(ctx->window, gl_ctx->gl_context);

    // Set viewport
    glViewport(0, 0, ctx->width, ctx->height);

    // Clear buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void opengl_backend_end_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    opengl_context_t* gl_ctx = (opengl_context_t*)ctx->native_context;

    // Swap buffers
    SDL_GL_SwapWindow(ctx->window);

    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error: 0x%04X\n", error);
    }
}

void opengl_backend_resize(renderer_context_t* ctx, int width, int height) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    opengl_context_t* gl_ctx = (opengl_context_t*)ctx->native_context;

    // Make context current
    SDL_GL_MakeCurrent(ctx->window, gl_ctx->gl_context);

    // Update viewport
    glViewport(0, 0, width, height);

    // If we have a custom framebuffer, resize it
    if (gl_ctx->has_framebuffer) {
        opengl_resize_framebuffer(gl_ctx, width, height);
    }
}

static bool opengl_create_framebuffer(opengl_context_t* gl_ctx, int width, int height) {
    // Generate framebuffer
    glGenFramebuffers(1, &gl_ctx->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_ctx->framebuffer);

    // Create color texture
    glGenTextures(1, &gl_ctx->color_texture);
    glBindTexture(GL_TEXTURE_2D, gl_ctx->color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach color texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_ctx->color_texture, 0);

    // Create depth texture
    glGenTextures(1, &gl_ctx->depth_texture);
    glBindTexture(GL_TEXTURE_2D, gl_ctx->depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach depth texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, gl_ctx->depth_texture, 0);

    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer not complete: 0x%04X\n", status);
        glDeleteFramebuffers(1, &gl_ctx->framebuffer);
        glDeleteTextures(1, &gl_ctx->color_texture);
        glDeleteTextures(1, &gl_ctx->depth_texture);
        gl_ctx->framebuffer = 0;
        gl_ctx->color_texture = 0;
        gl_ctx->depth_texture = 0;
        return false;
    }

    // Bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    gl_ctx->has_framebuffer = true;
    return true;
}

static void opengl_resize_framebuffer(opengl_context_t* gl_ctx, int width, int height) {
    if (!gl_ctx->has_framebuffer) {
        return;
    }

    // Delete old textures
    if (gl_ctx->color_texture) {
        glDeleteTextures(1, &gl_ctx->color_texture);
    }
    if (gl_ctx->depth_texture) {
        glDeleteTextures(1, &gl_ctx->depth_texture);
    }

    // Recreate textures with new size
    glBindFramebuffer(GL_FRAMEBUFFER, gl_ctx->framebuffer);

    // Recreate color texture
    glGenTextures(1, &gl_ctx->color_texture);
    glBindTexture(GL_TEXTURE_2D, gl_ctx->color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_ctx->color_texture, 0);

    // Recreate depth texture
    glGenTextures(1, &gl_ctx->depth_texture);
    glBindTexture(GL_TEXTURE_2D, gl_ctx->depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, gl_ctx->depth_texture, 0);

    // Bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Forward declarations
static bool opengl_create_framebuffer(opengl_context_t* gl_ctx, int width, int height);
static void opengl_resize_framebuffer(opengl_context_t* gl_ctx, int width, int height);

#endif // SOKOL_GLCORE