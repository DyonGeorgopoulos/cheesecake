#ifndef COMMON_H
#define COMMON_H

#include <SDL3/SDL.h>
#include <flecs.h>


typedef struct AppState {
    SDL_Window* window;
    int width, height;
    const char* title;
    float last_tick;
    float current_tick;
    float delta_time;
    int font[5];
    ecs_world_t *ecs;
  } AppState;

#endif
