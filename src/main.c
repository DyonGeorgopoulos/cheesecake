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
#include "shader.glsl.h"

// flecs
#include <flecs.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Sokol + SDL3 Demo"
#define TARGET_FPS 240 
#define TARGET_FRAME_TIME (1000 / TARGET_FPS)


typedef struct {
    float x, y;
} Position;

static float ecs_accumulator = 0.0f;
const float ECS_UPDATE_INTERVAL = 0.6f;

static sg_shader g_shader;
sg_pipeline g_pipeline;
text_renderer_t renderer;
 ecs_query_t *q;

// FPS counter state
typedef struct {
    int frame_count;
    Uint64 last_fps_update;
    float current_fps;
    char fps_text[32];
} fps_counter_t;

static fps_counter_t fps_counter = {0};

void Move(ecs_iter_t* it) {
    Position *p = ecs_field(it, Position, 0);

    for (int i=0; i < it->count; i++) {
        p[i].x = (float)(rand() % 1280);
        p[i].y = (float)(rand() % 720);
    }
}

void fps_counter_update(AppState* state) {
    fps_counter.frame_count++;
    
    // Update FPS display every second (1000ms)
    Uint64 elapsed = state->current_tick - fps_counter.last_fps_update;
    if (elapsed >= 1000) {
        fps_counter.current_fps = (float)fps_counter.frame_count / (elapsed / 1000.0f);
        snprintf(fps_counter.fps_text, sizeof(fps_counter.fps_text), 
                 "FPS: %.0f", fps_counter.current_fps);
        
        fps_counter.frame_count = 0;
        fps_counter.last_fps_update = state->current_tick;
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

    // initalise an entity
    ECS_COMPONENT(state->ecs, Position);

    q = ecs_query(state->ecs, {
        .terms = {
            { .id = ecs_id(Position) },
        },

        // QueryCache Auto automatically caches all terms that can be cached.
        .cache_kind = EcsQueryCacheAuto
    });

    ECS_SYSTEM(state->ecs, Move, EcsOnUpdate, Position);

    ecs_entity_t e = ecs_insert(state->ecs, ecs_value(Position, {10, 20}));

    printf("Starting %s...\n", WINDOW_TITLE);

    // Initialize window
    if (!window_init(state, WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        fprintf(stderr, "Failed to initialize window\n");
        return -1;
    }

    printf("window first");

    // Initialize renderer
    if (!renderer_init(state)) {
        fprintf(stderr, "Failed to  initialize renderer\n");
        window_shutdown(state->window);
        return -1;
    }

    printf("Application initialized successfully\n");

    // create the shader
    // initialise the shader
    g_shader = sg_make_shader(sgp_program_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(g_shader) != SG_RESOURCESTATE_VALID)
    {
        fprintf(stderr, "failed to make custom pipeline shader\n");
        exit(-1);
    }
    sgp_pipeline_desc pip_desc = {0};
    pip_desc.shader = g_shader;
    pip_desc.has_vs_color = true;
    pip_desc.blend_mode = SGP_BLENDMODE_BLEND;
    g_pipeline = sgp_make_pipeline(&pip_desc);


    // Initialise the text renderer
    text_renderer_init(&renderer, 1000);
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

    ecs_iter_t it = ecs_query_iter(state->ecs, q);
    Position* p = NULL;
    while (ecs_query_next(&it)) {
        p = ecs_field(&it, Position, 0);
    }

    // Update FPS counter
    fps_counter_update(state);

    int current_width, current_height;
    SDL_GetWindowSizeInPixels(state->window, &current_width, &current_height);

    // Begin frame
    renderer_begin_frame(state);

    // Begin render pass
    sg_pass pass = {.swapchain = renderer_get_swapchain(state)};
    sg_begin_pass(&pass);

    float ratio = current_width/(float)current_height;
    sgp_set_pipeline(g_pipeline);
    sgp_begin(current_width, current_height);
    sgp_viewport(0, 0, current_width, current_height);
    sgp_set_color(0.2f, 0.3f, 0.5f, 1);
    sgp_draw_filled_rect(p[0].x, p[0].y, 50, 50);


    // // Draw FPS counter in top-left corner
    text_renderer_draw_text(&renderer, state->font[0], fps_counter.fps_text, 
                            0, 0, 1.0f, (float[4])SG_WHITE, TEXT_ANCHOR_TOP_LEFT);

    text_renderer_draw_text(&renderer, state->font[1], "WORK IN PROGRESS", 
    current_width / 2, 0, 1.0f, (float[4])SG_WHITE, TEXT_ANCHOR_TOP_CENTER);

    // End frame and present
    sgp_flush();
    sgp_end();

    sg_end_pass();
    sg_commit();
    renderer_end_frame(state);
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
