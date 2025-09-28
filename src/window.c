#include "window.h"
#include <stdio.h>
#include <stdlib.h>

bool window_init(window_t* win, const char* title, int width, int height) {
    if (!win) {
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
        !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) ||
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

    win->window = SDL_CreateWindow(title, width, height, window_flags);

    if (!win->window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    win->width = width;
    win->height = height;
    win->title = title;
    win->should_close = false;

    printf("Window created successfully: %dx%d\n", width, height);
    return true;
}

void window_shutdown(window_t* win) {
    if (!win) {
        return;
    }

    if (win->window) {
        SDL_DestroyWindow(win->window);
        win->window = NULL;
    }

    SDL_Quit();
}

bool window_should_close(const window_t* win) {
    return win ? win->should_close : true;
}

void window_poll_events(window_t* win) {
    if (!win) {
        return;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                win->should_close = true;
                break;
                
            case SDL_EVENT_WINDOW_RESIZED:
                if (event.window.windowID == SDL_GetWindowID(win->window)) {
                    win->width = event.window.data1;
                    win->height = event.window.data2;
                }
                break;
                
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    win->should_close = true;
                }
                break;
                
            default:
                break;
        }
        
        // Pass events to ImGui if needed
        // simgui_handle_event(&event);
    }
}

void window_present(window_t* win) {
    if (!win || !win->window) {
        return;
    }
    
    // Platform-specific present is handled in the renderer backends
}

SDL_Window* window_get_handle(const window_t* win) {
    return win ? win->window : NULL;
}

void window_get_size(const window_t* win, int* width, int* height) {
    if (!win) {
        if (width) *width = 0;
        if (height) *height = 0;
        return;
    }
    
    if (width) *width = win->width;
    if (height) *height = win->height;
}
