#include "window.h"
#include "renderer.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "font_rendering.h"

#include "shader.glsl.h"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Sokol + SDL3 Demo"
#define TARGET_FPS 240 
#define TARGET_FRAME_TIME (1000 / TARGET_FPS)

static sg_shader g_shader;
sg_pipeline g_pipeline;
sg_bindings g_bind;
renderer_context_t renderer_ctx;
text_renderer_t renderer;

// FPS counter state
typedef struct {
    int frame_count;
    Uint64 last_fps_update;
    float current_fps;
    char fps_text[32];
} fps_counter_t;

static fps_counter_t fps_counter = {0};

void fps_counter_update(AppState* state) {
    fps_counter.frame_count++;
    
    // Update FPS display every second (1000ms)
    Uint64 elapsed = state->current_tick - fps_counter.last_fps_update;
    if (elapsed >= 1000) {
        fps_counter.current_fps = (float)fps_counter.frame_count / (elapsed / 1000.0f);
        snprintf(fps_counter.fps_text, sizeof(fps_counter.fps_text), 
                 "FPS: %.1f", fps_counter.current_fps);
        
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
    AppState* state = SDL_malloc(sizeof(AppState));
    *appstate = state;

    printf("Starting %s...\n", WINDOW_TITLE);

    // Initialize window
    if (!window_init(state, WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        fprintf(stderr, "Failed to initialize window\n");
        return -1;
    }

    // Initialize renderer
    if (!renderer_init(&renderer_ctx, window_get_handle(&state->window), WINDOW_WIDTH, WINDOW_HEIGHT)) {
        fprintf(stderr, "Failed to  initialize renderer\n");
        window_shutdown(&state->window);
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

    g_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        // if the vertex layout doesn't have gaps, don't need to provide strides and offsets
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT3,
                [1].format = SG_VERTEXFORMAT_FLOAT4
            }
        },
        .shader = g_shader,
    });

    // a vertex buffer with the triangle vertices
    const float vertices[] = {
        // positions            colors
         0.0f, 0.5f, 0.5f,      1.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f
    };
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices)
    });

    // define resource bindings
    g_bind = (sg_bindings){
        .vertex_buffers[0] = vbuf
    };

    text_renderer_init(&renderer, 1000);
    state->font = text_renderer_load_font(&renderer, "assets/fonts/Roboto-Black.ttf", 16);

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

    // Update FPS counter
    fps_counter_update(state);

    // Handle window resize
    int current_width, current_height;
    SDL_GetWindowSizeInPixels(state->window, &current_width, &current_height);
    if (current_width != renderer_ctx.width || current_height != renderer_ctx.height) {
        renderer_resize(&renderer_ctx, current_width, current_height);
    }

    // Begin frame
    renderer_begin_frame(&renderer_ctx);

    // update this for drawing all the entities
    sg_apply_pipeline(g_pipeline);
    sg_apply_bindings(&g_bind);
    sg_draw(0, 3, 1);
    
    
    // text / ui code should go here
    sg_apply_pipeline(renderer.pipeline);

    text_renderer_begin(&renderer);
    
    // Draw FPS counter in top-left corner
    text_renderer_draw_text(&renderer, state->font, fps_counter.fps_text, 
                           0, 20, 1.0f, (float[4]){0.0f, 1.0f, 0.0f, 1.0f});
    
    text_renderer_draw_text(&renderer, state->font, "GAME", 
    current_width / 2, 20, 1.0f, (float[4]){0.0f, 1.0f, 0.0f, 1.0f});

    text_renderer_render(&renderer, current_width, current_height); 

    // End frame and present
    renderer_end_frame(&renderer_ctx);
    window_present(&state->window);
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
            if (event->window.windowID == SDL_GetWindowID(state->window)) {
                state->width = event->window.data1;
                state->height = event->window.data2;
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
    renderer_shutdown(&renderer_ctx);
    window_shutdown(&state->window);

    printf("Application shut down successfully\n");
    // SDL will clean up the window/renderer for us.
}
