#include "input_system.h"

void input_system_init(AppState* state) {
    state->input_component = ecs_new(state->ecs);
    ecs_set_name(state->ecs, state->input_component, "InputSingleton");
    ecs_set(state->ecs, state->input_component, Input, {0});
}

SDL_AppResult handle_input_event(AppState* state, SDL_Event* event) {
    Input* input = ecs_get_mut(state->ecs, state->input_component, Input);

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event->key.key == SDLK_ESCAPE) return SDL_APP_SUCCESS;
            if (event->key.key == SDLK_LEFT) state->input.left = true;
            if (event->key.key == SDLK_RIGHT) state->input.right = true;
            if (event->key.key == SDLK_UP) state->input.up = true;
            if (event->key.key == SDLK_DOWN) state->input.down = true;
        break;

        case SDL_EVENT_KEY_UP:
            if (event->key.key == SDLK_LEFT) state->input.left = false;
            if (event->key.key == SDLK_RIGHT) state->input.right = false;
            if (event->key.key == SDLK_UP) state->input.up = false;
            if (event->key.key == SDLK_DOWN) state->input.down = false;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                input->mouse_left_pressed = true;
            } else if (event->button.button == SDL_BUTTON_RIGHT) {
                input->mouse_right_pressed = true;
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            input->mouse_left_pressed = false;
        } else if (event->button.button == SDL_BUTTON_RIGHT) {
            input->mouse_right_pressed = false;
        }
        break;
            
        case SDL_EVENT_MOUSE_MOTION:
            input->mouse_x = (int)event->motion.x;
            input->mouse_y = (int)event->motion.y;
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}
    