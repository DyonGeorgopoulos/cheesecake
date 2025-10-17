#include "input.h"

ECS_COMPONENT_DECLARE(Input);

void input_components_register(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, Input);
}
