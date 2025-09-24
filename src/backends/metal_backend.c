#ifdef SOKOL_METAL

#include "../renderer.h"
#include "sokol_gfx.h"
#include "sokol_metal.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_syswm.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <stdio.h>

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#endif

typedef struct {
    void* device;           // id<MTLDevice>
    void* command_queue;    // id<MTLCommandQueue>
    void* metal_layer;      // CAMetalLayer*
    void* view;             // NSView* or UIView*
    void* current_drawable; // id<CAMetalDrawable>
} metal_context_t;

bool metal_backend_init(renderer_context_t* ctx) {
    if (!ctx || !ctx->window) {
        fprintf(stderr, "Invalid context or window for Metal backend\n");
        return false;
    }

    // Get the window handle
    SDL_SysWMinfo wm_info;
    SDL_VERSION(&wm_info.version);
    if (!SDL_GetWindowWMInfo(ctx->window, &wm_info)) {
        fprintf(stderr, "Failed to get window info: %s\n", SDL_GetError());
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
        // Get the NSWindow and its content view
        NSWindow* ns_window = (__bridge NSWindow*)wm_info.info.cocoa.window;
        NSView* content_view = [ns_window contentView];
        
        // Create Metal device
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            fprintf(stderr, "Failed to create Metal device\n");
            free(metal_ctx);
            return false;
        }
        
        // Create command queue
        id<MTLCommandQueue> command_queue = [device newCommandQueue];
        if (!command_queue) {
            fprintf(stderr, "Failed to create Metal command queue\n");
            free(metal_ctx);
            return false;
        }
        
        // Create Metal layer
        CAMetalLayer* metal_layer = [CAMetalLayer layer];
        metal_layer.device = device;
        metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metal_layer.framebufferOnly = YES;
        metal_layer.drawableSize = CGSizeMake(ctx->width, ctx->height);
        
        // Set the layer as the view's backing layer
        content_view.layer = metal_layer;
        content_view.wantsLayer = YES;
        
        // Store references
        metal_ctx->device = (__bridge_retained void*)device;
        metal_ctx->command_queue = (__bridge_retained void*)command_queue;
        metal_ctx->metal_layer = (__bridge_retained void*)metal_layer;
        metal_ctx->view = (__bridge_retained void*)content_view;
    }
#else
    // If not using Objective-C, we need to use C APIs or create Objective-C wrappers
    fprintf(stderr, "Metal backend requires Objective-C compilation\n");
    free(metal_ctx);
    return false;
#endif

    ctx->native_context = metal_ctx;

    printf("Metal backend initialized successfully\n");
    return true;
}

void metal_backend_shutdown(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    metal_context_t* metal_ctx = (metal_context_t*)ctx->native_context;

#ifdef __OBJC__
    @autoreleasepool {
        if (metal_ctx->current_drawable) {
            CFRelease(metal_ctx->current_drawable);
        }
        if (metal_ctx->view) {
            CFRelease(metal_ctx->view);
        }
        if (metal_ctx->metal_layer) {
            CFRelease(metal_ctx->metal_layer);
        }
        if (metal_ctx->command_queue) {
            CFRelease(metal_ctx->command_queue);
        }
        if (metal_ctx->device) {
            CFRelease(metal_ctx->device);
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

#ifdef __OBJC__
    @autoreleasepool {
        CAMetalLayer* metal_layer = (__bridge CAMetalLayer*)metal_ctx->metal_layer;
        
        // Get next drawable
        id<CAMetalDrawable> drawable = [metal_layer nextDrawable];
        if (drawable) {
            metal_ctx->current_drawable = (__bridge_retained void*)drawable;
        }
    }
#endif
}

void metal_backend_end_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    metal_context_t* metal_ctx = (metal_context_t*)ctx->native_context;

#ifdef __OBJC__
    @autoreleasepool {
        if (metal_ctx->current_drawable) {
            id<CAMetalDrawable> drawable = (__bridge_transfer id<CAMetalDrawable>)metal_ctx->current_drawable;
            metal_ctx->current_drawable = NULL;
            
            // Present the drawable
            id<MTLCommandQueue> command_queue = (__bridge id<MTLCommandQueue>)metal_ctx->command_queue;
            id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
            [command_buffer presentDrawable:drawable];
            [command_buffer commit];
        }
    }
#endif
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
}

#endif // SOKOL_METAL