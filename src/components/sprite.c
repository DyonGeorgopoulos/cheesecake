#include "sprite.h"

ECS_COMPONENT_DECLARE(Sprite);
ECS_COMPONENT_DECLARE(Colour);
ECS_COMPONENT_DECLARE(RenderLayer);

void sprite_components_register(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, Sprite);
    ECS_COMPONENT_DEFINE(world, Colour);
    ECS_COMPONENT_DEFINE(world, RenderLayer);
}