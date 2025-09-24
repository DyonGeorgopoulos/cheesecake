#include "renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sokol implementation - only include the main header
#define SOKOL_IMPL
#include "sokol_gfx.h"
#include "sokol_log.h"

// Don't include platform-specific sokol headers here - they're handled in backends

// Dear ImGui via cimgui - be careful about C++ constructs
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

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

    // Platform-specific initialization - this creates the D3D11 device
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

    // Initialize pass action
    g_pass_action = (sg_pass_action){
        .colors[0] = {.load_action = SG_LOADACTION_CLEAR, .clear_value = {0.1f, 0.2f, 0.3f, 1.0}},
        .depth = {.load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f} // Clear depth buffer
    };

    // Note: sokol_gfx is now initialized by the backend, not here
    // The backend sets up the proper environment with the device context

    ctx->is_initialized = true;
    printf("Renderer initialized successfully\n");
    return true;
}

void renderer_shutdown(renderer_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) {
        return;
    }

    renderer_imgui_shutdown();
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
        fprintf(stderr, "renderer_begin_frame: Invalid context\n");
        return;
    }

    printf("renderer_begin_frame: Starting frame\n");

    // Platform-specific frame begin (minimal now)
#ifdef SOKOL_D3D11
    printf("renderer_begin_frame: Calling D3D11 backend begin\n");
    d3d11_backend_begin_frame(ctx);
#elif defined(SOKOL_METAL)
    metal_backend_begin_frame(ctx);
#elif defined(SOKOL_GLCORE)
    opengl_backend_begin_frame(ctx);
#endif

    // Check if sokol is in a valid state
    if (!sg_isvalid()) {
        fprintf(stderr, "renderer_begin_frame: sokol_gfx is not in valid state!\n");
        return;
    }
    printf("renderer_begin_frame: sokol_gfx is valid\n");

    // Get swapchain for modern sokol
    printf("renderer_begin_frame: Getting swapchain\n");
    sg_swapchain swapchain = {0};
    
#ifdef SOKOL_D3D11
    swapchain = d3d11_get_swapchain(ctx);
    printf("renderer_begin_frame: Got swapchain - width: %d, height: %d\n", swapchain.width, swapchain.height);
    printf("renderer_begin_frame: Swapchain render_view: %p, depth_view: %p\n", 
           (void*)swapchain.d3d11.render_view, (void*)swapchain.d3d11.depth_stencil_view);
#elif defined(SOKOL_METAL)
    swapchain = metal_get_swapchain(ctx);
#elif defined(SOKOL_GLCORE)
    swapchain = opengl_get_swapchain(ctx);
#endif

    printf("renderer_begin_frame: About to call sg_begin_pass\n");
    
    // Create the pass with swapchain (this is the modern way)
    sg_pass pass = {
        .action = g_pass_action,
        .swapchain = swapchain
    };
    
    printf("renderer_begin_frame: Pass action color load_action: %d\n", pass.action.colors[0].load_action);
    
    if (!swapchain.d3d11.render_view || !swapchain.d3d11.resolve_view) {
        fprintf(stderr, "renderer_begin_frame: invalid swapchain views (render=%p resolve=%p)\n",
            (void*)swapchain.d3d11.render_view, (void*)swapchain.d3d11.resolve_view);
        return;
    }

    sg_begin_pass(&pass);
    printf("renderer_begin_frame: sg_begin_pass completed\n");
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
    sg_pass_action pass_action = {0};
    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action.colors[0].clear_value.r = r;
    pass_action.colors[0].clear_value.g = g;
    pass_action.colors[0].clear_value.b = b;
    pass_action.colors[0].clear_value.a = a;
}

bool renderer_imgui_init(renderer_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) {
        return false;
    }

    // Initialize ImGui context via cimgui
    struct ImGuiContext* imgui_ctx = igCreateContext(NULL);
    igSetCurrentContext(imgui_ctx);

    struct ImGuiIO* io = igGetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup ImGui style
    igStyleColorsDark(NULL);

    printf("ImGui initialized via cimgui\n");
    return true;
}

void renderer_imgui_shutdown(void) {
    igDestroyContext(NULL);
}

void renderer_imgui_new_frame(void) {
    if (!g_renderer_ctx || !g_renderer_ctx->is_initialized) {
        return;
    }
    
    // Start new ImGui frame
    igNewFrame();
}

void renderer_imgui_render(void) {
    // Render ImGui
    igRender();
    // Note: Platform-specific rendering will be handled in the backends
}