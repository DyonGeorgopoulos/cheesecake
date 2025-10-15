#include "conveyor_system.h"
#include <stdio.h>
#include "util/grid_helper.h"

void conveyor_system_init(ecs_world_t *world)
{
    // ECS_OBSERVER(world, on_conveyor_placed, EcsOnAdd, Conveyor);
    // ECS_OBSERVER(world, on_conveyor_removed, EcsOnDelete, Conveyor);

    // System to update item positions on conveyors
    ECS_SYSTEM(world, update_conveyor_items, EcsOnUpdate,
               Position, ConveyorItem, !ConveyorTransfer);

    //ecs_set_interval(world, update_conveyor_items, 0.10);

    ECS_SYSTEM(world, update_conveyor_item_sprite, EcsOnUpdate,
               Position, ConveyorItem);

    // System to handle transfers between conveyors
    ECS_SYSTEM(world, process_conveyor_transfers, EcsOnUpdate,
               ConveyorItem, ConveyorTransfer, Position);
}

void on_conveyor_placed(ecs_iter_t *it)
{
    Conveyor *conveyors = ecs_field(it, Conveyor, 1); // Only field 1

    for (int i = 0; i < it->count; i++)
    {
        Conveyor *c = &conveyors[i];

        // Initialize lanes
        for (int lane = 0; lane < CONVEYOR_LANES; lane++)
        {
            c->lane_item_count[lane] = 0;
            for (int j = 0; j < MAX_CONVEYER_ITEMS; j++)
            {
                c->lane_items[lane][j] = 0;
            }
        }
    }
}

void on_conveyor_removed(ecs_iter_t *it)
{
    Conveyor *conveyors = ecs_field(it, Conveyor, 1); // Field 2

    for (int i = 0; i < it->count; i++)
    {
        Conveyor *c = &conveyors[i];

        // Delete all items on this conveyor
        for (int lane = 0; lane < CONVEYOR_LANES; lane++)
        {
            for (int j = 0; j < c->lane_item_count[lane]; j++)
            {
                if (c->lane_items[lane][j] != 0)
                {
                    ecs_delete(it->world, c->lane_items[lane][j]);
                }
            }
        }
    }
}

void update_conveyor_items(ecs_iter_t *it)
{
    float dt = it->delta_time;

    for (int i = 0; i < it->count; i++)
    {
        ecs_entity_t e = it->entities[i];
        ConveyorItem *item = ecs_get_mut(it->world, e, ConveyorItem);

        if (!item)
            continue;

        // Validate conveyor entity
        if (item->conveyor == 0 || !ecs_is_alive(it->world, item->conveyor))
        {
            continue;
        }

        const Conveyor *conveyor = ecs_get(it->world, item->conveyor, Conveyor);
        if (!conveyor)
            continue;

        // Check if there's an item ahead blocking us
        float max_progress = 1.0f;
        for (int j = 0; j < conveyor->lane_item_count[item->lane]; j++)
        {
            ecs_entity_t other = conveyor->lane_items[item->lane][j];
            if (other == e)
                continue;

            const ConveyorItem *other_item = ecs_get(it->world, other, ConveyorItem);
            if (other_item && other_item->progress > item->progress)
            {
                float blocked_progress = other_item->progress - ITEM_SPACING;
                if (blocked_progress < max_progress)
                {
                    max_progress = blocked_progress;
                }
                break;
            }
        }

        // Move item forward
        float new_progress = item->progress + (CONVEYOR_SPEED * dt);
        if (new_progress > max_progress)
        {
            new_progress = max_progress;
        }
        item->progress = new_progress;

        // Check if we need to transfer to next conveyor
        if (item->progress >= 1.0f)
        {
            item->progress = 1.0f;
            AppState *state = ecs_get_ctx(it->world);
            
            const Position *conveyor_pos = ecs_get(it->world, item->conveyor, Position);
            if (!conveyor_pos)
                continue;

            // Calculate next position based on direction
            float next_x = conveyor_pos->x;
            float next_y = conveyor_pos->y;

            switch (conveyor->dir)
            {
                case DIR_RIGHT:
                    next_x = conveyor_pos->x + TILE_SIZE;
                    break;
                case DIR_LEFT:
                    next_x = conveyor_pos->x - TILE_SIZE;
                    break;
                case DIR_UP:
                    next_y = conveyor_pos->y - TILE_SIZE;
                    break;
                case DIR_DOWN:
                    next_y = conveyor_pos->y + TILE_SIZE;
                    break;
            }

            // Get next belt entity at the calculated position
            ecs_entity_t next_belt = get_entity_at_grid_position(state, next_x, next_y);
            
            // Only add transfer if next belt exists AND has a Conveyor component
            if (next_belt != 0 && ecs_has(it->world, next_belt, Conveyor))
            {
                const Conveyor *next_conveyor = ecs_get(it->world, next_belt, Conveyor);

                // need to calculate target lane
                // If DIR_RIGHT -> DIR_DOWN target lane is 0 
                // if DIR_RIGHT -> DIR_UP target lane is 1
                Lane target_lane = LANE_LEFT;
                if (conveyor->dir == DIR_RIGHT && next_conveyor->dir == DIR_DOWN) {
                    target_lane = LANE_LEFT;
                }
                else if (conveyor->dir == DIR_RIGHT && next_conveyor->dir == DIR_RIGHT) {
                    target_lane = item->lane;
                }
                else if (conveyor->dir == DIR_LEFT && next_conveyor->dir == DIR_UP) {
                    target_lane = LANE_RIGHT;
                }

                if (conveyor_can_accept_item(it->world, next_belt, item->lane))
                {
                    ecs_set(it->world, e, ConveyorTransfer, {
                        .next_conveyor = next_belt, 
                        .target_lane = target_lane
                    });
                }
            }
        }
    }
}

