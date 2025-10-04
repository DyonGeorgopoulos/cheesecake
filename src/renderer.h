#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include "common.h"

// Include sokol for types
#include "sokol_gfx.h"
#include "sokol_gp.h"
#include "util/sokol_color.h"
#include "shader.glsl.h"

#include <flecs.h>

#include "components/rendering.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    int frame_count;
    Uint64 last_fps_update;
    float current_fps;
    char fps_text[32];
} fps_counter_t;


extern fps_counter_t fps_counter;
typedef struct {
    SDL_Window* window;
    void* native_context;
    int width;
    int height;
    bool is_initialized;
} renderer_context_t;

// Common renderer interface
bool renderer_init(void* appstate);
void renderer_shutdown(void* appstate);
void renderer_begin_frame(void* appstate);
void renderer_end_frame(void* appstate);
void renderer_resize(void* appstate, int width, int height);
void fps_counter_update(AppState* state);
bool renderer_initialize(AppState* state);
void renderer_draw_frame(void* appstate);
void update_animations(AppState *state, float dt);
void set_sprite_animation(ecs_world_t *world, ecs_entity_t entity, const char *anim_name);
sg_swapchain renderer_get_swapchain(AppState* state);
sg_swapchain get_swapchain(AppState* state);
void load_spritesheet(void* appstate, char* file); // loads a spritesheet

#ifdef __cplusplus
}
#endif

#endif // RENDERER_H
