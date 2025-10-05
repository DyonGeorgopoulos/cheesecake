#include "animation_graph.h"
#include "rendering.h"
#include <string.h>
#include <stdio.h>

ECS_COMPONENT_DECLARE(AnimationGraphComponent);

void animation_graph_components_register(ecs_world_t *world) {
    ECS_COMPONENT_DEFINE(world, AnimationGraphComponent);
}

// Define your condition functions
bool is_moving_left(ecs_world_t *world, ecs_entity_t e) {
    const Velocity *vel = ecs_get(world, e, Velocity);
    return vel && vel->x < 0;
}

bool is_moving_right(ecs_world_t *world, ecs_entity_t e) {
    const Velocity *vel = ecs_get(world, e, Velocity);
    return vel && vel->x > 0;
}

bool is_idle(ecs_world_t *world, ecs_entity_t e) {
    const Velocity *vel = ecs_get(world, e, Velocity);
    return vel && vel->x == 0 && vel->y == 0;
}

// Registry mapping names to functions
typedef struct {
    const char *name;
    AnimationConditionFunc func;
} ConditionRegistry;

static ConditionRegistry condition_registry[] = {
    {"is_moving_left", is_moving_left},
    {"is_moving_right", is_moving_right},
    {"is_idle", is_idle},
    {"animation_complete", NULL},  // NULL is special - checked separately
    {NULL, NULL}
};

AnimationConditionFunc lookup_condition_function(const char *name) {
    for (int i = 0; condition_registry[i].name; i++) {
        if (strcmp(condition_registry[i].name, name) == 0) {
            return condition_registry[i].func;
        }
    }
    fprintf(stderr, "Unknown condition: %s\n", name);
    return NULL;
}