void update_conveyor_item_sprite(ecs_iter_t *it)
{
    for (int i = 0; i < it->count; i++)
    {
        ecs_entity_t e = it->entities[i];
        Position *pos = ecs_get_mut(it->world, e, Position);
        ConveyorItem *item = ecs_get_mut(it->world, e, ConveyorItem);

        // Calculate visual position based on current progress
        // (No progress updates here!)

        const Conveyor *conveyor = ecs_get(it->world, item->conveyor, Conveyor);
        const Position *conv_pos = ecs_get(it->world, item->conveyor, Position);

        if (!conveyor || !conv_pos)
            continue;

        // Calculate position from progress
        float lane_offset = (item->lane == LANE_LEFT) ? -8.0f : 8.0f;
        float tile_size = 32.0f;

        switch (conveyor->dir)
        {
        case DIR_UP: // North
            pos->x = conv_pos->x + (lane_offset * 2);
            pos->y = conv_pos->y - (item->progress * tile_size - tile_size / 2);
            break;
        case DIR_DOWN: // South
            pos->x = conv_pos->x + lane_offset;
            pos->y = conv_pos->y + (item->progress * tile_size - tile_size / 2);
            break;
        case DIR_RIGHT: // East
            pos->x = conv_pos->x + (item->progress * tile_size - tile_size / 2);
            pos->y = conv_pos->y - lane_offset;
            break;
        case DIR_LEFT: // West
            pos->x = conv_pos->x - (item->progress * tile_size - tile_size / 2);
            pos->y = conv_pos->y + lane_offset;
            break;
        default:
            // For diagonal directions, just use horizontal for now
            pos->x = conv_pos->x + (item->progress * tile_size - tile_size / 2);
            pos->y = conv_pos->y;
            break;
        }
    }
}

void process_conveyor_transfers(ecs_iter_t *it)
{
    for (int i = 0; i < it->count; i++)
    {
        ecs_entity_t e = it->entities[i];

        ConveyorItem *item = ecs_get_mut(it->world, e, ConveyorItem);
        ConveyorTransfer *transfer = ecs_get_mut(it->world, e, ConveyorTransfer);

        if (!item || !transfer)
            continue;

        // Get old and new conveyors
        Conveyor *old_conv = ecs_get_mut(it->world, item->conveyor, Conveyor);
        Conveyor *new_conv = ecs_get_mut(it->world, transfer->next_conveyor, Conveyor);

        if (!old_conv || !new_conv)
        {
            printf("ERROR: Invalid conveyor in transfer\n");
            ecs_remove(it->world, e, ConveyorTransfer);
            continue;
        }

        // Remove from old conveyor's lane tracking
        int old_lane = item->lane;
        bool found = false;
        for (int j = 0; j < old_conv->lane_item_count[old_lane]; j++)
        {
            if (old_conv->lane_items[old_lane][j] == e)
            {
                // Shift remaining items down
                for (int k = j; k < old_conv->lane_item_count[old_lane] - 1; k++)
                {
                    old_conv->lane_items[old_lane][k] = old_conv->lane_items[old_lane][k + 1];
                }
                old_conv->lane_item_count[old_lane]--;
                found = true;
                break;
            }
        }

        if (!found)
        {
            printf("WARNING: Item not found in old conveyor's tracking array\n");
        }

        // Add to new conveyor's lane tracking (at the beginning, index 0)
        int new_lane = transfer->target_lane;
        if (new_conv->lane_item_count[new_lane] < MAX_CONVEYER_ITEMS)
        {
            // Shift existing items forward to make room at index 0
            for (int j = new_conv->lane_item_count[new_lane]; j > 0; j--)
            {
                new_conv->lane_items[new_lane][j] = new_conv->lane_items[new_lane][j - 1];
            }
            new_conv->lane_items[new_lane][0] = e;
            new_conv->lane_item_count[new_lane]++;
        }
        else
        {
            printf("WARNING: New conveyor lane is full, can't transfer item\n");
            ecs_remove(it->world, e, ConveyorTransfer);
            continue;
        }

        printf("Transferred item %llu from conveyor %llu to %llu (lane %d)\n",
            e, item->conveyor, transfer->next_conveyor, new_lane);

        // Update item data
        item->conveyor = transfer->next_conveyor;
        item->lane = transfer->target_lane;
        item->progress = 0.0f;

        // Remove transfer component
        ecs_remove(it->world, e, ConveyorTransfer);
    }
}

bool conveyor_can_accept_item(ecs_world_t *ecs, ecs_entity_t conveyor, Lane lane)
{
    return true;
}

ecs_entity_t conveyor_add_item(ecs_world_t *ecs, ecs_entity_t conveyor, Lane lane, ecs_entity_t entity)
{
    Conveyor *conv = ecs_get_mut(ecs, conveyor, Conveyor);
    if (!conv || !conveyor_can_accept_item(ecs, conveyor, lane))
    {
        printf("Failed to add item: conveyor invalid or can't accept\n");
        return 0;
    }

    ecs_set(ecs, entity, ConveyorItem, {
                                           .conveyor = conveyor,
                                           .lane = lane,
                                           .progress = 0.0f,
                                       });

    // Verify it was set
    const ConveyorItem *check = ecs_get(ecs, entity, ConveyorItem);
    if (check)
    {
        printf("ConveyorItem added successfully: conveyor=%llu, lane=%d\n",
               check->conveyor, check->lane);
    }
    else
    {
        printf("ERROR: Failed to add ConveyorItem component!\n");
    }

    // Add to conveyor's tracking
    conv->lane_items[lane][conv->lane_item_count[lane]++] = entity;
    return entity;
}