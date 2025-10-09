#include "conveyor.h"

ECS_COMPONENT_DECLARE(Conveyor);
ECS_COMPONENT_DECLARE(ConveyorItem);
ECS_COMPONENT_DECLARE(ConveyorTransfer);

void conveyor_components_register(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, Conveyor);
    ECS_COMPONENT_DEFINE(world, ConveyorItem);
    ECS_COMPONENT_DEFINE(world, ConveyorTransfer);
}