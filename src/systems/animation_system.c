#include "animation_system.h"

void AnimationSystem(ecs_iter_t *it) {
    SpriteAnimation *anim = ecs_field(it, SpriteAnimation, 0);
    Sprite *sprite = ecs_field(it, Sprite, 1);
    SpriteEntityRef *entity_ref = ecs_field(it, SpriteEntityRef, 2);
    
    for (int i = 0; i < it->count; i++) {
        anim[i].elapsed += it->delta_time;
        
        // Check if it's time to advance to the next frame
        if (anim[i].elapsed >= anim[i].frame_time) {
            anim[i].elapsed -= anim[i].frame_time;
            anim[i].current_frame++;
            
            // Handle looping or stopping at last frame
            if (anim[i].current_frame >= anim[i].frame_count) {
                if (anim[i].loop) {
                    anim[i].current_frame = 0;
                } else {
                    anim[i].current_frame = anim[i].frame_count - 1;
                }
            }
            
            // Update sprite's source rectangle to show current frame
            // Assumes horizontal sprite sheet layout
            sprite[i].src_x = anim[i].current_frame * entity_ref[i].entity_data->width;
        }
    }
}

void AnimationControllerSystem(ecs_iter_t *it) {
    AnimationController *ctrl = ecs_field(it, AnimationController, 0);
    Velocity *vel = ecs_field(it, Velocity, 1);
    SpriteAnimation *anim = ecs_field(it, SpriteAnimation, 2);
    
    for (int i = 0; i < it->count; i++) {
        // Determine desired animation based on state
        if (vel[i].x > 0) {
            strcpy(ctrl[i].desired_animation, "walk_right");
        } else if (vel[i].x < 0) {
            strcpy(ctrl[i].desired_animation, "walk_left");
        } else {
            strcpy(ctrl[i].desired_animation, "idle");
        }
        
        // Only change if different
        if (strcmp(ctrl[i].desired_animation, anim[i].anim_name) != 0) {
            set_sprite_animation(it->world, it->entities[i], ctrl[i].desired_animation);
        }
    }
}