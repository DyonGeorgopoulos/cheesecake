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
    
    printf("About to set AnimationSet, ID=%llu, valid=%d\n", 
           ecs_id(AnimationSet), ecs_id_is_valid(state->ecs, ecs_id(AnimationSet)));
    ecs_set_ptr(state->ecs, e, AnimationSet, &anim_set);
    printf("AnimationSet OK\n");
    
    int default_idx = 0;
    for (int i = 0; i < anim_set.clip_count; i++) {
        if (strcmp(anim_set.clip_names[i], loaded->default_animation) == 0) {
            default_idx = i;
            break;
        }
    }
    
    AnimationClip *clip = &anim_set.clips[default_idx];
    int initial_row = clip->direction_count > 1 ? 2 : (clip->row >= 0 ? clip->row : 0);
    
    printf("About to set AnimationState, ID=%llu, valid=%d\n", 
           ecs_id(AnimationState), ecs_id_is_valid(state->ecs, ecs_id(AnimationState)));
    ecs_set(state->ecs, e, AnimationState, {
        .current_clip = default_idx,
        .current_frame = 0,
        .elapsed = 0
    });
    printf("AnimationState OK\n");
    
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
    printf("Velocity OK\n");
    
    // Copy animation graph if exists
    if (loaded->transitions && loaded->transition_count > 0) {
        printf("About to create AnimationGraph\n");
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
        
        printf("About to set AnimationGraphComponent, ID=%llu, valid=%d\n", 
               ecs_id(AnimationGraphComponent), ecs_id_is_valid(state->ecs, ecs_id(AnimationGraphComponent)));
        ecs_set_ptr(state->ecs, e, AnimationGraphComponent, &(AnimationGraphComponent){graph});
        printf("AnimationGraphComponent OK\n");
    }
    
    printf("Entity spawn complete\n");
    return e;
}

ecs_entity_t entity_factory_spawn_belt(AppState* state, float x, float y, Direction dir) {
    ecs_entity_t belt = entity_factory_spawn_sprite(state, "belt", x, y);
    ecs_set(state->ecs, belt, Conveyor, {
        .dir = dir,
        .lane_items = {0},
        .lane_item_count = {0}
    });
    // switch this to updating a string
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
    float tile_size = 32.0f;
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