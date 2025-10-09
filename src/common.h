#ifndef COMMON_H
#define COMMON_H

#include <SDL3/SDL.h>
#include <flecs.h>
#include "font_rendering.h"
#include "util/sprite_loader.h"

#define TILE_SIZE 32
typedef struct {
    bool left, right, up, down;
} InputState;

typedef struct {
    ecs_query_t *shapes;
    ecs_query_t *background_tiles;
    ecs_query_t *ground_entities;
    ecs_query_t *sprites;
    ecs_query_t *particles;
    ecs_query_t *ui_elements;
    ecs_query_t *animations;
    ecs_query_t *animation_graphs;
    ecs_query_t *direction;
} RenderQueries;

typedef struct {
    RenderQueries queries;
    text_renderer_t* text_renderer;
    sg_shader sprite_shader;
    sg_shader particle_shader;
    sg_pipeline sprite_pipeline;
    sg_pipeline particle_pipeline;
    // other render state
} Renderer;

typedef struct {
    uint64_t key;
    ecs_entity_t value;
} GridEntry;

typedef struct {
    ecs_entity_t* level;
    int map_height;
    int map_width;
} Map;
typedef struct AppState {
    SDL_Window* window;
    int width, height;
    const char* title;
    float last_tick;
    float current_tick;
    float delta_time;
    int font[5];
    Renderer renderer;
    ecs_world_t* ecs;
    SpriteAtlas sprite_atlas;
    float ecs_accumulator;
    InputState input;
    Map map;
    GridEntry* grid;
  } AppState;

#endif
