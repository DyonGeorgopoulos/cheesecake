#ifdef SOKOL_D3D11

#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

#include "sokol_gfx.h"
#include "sokol_gp.h"
#include "sokol_log.h"

typedef struct {
    ID3D11Device* device;
    ID3D11DeviceContext* device_context;
    IDXGISwapChain* swap_chain;
    ID3D11RenderTargetView* render_target_view;
    ID3D11DepthStencilView* depth_stencil_view;
} d3d11_context_t;

static d3d11_context_t g_d3d11_ctx = {0};

// Forward declarations
static bool d3d11_create_device_and_swapchain(AppState* state);
static void d3d11_cleanup_device(void);
static void d3d11_create_render_targets(void);
static void d3d11_cleanup_render_targets(void);
static sg_environment d3d11_get_sokol_environment(void);
sg_swapchain renderer_get_swapchain(AppState* state);

bool renderer_init(AppState* state) {
    if (!state || !state->window) {
        fprintf(stderr, "Invalid state or window for D3D11 backend\n");
        return false;
    }

    if (!d3d11_create_device_and_swapchain(state)) {
        return false;
    }

    printf("D3D11 device created, now initializing sokol_gfx...\n");

    sg_desc sg_desc = {
        .environment = d3d11_get_sokol_environment(),
        .logger.func = slog_func,
    };
    
    sg_setup(&sg_desc);

    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to initialize sokol_gfx with D3D11\n");
        d3d11_cleanup_device();
        return false;
    }

    sgp_desc sgpdesc = {0};
    sgp_setup(&sgpdesc);
    if (!sgp_is_valid()) {
        fprintf(stderr, "Failed to create Sokol GP context: %s\n", 
                sgp_get_error_message(sgp_get_last_error()));
        sg_shutdown();
        d3d11_cleanup_device();
        return false;
    }

    printf("D3D11 backend initialized successfully\n");
    return true;
}

void renderer_shutdown(AppState* state) {
    (void)state;
    
    sgp_shutdown();
    sg_shutdown();
    d3d11_cleanup_device();
}

void renderer_begin_frame(AppState* state) {
    // Set render targets
    g_d3d11_ctx.device_context->lpVtbl->OMSetRenderTargets(
        g_d3d11_ctx.device_context, 
        1, 
        &g_d3d11_ctx.render_target_view, 
        g_d3d11_ctx.depth_stencil_view
    );

    // Set viewport
    D3D11_VIEWPORT viewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = (float)state->width,
        .Height = (float)state->height,
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f
    };

    g_d3d11_ctx.device_context->lpVtbl->RSSetViewports(g_d3d11_ctx.device_context, 1, &viewport);
}

void renderer_end_frame(AppState* state) {
    (void)state;
    g_d3d11_ctx.swap_chain->lpVtbl->Present(g_d3d11_ctx.swap_chain, 1, 0);
}

void renderer_resize(AppState* state, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    // Clean up old render targets
    d3d11_cleanup_render_targets();

    // Resize swap chain buffers
    HRESULT hr = g_d3d11_ctx.swap_chain->lpVtbl->ResizeBuffers(
        g_d3d11_ctx.swap_chain, 
        0,
        width, 
        height, 
        DXGI_FORMAT_UNKNOWN,
        0
    );

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to resize swap chain buffers: 0x%08lX\n", hr);
        return;
    }

    // Update state dimensions
    state->width = width;
    state->height = height;

    // Recreate render targets
    d3d11_create_render_targets();
}

void renderer_set_clear_color(float r, float g, float b, float a) {
    // D3D11 clears happen in the pass action, 
    // this function can be used to update clear values if needed
    (void)r; (void)g; (void)b; (void)a;
}

sg_swapchain get_swapchain(AppState* state) {
    return (sg_swapchain){
        .width = state->width,
        .height = state->height,
        .sample_count = 1,
        .color_format = SG_PIXELFORMAT_BGRA8,
        .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .d3d11 = {
            .render_view = g_d3d11_ctx.render_target_view,
            .resolve_view = NULL,
            .depth_stencil_view = g_d3d11_ctx.depth_stencil_view
        }
    };
}

static sg_environment d3d11_get_sokol_environment(void) {
    sg_environment env = {0};
    
    env.defaults.color_format = SG_PIXELFORMAT_BGRA8;
    env.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    env.defaults.sample_count = 1;
    
    env.d3d11.device = g_d3d11_ctx.device;
    env.d3d11.device_context = g_d3d11_ctx.device_context;
    
    if (!g_d3d11_ctx.device || !g_d3d11_ctx.device_context) {
        fprintf(stderr, "D3D11 device or context is NULL in sokol environment setup\n");
    } else {
        printf("Setting up sokol environment with valid D3D11 device: %p, context: %p\n", 
               (void*)g_d3d11_ctx.device, (void*)g_d3d11_ctx.device_context);
    }
    
    return env;
}

