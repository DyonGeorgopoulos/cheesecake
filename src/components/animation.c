#include "animation.h"

ECS_COMPONENT_DECLARE(AnimationClip);
ECS_COMPONENT_DECLARE(AnimationSet);
ECS_COMPONENT_DECLARE(AnimationState);


void animation_components_register(ecs_world_t *world) {
    ECS_COMPONENT_DEFINE(world, AnimationClip);
    ECS_COMPONENT_DEFINE(world, AnimationSet);
    ECS_COMPONENT_DEFINE(world, AnimationState);
}