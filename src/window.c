#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

bool window_init(void *appstate, const char* title, int width, int height) {
    AppState* state = (AppState*) appstate;
    if (!state->window) {
        fprintf(stderr, "Invalid window pointer\n");
        return false;
    }

    // Initialize SDL3 with video subsystem
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    // Set platform-specific window flags
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
    
#ifdef SOKOL_D3D11
    // No special flags needed for D3D11
#elif defined(SOKOL_GLCORE)
    window_flags |= SDL_WINDOW_OPENGL;
    
    // Set OpenGL attributes
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) ||
        !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) ||
        !SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) ||
        !SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) ||
        !SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) ||
        !SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8)) {
        fprintf(stderr, "Failed to set OpenGL attributes: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    #ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    #endif
#endif

    state->window = SDL_CreateWindow(title, width, height, window_flags);

    if (!state->window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    state->width = width;
    state->height = height;
    state->title = title;

    printf("Window created successfully: %dx%d\n", width, height);
    return true;
}

void window_shutdown(void* appstate) {
    AppState* state = (AppState*) appstate;
    if (!state->window) {
        return;
    }

    if (state->window) {
        SDL_DestroyWindow(state->window);
        state->window = NULL;
    }

    SDL_Quit();
}

void window_present(void* appstate) {
    AppState* state = (AppState*) appstate;
    if (!state || !state->window) {
        return;
    }
    
    // Platform-specific present is handled in the renderer backends
}

SDL_Window* window_get_handle(void* appstate) {
    AppState* state = (AppState*) appstate;
    return state ? state->window : NULL;
}

void window_get_size(void* appstate, int* width, int* height) {
    AppState* state = (AppState*) appstate;
    if (!state->window) {
        if (width) *width = 0;
        if (height) *height = 0;
        return;
    }
    
    if (width) *width = state->width;
    if (height) *height = state->height;
}
