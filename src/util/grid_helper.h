#ifndef GRID_HELPER_H
#define GRID_HELPER_H

#include <flecs.h>
#include "common.h"
#include "components/transform.h"
#include "util/stb_ds.h"

ecs_entity_t get_entity_at_grid_position(AppState* state, int x, int y);
void insert_entity_to_grid(AppState* state, int x, int y, ecs_entity_t entity);
void remove_entity_from_grid(AppState* state, int x, int y);
bool does_exist_in_grid(AppState* state, int x, int y);
int world_to_tile(float pos);

#endif