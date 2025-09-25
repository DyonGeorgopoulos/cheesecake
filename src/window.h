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
    bool should_close;
} window_t;

// Window management
bool window_init(window_t* win, const char* title, int width, int height);
void window_shutdown(window_t* win);
bool window_should_close(const window_t* win);
void window_poll_events(window_t* win);
void window_present(window_t* win);
SDL_Window* window_get_handle(const window_t* win);
void window_get_size(const window_t* win, int* width, int* height);

#ifdef __cplusplus
}
#endif

#endif // WINDOW_H
