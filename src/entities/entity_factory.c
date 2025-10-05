#include "entity_factory.h"


ecs_entity_t entity_factory_spawn_sprite(AppState* state, const char* sprite_name, float x, float y) {
    SpriteEntityData *entity_data = sprite_atlas_get(&state->sprite_atlas, sprite_name);
    if (!entity_data) {
        fprintf(stderr, "Failed to find sprite entity: %s\n", sprite_name);
        return 0;
    }
    
    // Determine which animation to use (default or first available)
    int default_anim_idx = 0;
    if (entity_data->default_animation[0] != '\0') {
        // Find the default animation by name
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
    
    // Set sprite component with first frame of default animation
    ecs_set(state->ecs, e, Sprite, {
        .texture = anim->texture,
        .src_x = 16,
        .src_y = anim->row * entity_data->height,
        .src_w = entity_data->width,
        .src_h = entity_data->height,
        .scale_x = 5,
        .scale_y = 5,
        .rotation = 0
    });
    
    // Set animation component
    ecs_set(state->ecs, e, SpriteAnimation, {
        .anim_name = "idle_left",  // Copy default animation name
        .frame_count = anim->frame_count,
        .current_frame = 0,
        .frame_time = anim->frame_time,
        .elapsed = 0,
        .loop = anim->loop
    });

    ecs_set(state->ecs, e, AnimationController, {
        .current_state = entity_data->default_animation 
    });

    ecs_set(state->ecs, e, Velocity, {
        .x = 0,
        .y = 0
    });

    strncpy(ecs_get_mut(state->ecs, e, SpriteAnimation)->anim_name, 
            entity_data->animation_names[default_anim_idx], 63);
    
    // Store pointer to entity data for easy animation switching
    // You'll need a component for this:
    ecs_set_ptr(state->ecs, e, SpriteEntityRef, &(SpriteEntityRef){
        .entity_data = entity_data
    });
    
        if (entity_data && entity_data->animation_graph) {
        ecs_set_ptr(state->ecs, e, AnimationGraphComponent, &(AnimationGraphComponent){
            .graph = entity_data->animation_graph
        });
    }
    
    return e;
}