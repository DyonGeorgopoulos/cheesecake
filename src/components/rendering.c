#include "rendering.h"

ECS_COMPONENT_DECLARE(Sprite);
ECS_COMPONENT_DECLARE(Position);
ECS_COMPONENT_DECLARE(Colour);
ECS_COMPONENT_DECLARE(RenderLayer);
ECS_COMPONENT_DECLARE(SpriteAnimation);
ECS_COMPONENT_DECLARE(UVCoords);
ECS_COMPONENT_DECLARE(RenderOffset);

void rendering_components_register(ecs_world_t *world) {
    ECS_COMPONENT_DEFINE(world, Sprite);
    ECS_COMPONENT_DEFINE(world, Position);
    ECS_COMPONENT_DEFINE(world, Colour);
    ECS_COMPONENT_DEFINE(world, RenderLayer);
    ECS_COMPONENT_DEFINE(world, SpriteAnimation);
    ECS_COMPONENT_DEFINE(world, UVCoords);
    ECS_COMPONENT_DEFINE(world, RenderOffset);
}