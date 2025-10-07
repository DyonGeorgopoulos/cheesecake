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
    ecs_set(state->ecs, e, Direction, {.direction = 2});
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