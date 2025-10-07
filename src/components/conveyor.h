#ifndef CONVEYOR_H
#define CONVEYOR_H


#include <flecs.h>
#include "transform.h"

typedef struct {
    ecs_entity_t* held_item;
    Direction dir;
} Conveyor;

#endif