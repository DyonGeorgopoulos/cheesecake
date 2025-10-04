#ifndef RENDERING_H
#define RENDERING_H
#include <flecs.h>
#include "sokol_gfx.h"
#include "util/sprite_loader.h"

// Basic sprite rendering
typedef struct {
    sg_image texture;        // Texture atlas or individual sprite
    float src_x, src_y;      // Source rect in texture (pixels)
    float src_w, src_h;
    float scale_x, scale_y;  // Scale multiplier
    float rotation;          // Radians
} Sprite;

// Where to draw it
typedef struct {
    float x, y;
} Position, Velocity;

// Visual properties
typedef struct {
    float r, g, b, a;
} Colour;

// Rendering order
typedef struct {
    int layer;              // Lower = drawn first (background)
} RenderLayer;

// Animation state
typedef struct {
    char anim_name[64];
    int frame_count;
    int current_frame;
    float frame_time;       // Time per frame
    float elapsed;
    bool loop;
} SpriteAnimation;

// Optional: UV coordinates if using normalized coords
typedef struct {
    float u0, v0;  // Top-left
    float u1, v1;  // Bottom-right
} UVCoords;

// Optional: offset from position (useful for centering)
typedef struct {
    float x, y;
} RenderOffset;

typedef struct {
    SpriteEntityData *entity_data;
} SpriteEntityRef;

typedef struct {
    char current_state[32];  // "idle", "walking", "jumping"
    char desired_animation[64];
} AnimationController;

extern ECS_COMPONENT_DECLARE(Position);
extern ECS_COMPONENT_DECLARE(Sprite);
extern ECS_COMPONENT_DECLARE(Colour);
extern ECS_COMPONENT_DECLARE(SpriteAnimation);
extern ECS_COMPONENT_DECLARE(RenderLayer);
extern ECS_COMPONENT_DECLARE(UVCoords);
extern ECS_COMPONENT_DECLARE(RenderOffset);
extern ECS_COMPONENT_DECLARE(SpriteEntityRef);
extern ECS_COMPONENT_DECLARE(AnimationController);
extern ECS_COMPONENT_DECLARE(Velocity);

void rendering_components_register(ecs_world_t *world);

#endif