#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include <flecs.h>
#include "SDL3/SDL.h"
#include "common.h"
#include "input.h"


SDL_AppResult handle_input_event(AppState* state, SDL_Event* event);
void input_system_init(AppState* state);
#endif