#ifdef SOKOL_METAL

#include "../renderer.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

typedef struct {
    void* metal_layer;      // CAMetalLayer*
    void* view;             // NSView*
    sg_pass_action pass_action;
    bool initialized;
} metal_context_t;

bool metal_backend_init(renderer_context_t* ctx) {
    if (!ctx || !ctx->window) {
        fprintf(stderr, "Invalid context or window for Metal backend\n");
        return false;
    }

    // Get native window handle using SDL3's properties system
    SDL_PropertiesID props = SDL_GetWindowProperties(ctx->window);
    if (!props) {
        fprintf(stderr, "Failed to get window properties: %s\n", SDL_GetError());
        return false;
    }

    // Allocate Metal context
    metal_context_t* metal_ctx = (metal_context_t*)malloc(sizeof(metal_context_t));
    if (!metal_ctx) {
        fprintf(stderr, "Failed to allocate Metal context\n");
        return false;
    }
    memset(metal_ctx, 0, sizeof(metal_context_t));

#ifdef __OBJC__
    @autoreleasepool {
        // Get the NSWindow from SDL3 properties
        NSWindow* ns_window = (__bridge NSWindow*)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
        if (!ns_window) {
            fprintf(stderr, "Failed to get NSWindow from SDL3 properties\n");
            free(metal_ctx);
            return false;
        }
        
        NSView* content_view = [ns_window contentView];
        
        // Create Metal layer
        CAMetalLayer* metal_layer = [CAMetalLayer layer];
        metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metal_layer.framebufferOnly = YES;
        metal_layer.drawableSize = CGSizeMake(ctx->width, ctx->height);
        
        // Set the layer as the view's backing layer
        content_view.layer = metal_layer;
        content_view.wantsLayer = YES;
        
        // Store references
        metal_ctx->metal_layer = (__bridge_retained void*)metal_layer;
        metal_ctx->view = (__bridge_retained void*)content_view;
    }
#else
    fprintf(stderr, "Metal backend requires Objective-C compilation\n");
    free(metal_ctx);
    return false;
#endif

    ctx->native_context = metal_ctx;

    // Setup Sokol GFX descriptor - simplified, no Metal-specific setup
    sg_desc desc = {0};
    
    // Initialize sokol-gfx
    sg_setup(&desc);
    
    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to initialize Sokol GFX\n");
        metal_backend_shutdown(ctx);
        return false;
    }

    // Setup pass action for clearing - fixed structure
    metal_ctx->pass_action = (sg_pass_action) {
        .colors[0] = { 
            .load_action = SG_LOADACTION_CLEAR, 
            .clear_value = {0.0f, 0.0f, 0.0f, 1.0f} 
        }
    };
    
    metal_ctx->initialized = true;
    printf("Metal backend initialized successfully\n");
    return true;
}

void metal_backend_shutdown(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    metal_context_t* metal_ctx = (metal_context_t*)ctx->native_context;

    if (metal_ctx->initialized) {
        sg_shutdown();
    }

#ifdef __OBJC__
    @autoreleasepool {
        if (metal_ctx->view) {
            NSView* view = (__bridge_transfer NSView*)metal_ctx->view;
            view.layer = nil;
            view.wantsLayer = NO;
        }
        if (metal_ctx->metal_layer) {
            CFRelease(metal_ctx->metal_layer);
        }
    }
#endif

    free(metal_ctx);
    ctx->native_context = NULL;
}

void metal_backend_begin_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    metal_context_t* metal_ctx = (metal_context_t*)ctx->native_context;
    
    if (!metal_ctx->initialized) {
        return;
    }

    // Begin default pass
    sg_begin_default_pass(&metal_ctx->pass_action, ctx->width, ctx->height);
}

void metal_backend_end_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    metal_context_t* metal_ctx = (metal_context_t*)ctx->native_context;
    
    if (!metal_ctx->initialized) {
        return;
    }

    // End the render pass
    sg_end_pass();
    
    // Commit the frame
    sg_commit();
}

void metal_backend_resize(renderer_context_t* ctx, int width, int height) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    metal_context_t* metal_ctx = (metal_context_t*)ctx->native_context;

#ifdef __OBJC__
    @autoreleasepool {
        CAMetalLayer* metal_layer = (__bridge CAMetalLayer*)metal_ctx->metal_layer;
        metal_layer.drawableSize = CGSizeMake(width, height);
    }
#endif

    // Update context dimensions
    ctx->width = width;
    ctx->height = height;
}

void metal_backend_clear(renderer_context_t* ctx, float r, float g, float b, float a) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    metal_context_t* metal_ctx = (metal_context_t*)ctx->native_context;
    
    // Update clear color - fixed field names
    metal_ctx->pass_action.colors[0].clear_value.r = r;
    metal_ctx->pass_action.colors[0].clear_value.g = g;
    metal_ctx->pass_action.colors[0].clear_value.b = b;
    metal_ctx->pass_action.colors[0].clear_value.a = a;
}

#endif // SOKOL_METAL