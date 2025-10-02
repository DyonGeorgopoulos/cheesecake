#ifndef WINDOW_H
#define WINDOW_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SDL_Window* window;
    int width;
    int height;
    const char* title;
} window_t;

// Window management
bool window_init(void *appstate, const char* title, int width, int height);
void window_shutdown(void *appstate);
void window_poll_events(void *appstate);
void window_present(void *appstate);
SDL_Window* window_get_handle(void *appstate);
void window_get_size(void *appstate, int* width, int* height);

#ifdef __cplusplus
}
#endif

#endif // WINDOW_H
