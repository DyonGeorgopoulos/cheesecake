#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <flecs.h>

typedef enum {
    DIR_UP_LEFT = 0,      // Row 0 - Northwest
    DIR_LEFT = 1,         // Row 1 - West
    DIR_DOWN_LEFT = 2,    // Row 2 - Southwest
    DIR_DOWN = 3,         // Row 3 - South
    DIR_DOWN_RIGHT = 4,   // Row 4 - Southeast
    DIR_RIGHT = 5,        // Row 5 - East
    DIR_UP_RIGHT = 6,     // Row 6 - Northeast
    DIR_UP = 7            // Row 7 - North
} DirectionEnum;

typedef struct { float x, y; } Position;
typedef struct { float x, y; } Velocity;
typedef struct {
    DirectionEnum direction;
} Direction;

extern ECS_COMPONENT_DECLARE(Velocity);
extern ECS_COMPONENT_DECLARE(Direction);
extern ECS_COMPONENT_DECLARE(Position);


void transform_components_register(ecs_world_t *world);
#endif 