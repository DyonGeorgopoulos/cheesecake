#include "entity_factory.h"

ecs_entity_t entity_factory_spawn_sprite(AppState* state, const char* sprite_name, float x, float y) {
    LoadedSpriteData *loaded = sprite_atlas_get(&state->sprite_atlas, sprite_name);
    if (!loaded) {
        fprintf(stderr, "Failed to find sprite: %s\n", sprite_name);
        return 0;
    }
    
    ecs_entity_t e = ecs_new(state->ecs);
    printf("Created entity: %llu\n", e);
    
    AnimationSet anim_set = {0};
    anim_set.width = loaded->width;
    anim_set.height = loaded->height;
    anim_set.clip_count = loaded->clip_count;
    
    for (int i = 0; i < loaded->clip_count; i++) {
        anim_set.clips[i].texture = loaded->clips[i].texture;
        anim_set.clips[i].frame_count = loaded->clips[i].frame_count;
        anim_set.clips[i].direction_count = loaded->clips[i].direction_count;
        anim_set.clips[i].frame_time = loaded->clips[i].frame_time;
        anim_set.clips[i].loop = loaded->clips[i].loop;
        anim_set.clips[i].row = loaded->clips[i].row;
        strncpy(anim_set.clip_names[i], loaded->clip_names[i], 63);
    }

    ecs_set_ptr(state->ecs, e, AnimationSet, &anim_set);

    
    int default_idx = 0;
    for (int i = 0; i < anim_set.clip_count; i++) {
        if (strcmp(anim_set.clip_names[i], loaded->default_animation) == 0) {
            default_idx = i;
            break;
        }
    }
    
    AnimationClip *clip = &anim_set.clips[default_idx];
    int initial_row = clip->direction_count > 1 ? 2 : (clip->row >= 0 ? clip->row : 0);

    ecs_set(state->ecs, e, AnimationState, {
        .current_clip = default_idx,
        .current_frame = 0,
        .elapsed = 0
    });
    
    ecs_set(state->ecs, e, Sprite, {
        .texture = clip->texture,
        .src_x = 0,
        .src_y = initial_row * loaded->height,
        .src_w = loaded->width,
        .src_h = loaded->height,
        .scale_x = loaded->scale_x,
        .scale_y = loaded->scale_y,
        .rotation = 0
    });

    ecs_set(state->ecs, e, Position, {x, y});
    ecs_set(state->ecs, e, Direction, { DIR_RIGHT});
    ecs_set(state->ecs, e, Velocity, {0, 0});
    
    // Copy animation graph if exists
    if (loaded->transitions && loaded->transition_count > 0) {
        AnimationGraph *graph = malloc(sizeof(AnimationGraph));
        graph->transition_count = loaded->transition_count;
        graph->transitions = malloc(loaded->transition_count * sizeof(AnimationTransition));
        
        for (int i = 0; i < loaded->transition_count; i++) {
            AnimationTransition *t = &graph->transitions[i];
            strncpy(t->from, loaded->transitions[i].from, 63);
            strncpy(t->to, loaded->transitions[i].to, 63);
            t->condition = loaded->transitions[i].condition;
            t->priority = loaded->transitions[i].priority;
        }
        
        ecs_set_ptr(state->ecs, e, AnimationGraphComponent, &(AnimationGraphComponent){graph});
    }
    
    printf("Entity spawn complete\n");
    return e;
}

