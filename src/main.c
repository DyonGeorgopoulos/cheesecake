#include "window.h"
#include "renderer.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "font_rendering.h"
#include "renderer.h"
#include "systems/rendering_system.h"
#include "shader.glsl.h"

// flecs
#include <flecs.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Sokol + SDL3 Demo"
#define TARGET_FPS 240 
#define TARGET_FRAME_TIME (1000 / TARGET_FPS)

// every component, will be in a file that has it's queries in it's init function, that are stored in the header. 
// that way system can just call into the queries to get the data they need every update.

static float ecs_accumulator = 0.0f;
const float ECS_UPDATE_INTERVAL = 0.6f;

static sg_shader g_shader;
sg_pipeline g_pipeline;
text_renderer_t renderer;
ecs_query_t *q;

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
    AppState* state = SDL_malloc(sizeof(AppState));
    *appstate = state;

    // Initialise ecs_world
    state->ecs = ecs_init();
    rendering_components_register(state->ecs);

    state->renderer.queries.shapes = ecs_query(state->ecs, {
        .terms = {
            { .id = ecs_id(Position) },
            { .id = ecs_id(Colour) }
        }
    });

    ECS_SYSTEM(state->ecs, Move, EcsOnUpdate, Position);
    ecs_entity_t player = ecs_new(state->ecs);
    ecs_entity_t player2 = ecs_new(state->ecs);
    ecs_set(state->ecs, player, Position, {10.0f, 20.0f});
    ecs_set(state->ecs, player2, Position, {10.0f, 20.0f});

    ecs_set(state->ecs, player, Colour, {
        .r = 0.1f,
        .g = 0.5f,
        .b = 0.6f,
        .a = 1,
    });

    ecs_set(state->ecs, player2, Colour, {
        .r = 0.6f,
        .g = 0.2f,
        .b = 0.1f,
        .a = 1,
    });

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

    ecs_accumulator += state->delta_time;

    // 0.6 tick system. Run game update code in here.
    if (ecs_accumulator >= ECS_UPDATE_INTERVAL) {
        ecs_progress(state->ecs, ecs_accumulator);
        ecs_accumulator = 0.0f;
    }

    // Update FPS counter
    fps_counter_update(state);

    // draw the frame
    renderer_draw_frame(state);

    app_wait_for_next_frame(appstate);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState* state = (AppState*) appstate;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
            break;
            
        case SDL_EVENT_WINDOW_RESIZED:
            int current_width, current_height;
            SDL_GetWindowSizeInPixels(state->window, &current_width, &current_height);
            if (current_width != state->width || current_height != state->height) {
                renderer_resize(state, current_width, current_height);
            }
            break;
            
        case SDL_EVENT_KEY_DOWN:
            if (event->key.key == SDLK_ESCAPE) {
                return SDL_APP_SUCCESS;
            }
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
