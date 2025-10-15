#ifndef CONVEYOR_H
#define CONVEYOR_H


#include <flecs.h>
#include "transform.h"

#define CONVEYOR_LANES 2
#define CONVEYOR_SPEED 0.5f
#define ITEM_SPACING 0.5f // Minimum distance between items
#define MAX_CONVEYER_ITEMS 4

typedef enum { 
    LANE_LEFT = 0,
    LANE_RIGHT = 1
} Lane;

typedef struct {
    Direction dir;
    ecs_entity_t lane_items[CONVEYOR_LANES][MAX_CONVEYER_ITEMS];
    int lane_item_count[CONVEYOR_LANES];
    bool isCorner;
} Conveyor;

typedef struct {
    ecs_entity_t conveyor; // Which conveyor this item is on
    Lane lane;             // Which lane (left or right)
    float progress;        // 0.0 to 1.0 along the conveyor
} ConveyorItem;

// marks item for transfer to next conveyor
typedef struct {
    ecs_entity_t next_conveyor;
    Lane target_lane;
} ConveyorTransfer;

extern ECS_COMPONENT_DECLARE(Conveyor);
extern ECS_COMPONENT_DECLARE(ConveyorItem);
extern ECS_COMPONENT_DECLARE(ConveyorTransfer);

void conveyor_components_register(ecs_world_t* world);

#endif