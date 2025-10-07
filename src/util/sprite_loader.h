#ifndef SPRITE_LOADER_H
#define SPRITE_LOADER_H

#include "sokol_gfx.h"
#include <stdbool.h>
#include <cJSON.h>
#include "components/animation_graph.h"

#define INITIAL_SPRITE_ATLAS_CAPACITY 32

// Loaded animation clip (temporary)
typedef struct {
    sg_image texture;
    int frame_count;
    int direction_count;
    float frame_time;
    bool loop;
    int row;
} LoadedAnimationClip;

// Transition definition (temporary)
typedef struct {
    char from[64];
    char to[64];
    AnimationConditionFunc condition;
    int priority;
} LoadedTransition;

// Loaded sprite data (temporary)
typedef struct {
    char name[64];
    int width, height;
    float scale_x, scale_y;
    char default_animation[64];
    
    LoadedAnimationClip clips[16];
    char clip_names[16][64];
    int clip_count;
    
    LoadedTransition *transitions;
    int transition_count;
} LoadedSpriteData;

// Atlas for all loaded sprites
typedef struct {
    LoadedSpriteData *entities;
    int entity_count;
} SpriteAtlas;

bool sprite_atlas_init(SpriteAtlas* atlas);
bool sprite_atlas_load(SpriteAtlas* atlas, const char* path);
LoadedSpriteData* sprite_atlas_get(SpriteAtlas *atlas, const char *name);
void sprite_atlas_shutdown(SpriteAtlas* atlas);
AnimationConditionFunc lookup_condition_function(const char *name);

#endif