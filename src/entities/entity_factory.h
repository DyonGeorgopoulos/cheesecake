#ifndef ENTITY_FACTORY_H
#define ENTITY_FACTORY_H


#include <flecs.h>
#include "util/sprite_loader.h"
#include "components/rendering.h"
#include "common.h"


ecs_entity_t entity_factory_spawn_sprite(AppState* state, const char* sprite_name, float x, float y);
#endif