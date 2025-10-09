#include "conveyor_system.h"
#include <stdio.h>

void conveyor_system_init(ecs_world_t* world) {
    //ECS_OBSERVER(world, on_conveyor_placed, EcsOnAdd, Conveyor);
    //ECS_OBSERVER(world, on_conveyor_removed, EcsOnDelete, Conveyor);

    // System to update item positions on conveyors
    ECS_SYSTEM(world, update_conveyor_items, EcsOnUpdate,
           Position, ConveyorItem, !ConveyorTransfer);

    ecs_set_interval(world, update_conveyor_items, 0.6);
    
    // System to handle transfers between conveyors
    // ECS_SYSTEM(world, process_conveyor_transfers, EcsOnUpdate,
    //            ConveyorItem, ConveyorTransfer, Position);

    // ecs_set_interval(world, process_conveyor_transfers, 0.6);
}

void on_conveyor_placed(ecs_iter_t* it) {
    Conveyor* conveyors = ecs_field(it, Conveyor, 1);  // Only field 1
    
    for (int i = 0; i < it->count; i++) {
        Conveyor* c = &conveyors[i];
        
        // Initialize lanes
        for (int lane = 0; lane < CONVEYOR_LANES; lane++) {
            c->lane_item_count[lane] = 0;
            for (int j = 0; j < 16; j++) {
                c->lane_items[lane][j] = 0;
            }
        }
    }
}

void on_conveyor_removed(ecs_iter_t* it) {
    Conveyor* conveyors = ecs_field(it, Conveyor, 1);  // Field 2
    
    
    for (int i = 0; i < it->count; i++) {
        Conveyor* c = &conveyors[i];
        
        // Delete all items on this conveyor
        for (int lane = 0; lane < CONVEYOR_LANES; lane++) {
            for (int j = 0; j < c->lane_item_count[lane]; j++) {
                if (c->lane_items[lane][j] != 0) {
                    ecs_delete(it->world, c->lane_items[lane][j]);
                }
            }
        }
    }
}

void update_conveyor_items(ecs_iter_t* it) {
    float dt = it->delta_time;
    
    for (int i = 0; i < it->count; i++) {
        ecs_entity_t e = it->entities[i];
        
        Position* pos = ecs_get_mut(it->world, e, Position);
        ConveyorItem* item = ecs_get_mut(it->world, e, ConveyorItem);
        
        if (!pos || !item) continue;
        
        // Validate conveyor entity
        if (item->conveyor == 0 || !ecs_is_alive(it->world, item->conveyor)) {
            continue;
        }
        
        const Conveyor* conveyor = ecs_get(it->world, item->conveyor, Conveyor);
        if (!conveyor) continue;
        
        const Position* conv_pos = ecs_get(it->world, item->conveyor, Position);
        if (!conv_pos) continue;
        
        // TODO: Check if there's an item ahead blocking us
        float max_progress = 1.0f;
        for (int j = 0; j < conveyor->lane_item_count[item->lane]; j++) {
            ecs_entity_t other = conveyor->lane_items[item->lane][j];
            if (other == e) continue;
            
            const ConveyorItem* other_item = ecs_get(it->world, other, ConveyorItem);
            if (other_item && other_item->progress > item->progress) {
                float blocked_progress = other_item->progress - ITEM_SPACING;
                if (blocked_progress < max_progress) {
                    max_progress = blocked_progress;
                }
                break;
            }
        }
        
        // Move item forward
        float new_progress = item->progress + (CONVEYOR_SPEED * dt);
        if (new_progress > max_progress) {
            new_progress = max_progress;
        }
        item->progress = new_progress;
        
        // TODO: Check if we need to transfer to next conveyor
        if (item->progress >= 1.0f) {
            // For now just cap it
            item->progress = 1.0f;
        }
        
        // Update visual position based on progress and lane
        float lane_offset = (item->lane == LANE_LEFT) ? -8.0f : 8.0f;
        
        // Calculate position along conveyor based on direction
        // Assuming each conveyor tile is 32x32 pixels (adjust to your tile size)
        float tile_size = 32.0f;
        
        switch (conveyor->dir.direction) {
            case DIR_UP:  // North
                pos->x = conv_pos->x + lane_offset;
                pos->y = conv_pos->y - (item->progress * tile_size - tile_size/2);
                break;
            case DIR_DOWN:  // South
                pos->x = conv_pos->x - lane_offset;
                pos->y = conv_pos->y + (item->progress * tile_size - tile_size/2);
                break;
            case DIR_RIGHT:  // East
                pos->x = conv_pos->x + (item->progress * tile_size - tile_size/2);
                pos->y = conv_pos->y - lane_offset;
                break;
            case DIR_LEFT:  // West
                pos->x = conv_pos->x - (item->progress * tile_size - tile_size/2);
                pos->y = conv_pos->y + lane_offset;
                break;
            default:
                // For diagonal directions, just use horizontal for now
                pos->x = conv_pos->x + (item->progress * tile_size - tile_size/2);
                pos->y = conv_pos->y;
                break;
        }
    }
}

