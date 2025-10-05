#ifndef SPRITE_LOADER_H
#define SPRITE_LOADER_H

#include "sokol_gfx.h"
#include <stdbool.h>
#include <cJSON.h>
#include "components/animation_graph.h"

#define INITIAL_SPRITE_ATLAS_CAPACITY 32

typedef struct {
    sg_image texture;
    int row;
    int frame_count;
    float frame_time;
    bool loop;
} AnimationData;

typedef struct {
    char name[32];              // "player"
    int width, height;
    float origin_x, origin_y;
    float scale_x, scale_y;
    char default_animation[32]; // "idle_left"
    
    // Animation lookup for this entity
    AnimationData *animations;  // Array of animations
    char **animation_names;     // Parallel array of names
    int animation_count;
    AnimationGraph *animation_graph;
} SpriteEntityData;

typedef struct {
    SpriteEntityData *entities;
    int entity_count;
} SpriteAtlas;

bool sprite_atlas_init(SpriteAtlas* atlas);
bool sprite_atlas_load(SpriteAtlas* atlas, const char* path);
SpriteEntityData* sprite_atlas_get(SpriteAtlas* atlas, const char* name); // gets the sprite data
void sprite_atlas_shutdown(SpriteAtlas* atlas);

#endif