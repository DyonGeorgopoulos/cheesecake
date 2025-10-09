#ifndef CONVEYOR_SYSTEM_H
#define CONVEYOR_SYSTEM_H

#include <flecs.h>
#include "components/conveyor.h"

void conveyor_system_init(ecs_world_t* world);
void on_conveyor_placed(ecs_iter_t* it);
void on_conveyor_removed(ecs_iter_t* it);
void update_conveyor_items(ecs_iter_t* it);
void process_conveyor_transfers(ecs_iter_t* it);

// Helper functions
ecs_entity_t conveyor_get_next(ecs_world_t* ecs, ecs_entity_t conveyor);
bool conveyor_can_accept_item(ecs_world_t* ecs, ecs_entity_t conveyor, Lane lane);
ecs_entity_t conveyor_add_item(ecs_world_t* ecs, ecs_entity_t conveyor, Lane lane, ecs_entity_t entity);
float conveyor_get_lane_head_progress(ecs_world_t* ecs, ecs_entity_t conveyor, Lane lane);

#endif