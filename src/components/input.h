#ifndef INPUT_H
#define INPUT_H

#include <flecs.h>
#include <SDL3/SDL.h>
#include "transform.h"

typedef struct {
    float mouse_x, mouse_y;
    bool mouse_left_pressed;
    bool mouse_right_pressed;
    int grid_x, grid_y; // calculated grid position
    bool valid_grid_pos;

    // events for the frame
    SDL_Event events[32];
    int event_count;
} Input;


extern ECS_COMPONENT_DECLARE(Input);

void input_components_register(ecs_world_t *world);
#endif