// void update_conveyor_items(ecs_iter_t* it) {
//     ConveyorItem* items = ecs_field(it, ConveyorItem, 1);
//     Position* pos = ecs_field(it, Position, 2);

//     float dt = it->delta_time;

//     for (int i = 0; i < it->count; i++) {
//     ConveyorItem* item = &items[i];  // Get pointer, don't copy
//     Position* position = &pos[i];
    
//     // Debug: Check if the component data looks valid
//     printf("Item %d: conveyor=%llu, lane=%d, progress=%f\n", 
//            i, item->conveyor, item->lane, item->progress);
    
//     // Validate conveyor entity
//     if (item->conveyor == 0) {
//         printf("  ERROR: conveyor is 0!\n");
//         continue;
//     }
    
//     if (!ecs_is_alive(it->world, item->conveyor)) {
//         printf("  ERROR: conveyor entity is not alive!\n");
//         continue;
//     }
    
//     // Now try to get the conveyor component
//     const Conveyor* conveyor = ecs_get(it->world, item->conveyor, Conveyor);
//     if (!conveyor) {
//         printf("  ERROR: conveyor doesn't have Conveyor component!\n");
//         continue;
//     }

//         const Position* conv_pos = ecs_get(it->world, item->conveyor, Position);

//         if (!conv_pos) continue;

//         // check if theres an item ahead blocking
//         float max_progress = 1.0f;
//         // loop each lane
//         for (int j = 0; j < conveyor->lane_item_count[item->lane]; j++) {
//             ecs_entity_t other = conveyor->lane_items[item->lane][j];
//             if (other == it->entities[i]) continue;

//             const ConveyorItem* other_item = ecs_get(it->world, other, ConveyorItem);

//             if (other_item && other_item->progress > item->progress) {
//                 float blocked_progress = other_item->progress - ITEM_SPACING;
//                 if (blocked_progress < max_progress) {
//                     max_progress = blocked_progress;
//                 }
//                 break;
//             }
//         }

//         // move item forward
//         float new_progress = item->progress + (CONVEYOR_SPEED * dt);
//         if (new_progress > max_progress) {
//             new_progress = max_progress;
//         }
//         item->progress = new_progress;

//         //check if need to transfer to next conveyor


//         // update positioon based on progress and lane
//         float lane_offset = (item->lane == LANE_LEFT) ? -0.25f : 0.25f;

//         switch (conveyor->dir.direction) {
//             case DIR_UP:
//                 position->x = conv_pos->x + lane_offset;
//                 position->y = conv_pos->y - 0.5f + item->progress;
//                 break;
//             case DIR_DOWN:
//                 position->x = conv_pos->x - lane_offset;
//                 position->y = conv_pos->y + 0.5f - item->progress;
//                 break;
//             case DIR_RIGHT:
//                 position->x = conv_pos->x - 0.5f + item->progress;
//                 position->y = conv_pos->y - lane_offset;
//                 break;
//             case DIR_LEFT:
//                 position->x = conv_pos->x + 0.5f - item->progress;
//                 position->y = conv_pos->y + lane_offset;
//                 break;
//         }
//     }
// }
void process_conveyor_transfers(ecs_iter_t* it) {

}

bool conveyor_can_accept_item(ecs_world_t* ecs, ecs_entity_t conveyor, Lane lane) {
    return true;
}

ecs_entity_t conveyor_add_item(ecs_world_t* ecs, ecs_entity_t conveyor, Lane lane, ecs_entity_t entity) {
    Conveyor* conv = ecs_get_mut(ecs, conveyor, Conveyor);
    if (!conv || !conveyor_can_accept_item(ecs, conveyor, lane)) {
        printf("Failed to add item: conveyor invalid or can't accept\n");
        return 0;
    }
   
    ecs_set(ecs, entity, ConveyorItem, {
        .conveyor = conveyor,
        .lane = lane,
        .progress = 0.0f,
    });
   
    // Verify it was set
    const ConveyorItem* check = ecs_get(ecs, entity, ConveyorItem);
    if (check) {
        printf("ConveyorItem added successfully: conveyor=%llu, lane=%d\n", 
               check->conveyor, check->lane);
    } else {
        printf("ERROR: Failed to add ConveyorItem component!\n");
    }
   
    // Add to conveyor's tracking
    conv->lane_items[lane][conv->lane_item_count[lane]++] = entity;
    return entity;
}