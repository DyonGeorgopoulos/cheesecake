#ifndef ANIMATION_GRAPH_H
#define ANIMATION_GRAPH_H

#include <flecs.h>
#include <stdbool.h>

// Condition function type
typedef bool (*AnimationConditionFunc)(ecs_world_t*, ecs_entity_t);

typedef struct {
    char from[64];
    char to[64];
    AnimationConditionFunc condition;
    int priority;
} AnimationTransition;

typedef struct {
    AnimationTransition *transitions;
    int transition_count;
} AnimationGraph;

typedef struct {
    AnimationGraph *graph;
} AnimationGraphComponent;

extern ECS_COMPONENT_DECLARE(AnimationGraphComponent);

void animation_graph_components_register(ecs_world_t *world);
AnimationConditionFunc lookup_condition_function(const char *name);

#endif