static bool d3d11_create_device_and_swapchain(AppState* state) {
    SDL_PropertiesID props = SDL_GetWindowProperties(state->window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    
    if (!hwnd) {
        fprintf(stderr, "Failed to get Win32 HWND from SDL3 window\n");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC swap_desc = {
        .BufferDesc = {
            .Width = (UINT)state->width,
            .Height = (UINT)state->height,
            .RefreshRate = { .Numerator = 60, .Denominator = 1 },
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM
        },
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .OutputWindow = hwnd,
        .Windowed = TRUE,
        .SwapEffect = DXGI_SWAP_EFFECT_DISCARD
    };

    UINT create_flags = 0;
    #ifdef DEBUG
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    D3D_FEATURE_LEVEL feature_level;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        create_flags,
        NULL,
        0,
        D3D11_SDK_VERSION,
        &swap_desc,
        &g_d3d11_ctx.swap_chain,
        &g_d3d11_ctx.device,
        &feature_level,
        &g_d3d11_ctx.device_context
    );

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create D3D11 device and swap chain: 0x%08lX\n", hr);
        return false;
    }

    printf("D3D11 device created with feature level: 0x%X\n", feature_level);

    d3d11_create_render_targets();
    return true;
}

static void d3d11_create_render_targets(void) {
    if (!g_d3d11_ctx.swap_chain) {
        return;
    }

    ID3D11Texture2D* back_buffer = NULL;
    HRESULT hr = g_d3d11_ctx.swap_chain->lpVtbl->GetBuffer(
        g_d3d11_ctx.swap_chain, 
        0, 
        &IID_ID3D11Texture2D, 
        (void**)&back_buffer
    );

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to get back buffer: 0x%08lX\n", hr);
        return;
    }

    hr = g_d3d11_ctx.device->lpVtbl->CreateRenderTargetView(
        g_d3d11_ctx.device, 
        (ID3D11Resource*)back_buffer, 
        NULL, 
        &g_d3d11_ctx.render_target_view
    );

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create render target view: 0x%08lX\n", hr);
        back_buffer->lpVtbl->Release(back_buffer);
        return;
    }

    D3D11_TEXTURE2D_DESC back_desc;
    back_buffer->lpVtbl->GetDesc(back_buffer, &back_desc);
    back_buffer->lpVtbl->Release(back_buffer);

    D3D11_TEXTURE2D_DESC depth_desc = {
        .Width = back_desc.Width,
        .Height = back_desc.Height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL,
    };

    ID3D11Texture2D* depth_texture = NULL;
    hr = g_d3d11_ctx.device->lpVtbl->CreateTexture2D(g_d3d11_ctx.device, &depth_desc, NULL, &depth_texture);
    
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create depth texture: 0x%08lX\n", hr);
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
        .Format = depth_desc.Format,
        .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
        .Texture2D = { .MipSlice = 0 }
    };

    hr = g_d3d11_ctx.device->lpVtbl->CreateDepthStencilView(
        g_d3d11_ctx.device, 
        (ID3D11Resource*)depth_texture, 
        &dsv_desc, 
        &g_d3d11_ctx.depth_stencil_view
    );

    depth_texture->lpVtbl->Release(depth_texture);

    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create depth stencil view: 0x%08lX\n", hr);
    }
}

static void d3d11_cleanup_render_targets(void) {
    if (g_d3d11_ctx.render_target_view) {
        g_d3d11_ctx.render_target_view->lpVtbl->Release(g_d3d11_ctx.render_target_view);
        g_d3d11_ctx.render_target_view = NULL;
    }
    
    if (g_d3d11_ctx.depth_stencil_view) {
        g_d3d11_ctx.depth_stencil_view->lpVtbl->Release(g_d3d11_ctx.depth_stencil_view);
        g_d3d11_ctx.depth_stencil_view = NULL;
    }
}

static void d3d11_cleanup_device(void) {
    d3d11_cleanup_render_targets();

    if (g_d3d11_ctx.swap_chain) {
        g_d3d11_ctx.swap_chain->lpVtbl->Release(g_d3d11_ctx.swap_chain);
        g_d3d11_ctx.swap_chain = NULL;
    }

    if (g_d3d11_ctx.device_context) {
        g_d3d11_ctx.device_context->lpVtbl->Release(g_d3d11_ctx.device_context);
        g_d3d11_ctx.device_context = NULL;
    }

    if (g_d3d11_ctx.device) {
        g_d3d11_ctx.device->lpVtbl->Release(g_d3d11_ctx.device);
        g_d3d11_ctx.device = NULL;
    }
}

#endif // SOKOL_D3D11