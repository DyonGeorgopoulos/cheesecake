#include "entity_factory.h"

ecs_entity_t entity_factory_spawn_sprite(AppState* state, const char* sprite_name, float x, float y) {
    SpriteEntityData *entity_data = sprite_atlas_get(&state->sprite_atlas, sprite_name);
    if (!entity_data) {
        fprintf(stderr, "Failed to find sprite entity: %s\n", sprite_name);
        return 0;
    }
   
    // Find default animation
    int default_anim_idx = 0;
    if (entity_data->default_animation[0] != '\0') {
        for (int i = 0; i < entity_data->animation_count; i++) {
            if (strcmp(entity_data->animation_names[i], entity_data->default_animation) == 0) {
                default_anim_idx = i;
                break;
            }
        }
    }
    
    AnimationData *anim = &entity_data->animations[default_anim_idx];
    
    // Create entity
    ecs_entity_t e = ecs_new(state->ecs);
   
    // Set position
    ecs_set(state->ecs, e, Position, {x, y});
    
    // NEW: Initialize direction (default to south/down = direction 2)
    ecs_set(state->ecs, e, Direction, {
        .direction = 2  // Start facing south
    });
    
    // Set velocity
    ecs_set(state->ecs, e, Velocity, {
        .x = 0,
        .y = 0
    });
   
    // Determine initial row based on animation type
    int initial_row = 0;
    if (anim->direction_count > 1) {
        // Direction-based animation - use default direction (2 = south)
        initial_row = 2;
    } else if (anim->row >= 0) {
        // Old format with explicit row
        initial_row = anim->row;
    }
    
    // Set sprite component
    ecs_set(state->ecs, e, Sprite, {
        .texture = anim->texture,
        .src_x = 0,  // Start at first frame
        .src_y = initial_row * entity_data->height,
        .src_w = entity_data->width,
        .src_h = entity_data->height,
        .scale_x = entity_data->scale_x,
        .scale_y = entity_data->scale_y,
        .rotation = 0
    });
   
    // Set animation component
    ecs_set(state->ecs, e, SpriteAnimation, {
        .frame_count = anim->frame_count,
        .current_frame = 0,
        .frame_time = anim->frame_time,
        .elapsed = 0,
        .loop = anim->loop
    });
    
    // Copy animation name
    strncpy(ecs_get_mut(state->ecs, e, SpriteAnimation)->anim_name,
            entity_data->animation_names[default_anim_idx], 63);
    
    // Set animation controller
    ecs_set(state->ecs, e, AnimationController, {
        .current_state = {0}
    });
    strncpy(ecs_get_mut(state->ecs, e, AnimationController)->current_state,
            entity_data->default_animation, 31);
   
    // Store reference to entity data
    ecs_set_ptr(state->ecs, e, SpriteEntityRef, &(SpriteEntityRef){
        .entity_data = entity_data
    });
   
    // Set up animation graph if present
    if (entity_data && entity_data->animation_graph) {
        ecs_set_ptr(state->ecs, e, AnimationGraphComponent, &(AnimationGraphComponent){
            .graph = entity_data->animation_graph
        });
    }
   
    return e;
}