ecs_entity_t entity_factory_spawn_belt(AppState* state, float x, float y, Direction dir) {
    ecs_entity_t belt = entity_factory_spawn_sprite(state, "belt", x, y);
    // need to do an adjacent tiles check and see 
    ecs_set(state->ecs, belt, Conveyor, {
        .dir = dir,
        .lane_items = {0},
        .lane_item_count = {0}
    });

    // to determine if we need a corner belt, we need to look at the surrounding tiles. 
    // for example:
    // [b][x][x]  
    // [x][x][x]
    // [x][x][x]
    // if i place a belt into [0][1], i need to check up, down, right, (ignore origin)
    // if theres a belt there, then we can go one step further and check
    // if the belt direction is facing towards the to be placed belt then we just place the DIR.
    // otherwise we can use the angled dir, 
    // so if the from direction is 
    // switch this to updating a string

    // this is a DIR_RIGHT check
    ecs_entity_t ent = get_entity_at_grid_position(state,  x + 32, y);
    if ( ent != 0) { 
        // check if it is a conveyor & direction is the same
        Conveyor* conv = ecs_get_mut(state->ecs, ent, Conveyor);
        if (conv == NULL) {
            // do nothing
        }
        else if (dir == conv->dir) {
            // leave early
            printf("same dir leaving early\n");
        }
        else if (dir == DIR_RIGHT && conv->dir == DIR_LEFT) {
            printf("found a dir left, while im facing dir_right, so place as normal\n");
        }
        else if (dir == DIR_RIGHT && conv->dir == DIR_DOWN) {
            // now we can set the dir of the next belt to right_down
            conv->dir = DIR_DOWN_RIGHT;
            set_sprite_animation(state->ecs, ent, "down_right");
        }
        else if (dir == DIR_RIGHT && conv->dir == DIR_UP) {
            // now we can set the dir of the next belt to right_down
            // need to get the animation for this one, its WEST->NORTH
            conv->dir = DIR_UP_LEFT;
            set_sprite_animation(state->ecs, ent, "up_left");
        }
    }

    switch (dir) {
        case DIR_RIGHT:
            set_sprite_animation(state->ecs, belt, "right");
            break;
        case DIR_UP:
            set_sprite_animation(state->ecs, belt, "up");
            break;
        case DIR_DOWN:
            set_sprite_animation(state->ecs, belt, "down");
            break;
        case DIR_LEFT:
            set_sprite_animation(state->ecs, belt, "left");
            break;
        case DIR_DOWN_RIGHT:
            set_sprite_animation(state->ecs, belt, "down_right");
            break;
        case DIR_DOWN_LEFT:
            set_sprite_animation(state->ecs, belt, "down_left");
            break;
        case DIR_UP_RIGHT:
            set_sprite_animation(state->ecs, belt, "up_right");
            break;
        case DIR_UP_LEFT:
            set_sprite_animation(state->ecs, belt, "up_left");
            break;
        default:
            break;
    }
    
    insert_entity_to_grid(state, x, y, belt);
    return belt;
}

void entity_factory_spawn_conveyor_item(AppState* state, ecs_entity_t conveyor, Lane lane) {
    const Position* conv_pos = ecs_get(state->ecs, conveyor, Position);
    const Conveyor* conv = ecs_get(state->ecs, conveyor, Conveyor);
    
    if (!conv_pos || !conv) {
        printf("ERROR: Conveyor missing components!\n");
        return;
    }
    
    // Calculate starting position based on conveyor direction and lane
    float start_x = conv_pos->x;
    float start_y = conv_pos->y;
    float tile_size = TILE_SIZE;
    float lane_offset = (lane == LANE_LEFT) ? -8.0f : 8.0f;
    
    switch (conv->dir) {
        case DIR_UP:    // North - start at bottom
            start_x = conv_pos->x + lane_offset;
            start_y = conv_pos->y + tile_size/2;
            break;
        case DIR_DOWN:  // South - start at top
            start_x = conv_pos->x - lane_offset;
            start_y = conv_pos->y - tile_size/2;
            break;
        case DIR_RIGHT: // East - start at left
            start_x = conv_pos->x - tile_size/2;
            start_y = conv_pos->y - lane_offset;
            break;
        case DIR_LEFT:  // West - start at right
            start_x = conv_pos->x + tile_size/2;
            start_y = conv_pos->y + lane_offset;
            break;
        default:
            // For diagonal directions
            start_x = conv_pos->x - tile_size/2;
            start_y = conv_pos->y;
            break;
    }
    
    ecs_entity_t entity = entity_factory_spawn_sprite(state, "iron", start_x, start_y);
    conveyor_add_item(state->ecs, conveyor, lane, entity);
}