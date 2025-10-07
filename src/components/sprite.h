#ifndef SPRITE_H
#define SPRITE_H

#include <sokol_gfx.h>
#include <flecs.h>

typedef struct {
    sg_image texture;
    float src_x, src_y, src_w, src_h;
    float scale_x, scale_y;
    float rotation;
} Sprite;

typedef struct { float r, g, b, a; } Colour;
typedef struct { int layer; } RenderLayer;

extern ECS_COMPONENT_DECLARE(Sprite);
extern ECS_COMPONENT_DECLARE(Colour);
extern ECS_COMPONENT_DECLARE(RenderLayer);

void sprite_components_register(ecs_world_t *world);
#endif