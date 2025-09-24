#ifdef SOKOL_D3D11

#include "../renderer.h"
#include <SDL3/SDL.h>

#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Sokol integration
#include "sokol_gfx.h"
#include "sokol_log.h"

typedef struct {
    ID3D11Device* device;
    ID3D11DeviceContext* device_context;
    IDXGISwapChain* swap_chain;
    ID3D11RenderTargetView* render_target_view;
    ID3D11DepthStencilView* depth_stencil_view;
    SDL_Window* window;
} d3d11_context_t;

// Forward declarations
static bool d3d11_create_device_and_swapchain(d3d11_context_t* ctx, int width, int height);
static void d3d11_cleanup_device(d3d11_context_t* ctx);
static void d3d11_create_render_targets(d3d11_context_t* ctx);
static void d3d11_cleanup_render_targets(d3d11_context_t* ctx);
static sg_environment d3d11_get_sokol_environment(d3d11_context_t* ctx);

bool d3d11_backend_init(renderer_context_t* ctx) {
    if (!ctx || !ctx->window) {
        fprintf(stderr, "Invalid context or window for D3D11 backend\n");
        return false;
    }

    // Allocate D3D11 context
    d3d11_context_t* d3d11_ctx = (d3d11_context_t*)calloc(1, sizeof(d3d11_context_t));
    if (!d3d11_ctx) {
        fprintf(stderr, "Failed to allocate D3D11 context\n");
        return false;
    }
    
    d3d11_ctx->window = ctx->window;

    if (!d3d11_create_device_and_swapchain(d3d11_ctx, ctx->width, ctx->height)) {
        free(d3d11_ctx);
        return false;
    }

    printf("D3D11 device created, now initializing sokol_gfx...\n");

    // Initialize sokol_gfx with D3D11 context
    sg_desc sg_desc = {
        .environment = d3d11_get_sokol_environment(d3d11_ctx)
        // Logger will be initialized by the main renderer
    };
    sg_setup(&sg_desc);

    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to initialize sokol_gfx with D3D11\n");
        d3d11_cleanup_device(d3d11_ctx);
        free(d3d11_ctx);
        return false;
    }

    printf("sokol_gfx initialized successfully with D3D11\n");

    ctx->native_context = d3d11_ctx;
    printf("D3D11 backend initialized successfully\n");
    return true;
}

void d3d11_backend_shutdown(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    d3d11_context_t* d3d11_ctx = (d3d11_context_t*)ctx->native_context;
    d3d11_cleanup_device(d3d11_ctx);
    free(d3d11_ctx);
    ctx->native_context = NULL;
}

void d3d11_backend_begin_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    d3d11_context_t* d3d11_ctx = (d3d11_context_t*)ctx->native_context;

    // Set render targets using COM interface
    d3d11_ctx->device_context->lpVtbl->OMSetRenderTargets(
        d3d11_ctx->device_context, 
        1, 
        &d3d11_ctx->render_target_view, 
        d3d11_ctx->depth_stencil_view
    );

    // Set viewport
    D3D11_VIEWPORT viewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = (float)ctx->width,
        .Height = (float)ctx->height,
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f
    };

    d3d11_ctx->device_context->lpVtbl->RSSetViewports(d3d11_ctx->device_context, 1, &viewport);
}

void d3d11_backend_end_frame(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return;
    }

    d3d11_context_t* d3d11_ctx = (d3d11_context_t*)ctx->native_context;
    d3d11_ctx->swap_chain->lpVtbl->Present(d3d11_ctx->swap_chain, 1, 0);
}

void d3d11_backend_resize(renderer_context_t* ctx, int width, int height) {
    if (!ctx || !ctx->native_context || width <= 0 || height <= 0) {
        return;
    }

    d3d11_context_t* d3d11_ctx = (d3d11_context_t*)ctx->native_context;

    // Clean up old render targets
    d3d11_cleanup_render_targets(d3d11_ctx);

    // Resize swap chain buffers
    HRESULT hr = d3d11_ctx->swap_chain->lpVtbl->ResizeBuffers(
        d3d11_ctx->swap_chain, 
        0,              // Keep buffer count
        width, 
        height, 
        DXGI_FORMAT_UNKNOWN,  // Keep format
        0               // No flags
    );

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to resize swap chain buffers: 0x%08lX\n", hr);
        return;
    }

    // Recreate render targets with new size
    d3d11_create_render_targets(d3d11_ctx);
}

sg_swapchain d3d11_get_swapchain(renderer_context_t* ctx) {
    if (!ctx || !ctx->native_context) {
        return (sg_swapchain){0};
    }

    d3d11_context_t* d3d11_ctx = (d3d11_context_t*)ctx->native_context;
    
    return (sg_swapchain){
        .width = ctx->width,
        .height = ctx->height,
        .sample_count = 1,
        .color_format = SG_PIXELFORMAT_BGRA8,
        .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .d3d11 = {
            .render_view = d3d11_ctx->render_target_view,
            .resolve_view = NULL,
            .depth_stencil_view = d3d11_ctx->depth_stencil_view
        }
    };
}

