#ifndef ENTITY_FACTORY_H
#define ENTITY_FACTORY_H


#include <flecs.h>
#include "util/sprite_loader.h"
#include "util/grid_helper.h"
#include "components/transform.h"
#include "components/animation.h"
#include "components/sprite.h"
#include "common.h"
#include "systems/conveyor_system.h"

ecs_entity_t entity_factory_spawn_sprite(AppState* state, const char* sprite_name, float x, float y);
ecs_entity_t entity_factory_spawn_belt(AppState* state, float x, float y);
void entity_factory_spawn_conveyor_item(AppState* state, ecs_entity_t conveyor, Lane lane);
#endif