#ifndef ANIMATION_H
#define ANIMATION_H

#include <stdlib.h>
#include <flecs.h>
#include "util/sprite_loader.h"

typedef struct {
    sg_image texture;
    int frame_count;
    int direction_count;  // 1 or 8
    float frame_time;
    bool loop;
    int row;  // -1 if direction-based
} AnimationClip;

// Entity's animation data
typedef struct {
    AnimationClip clips[8];  // Max 8 different animations per entity
    char clip_names[8][64];
    int clip_count;
    int width, height;  // Frame dimensions
} AnimationSet;

// Current playback state
typedef struct {
    int current_clip;  // Index into AnimationSet
    int current_frame;
    float elapsed;
} AnimationState;

extern ECS_COMPONENT_DECLARE(AnimationClip);
extern ECS_COMPONENT_DECLARE(AnimationSet);
extern ECS_COMPONENT_DECLARE(AnimationState);

void animation_components_register(ecs_world_t *world);
#endif
