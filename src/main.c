#include "window.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "font_rendering.h"
#include "shader.glsl.h"
#include "util/map_loader.h"
#include "entities/entity_factory.h"
#include "systems/animation_system.h"
#include "systems/render_system.h"
#include "systems/conveyor_system.h"

#include "components/animation_graph.h"
#include "components/conveyor.h"
// flecs
#include <flecs.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Cheesecake"
#define TARGET_FPS 240 
#define TARGET_FRAME_TIME (1000 / TARGET_FPS)

// every component, will be in a file that has it's queries in it's init function, that are stored in the header. 
// that way system can just call into the queries to get the data they need every update.
const float ECS_UPDATE_INTERVAL = 0.6f;

static sg_shader g_shader;
sg_pipeline g_pipeline;
text_renderer_t renderer;
ecs_query_t *q;
ecs_entity_t player;

void Move(ecs_iter_t* it) {
    Position *p = ecs_field(it, Position, 0);

    for (int i=0; i < it->count; i++) {
        p[i].x = (float)(rand() % 1280);
        p[i].y = (float)(rand() % 720);
    }
}

void app_wait_for_next_frame(void *appstate) {
    AppState* state = (AppState*) appstate;

    Uint64 frame_time = SDL_GetTicks() - state->current_tick;
    if (frame_time < TARGET_FRAME_TIME) SDL_Delay(TARGET_FRAME_TIME - frame_time);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    srand(time(NULL));

    AppState* state = SDL_calloc(1, sizeof(AppState));
    *appstate = state;

    // Initialise ecs_world
    state->ecs = ecs_init();

    ecs_set_ctx(state->ecs, state, NULL);

    // register components
    transform_components_register(state->ecs);
    animation_components_register(state->ecs);
    animation_graph_components_register(state->ecs);
    conveyor_components_register(state->ecs);
    sprite_components_register(state->ecs);

        // register systems
    ECS_SYSTEM(state->ecs, UpdateDirectionSystem, EcsOnUpdate, Velocity, Direction);
    ECS_SYSTEM(state->ecs, AnimationGraphSystem, EcsOnUpdate, AnimationSet, AnimationState, AnimationGraphComponent);
    conveyor_system_init(state->ecs);

    printf("Starting %s...\n", WINDOW_TITLE);

    // Initialize window
    if (!window_init(state, WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        fprintf(stderr, "Failed to initialize window\n");
        return -1;
    }

    printf("window first");

    // Initialize renderer
    if (!renderer_initialize(state)) {
        fprintf(stderr, "Failed to  initialize renderer\n");
        window_shutdown(state->window);
        return -1;
    }

    printf("Application initialized successfully\n");

    // Initialise the sprite system
    sprite_atlas_init(&state->sprite_atlas);
    sprite_atlas_load(&state->sprite_atlas, "assets/sprites/sprite_definitions.json");
    // spawn a player entity
    player = entity_factory_spawn_sprite(state, "player", 200, 200);
    ecs_entity_t belt = entity_factory_spawn_belt(state, 300, 300, DIR_RIGHT);
    entity_factory_spawn_conveyor_item(state, belt, LANE_LEFT);
    entity_factory_spawn_conveyor_item(state, belt, LANE_RIGHT);

    ecs_entity_t belt2 = entity_factory_spawn_belt(state, 332, 300, DIR_RIGHT);
    ecs_entity_t belt3 = entity_factory_spawn_belt(state, 364, 300, DIR_DOWN);
    entity_factory_spawn_belt(state, 364, 332, DIR_LEFT);
    entity_factory_spawn_belt(state, 332, 332, DIR_LEFT);
    entity_factory_spawn_belt(state, 300, 332, DIR_UP);

    
    // load the map
    load_map(state, "");
    // Initialise the text renderer
    text_renderer_init(&renderer, 1000);
    state->renderer.text_renderer = &renderer;
    // load the font we want to use. 
    // TODO: Update state->font to be a hasmap that keymaps the fonts so they can be looked up.c 
    state->font[0] = text_renderer_load_font(&renderer, "assets/fonts/PxPlus_Toshiba.ttf", 16);
    state->font[1] = text_renderer_load_font(&renderer, "assets/fonts/Roboto-Black.ttf", 12);
    // Initialize FPS counter
    fps_counter.last_fps_update = SDL_GetTicks();
    snprintf(fps_counter.fps_text, sizeof(fps_counter.fps_text), "FPS: 0.0");

    return 0;
}


SDL_AppResult SDL_AppIterate(void *appstate) {
    // Poll events
    AppState* state = (AppState*) appstate;
    state->last_tick = state->current_tick;
    state->current_tick = SDL_GetTicks();
    state->delta_time = (state->current_tick - state->last_tick) / 1000.0f;

    state->ecs_accumulator += state->delta_time;

    Velocity* vel = ecs_get_mut(state->ecs, player, Velocity);
    vel->x = 0;
    if (state->input.right) vel->x = 3;
    if (state->input.left) vel->x = -3;

    vel->y = 0;
    if (state->input.up) vel->y = -3;
    if (state->input.down) vel->y = 3;

    // Then let a movement system apply velocity every frame
    Position* pos = ecs_get_mut(state->ecs, player, Position);
    pos->x += vel->x * state->delta_time * 60;  // Scale by delta time
    pos->y += vel->y * state->delta_time * 60;  // Scale by delta time
    
    ecs_progress(state->ecs, state->delta_time);

    update_animations(state, state->delta_time);
    // Update FPS counter
    fps_counter_update(state);

    // draw the frame
    renderer_draw_frame(state);

    app_wait_for_next_frame(appstate);
    return SDL_APP_CONTINUE;
}

void do_resize(void* appstate) {
    AppState* state = (AppState*) appstate;
    int width, height;
    SDL_GetWindowSize(state->window, &width, &height);
    renderer_resize(state, width, height);
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState* state = (AppState*) appstate;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            do_resize(appstate); 
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
        default:
            break;
    }
        
        // Pass events to ImGui if needed
        // simgui_handle_event(&event);
    return SDL_APP_CONTINUE;
}

// This function runs once at shutdown
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    AppState* state = (AppState*) appstate;
    // Cleanup
    printf("Shutting down application...\n");
    renderer_shutdown(state);
    window_shutdown(state->window);

    printf("Application shut down successfully\n");
    // SDL will clean up the window/renderer for us.
}