// Sokol environment setup
static sg_environment d3d11_get_sokol_environment(d3d11_context_t* ctx) {
    sg_environment env = {0};
    
    env.defaults.color_format = SG_PIXELFORMAT_BGRA8;
    env.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    env.defaults.sample_count = 1;
    
    // Set D3D11 specific environment
    env.d3d11.device = ctx->device;
    env.d3d11.device_context = ctx->device_context;
    
    // Ensure the device pointers are valid
    if (!ctx->device || !ctx->device_context) {
        fprintf(stderr, "D3D11 device or context is NULL in sokol environment setup\n");
    } else {
        printf("Setting up sokol environment with valid D3D11 device: %p, context: %p\n", 
               (void*)ctx->device, (void*)ctx->device_context);
    }
    
    return env;
}

// Private helper functions
static bool d3d11_create_device_and_swapchain(d3d11_context_t* ctx, int width, int height) {
    // Get HWND using SDL3 properties API
    SDL_PropertiesID props = SDL_GetWindowProperties(ctx->window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    
    if (!hwnd) {
        fprintf(stderr, "Failed to get Win32 HWND from SDL3 window\n");
        return false;
    }

    // Configure swap chain descriptor
    DXGI_SWAP_CHAIN_DESC swap_desc = {
        .BufferDesc = {
            .Width = (UINT)width,
            .Height = (UINT)height,
            .RefreshRate = { .Numerator = 60, .Denominator = 1 },
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM
        },
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .OutputWindow = hwnd,
        .Windowed = TRUE,
        .SwapEffect = DXGI_SWAP_EFFECT_DISCARD
    };

    // Device creation flags
    UINT create_flags = 0;
    #ifdef DEBUG
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    // Create device and swap chain
    D3D_FEATURE_LEVEL feature_level;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL,                           // Default adapter
        D3D_DRIVER_TYPE_HARDWARE,       // Hardware acceleration
        NULL,                           // No software module
        create_flags,                   // Creation flags
        NULL,                           // Feature levels (use defaults)
        0,                              // Number of feature levels
        D3D11_SDK_VERSION,              // SDK version
        &swap_desc,                     // Swap chain descriptor
        &ctx->swap_chain,               // Output swap chain
        &ctx->device,                   // Output device
        &feature_level,                 // Output feature level
        &ctx->device_context            // Output device context
    );

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create D3D11 device and swap chain: 0x%08lX\n", hr);
        return false;
    }

    printf("D3D11 device created with feature level: 0x%X\n", feature_level);

    // Create render targets
    d3d11_create_render_targets(ctx);
    return true;
}

static void d3d11_create_render_targets(d3d11_context_t* ctx) {
    if (!ctx->swap_chain) {
        return;
    }

    // Get back buffer
    ID3D11Texture2D* back_buffer = NULL;
    HRESULT hr = ctx->swap_chain->lpVtbl->GetBuffer(
        ctx->swap_chain, 
        0, 
        &IID_ID3D11Texture2D, 
        (void**)&back_buffer
    );

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to get back buffer: 0x%08lX\n", hr);
        return;
    }

    // Create render target view
    hr = ctx->device->lpVtbl->CreateRenderTargetView(
        ctx->device, 
        (ID3D11Resource*)back_buffer, 
        NULL, 
        &ctx->render_target_view
    );

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create render target view: 0x%08lX\n", hr);
        back_buffer->lpVtbl->Release(back_buffer);
        return;
    }

    // Get back buffer description for depth buffer sizing
    D3D11_TEXTURE2D_DESC back_desc;
    back_buffer->lpVtbl->GetDesc(back_buffer, &back_desc);
    back_buffer->lpVtbl->Release(back_buffer);

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC depth_desc = {
        .Width = back_desc.Width,
        .Height = back_desc.Height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL,
        .CPUAccessFlags = 0,
        .MiscFlags = 0
    };

    ID3D11Texture2D* depth_texture = NULL;
    hr = ctx->device->lpVtbl->CreateTexture2D(ctx->device, &depth_desc, NULL, &depth_texture);
    
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create depth texture: 0x%08lX\n", hr);
        return;
    }

    // Create depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
        .Format = depth_desc.Format,
        .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
        .Texture2D = { .MipSlice = 0 }
    };

    hr = ctx->device->lpVtbl->CreateDepthStencilView(
        ctx->device, 
        (ID3D11Resource*)depth_texture, 
        &dsv_desc, 
        &ctx->depth_stencil_view
    );

    depth_texture->lpVtbl->Release(depth_texture);

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create depth stencil view: 0x%08lX\n", hr);
    }
}

static void d3d11_cleanup_render_targets(d3d11_context_t* ctx) {
    if (ctx->render_target_view) {
        ctx->render_target_view->lpVtbl->Release(ctx->render_target_view);
        ctx->render_target_view = NULL;
    }
    
    if (ctx->depth_stencil_view) {
        ctx->depth_stencil_view->lpVtbl->Release(ctx->depth_stencil_view);
        ctx->depth_stencil_view = NULL;
    }
}

static void d3d11_cleanup_device(d3d11_context_t* ctx) {
    d3d11_cleanup_render_targets(ctx);

    if (ctx->swap_chain) {
        ctx->swap_chain->lpVtbl->Release(ctx->swap_chain);
        ctx->swap_chain = NULL;
    }

    if (ctx->device_context) {
        ctx->device_context->lpVtbl->Release(ctx->device_context);
        ctx->device_context = NULL;
    }

    if (ctx->device) {
        ctx->device->lpVtbl->Release(ctx->device);
        ctx->device = NULL;
    }
}

#endif // SOKOL_D3D11