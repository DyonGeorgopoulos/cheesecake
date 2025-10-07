#include "animation_graph.h"

#include "transform.h"
#include <string.h>
#include <stdio.h>

ECS_COMPONENT_DECLARE(AnimationGraphComponent);

void animation_graph_components_register(ecs_world_t *world) {
    ECS_COMPONENT_DEFINE(world, AnimationGraphComponent);
}

// Registry mapping names to functions
typedef struct {
    const char *name;
    AnimationConditionFunc func;
} ConditionRegistry;

// animation_graph.c - simplified conditions
bool is_moving(ecs_world_t *world, ecs_entity_t e) {
    const Velocity *vel = ecs_get(world, e, Velocity);
    return vel && (vel->x != 0 || vel->y != 0);
}

bool is_idle(ecs_world_t *world, ecs_entity_t e) {
    const Velocity *vel = ecs_get(world, e, Velocity);
    return vel && vel->x == 0 && vel->y == 0;
}

// Simplified registry
static ConditionRegistry condition_registry[] = {
    {"is_moving", is_moving},
    {"is_idle", is_idle},
    {"animation_complete", NULL},
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