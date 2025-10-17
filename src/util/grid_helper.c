#include "grid_helper.h"
#include <math.h>
#include "components/transform.h"

int world_to_tile(float pos) {
    int f_pos = (int)floorf(pos / TILE_SIZE);
    return f_pos;
}

static inline uint64_t grid_key(Position pos) {
    int x = world_to_tile(pos.x);
    int y = world_to_tile(pos.y);
    return ((uint64_t) (uint32_t)x << 32) | (uint32_t)y;
}

ecs_entity_t get_entity_at_grid_position(AppState* state, int x, int y) {
    GridEntry* entry = hmgetp_null(state->grid, grid_key((Position) {x, y}));
    return entry ? entry->value : 0;
}

void insert_entity_to_grid(AppState* state, int x, int y, ecs_entity_t entity) {
    hmput(state->grid, grid_key((Position) {x, y}), entity);
}

void delete_entity_from_grid(AppState* state, int x, int y, ecs_entity_t entity) {
    hmdel(state->grid, grid_key((Position) {x, y}));
}

bool does_exist_in_grid(AppState* state, int x, int y) {
    GridEntry* entry =  hmgetp_null(state->grid, grid_key((Position) {x, y}));
    return entry ? true : false;
}

