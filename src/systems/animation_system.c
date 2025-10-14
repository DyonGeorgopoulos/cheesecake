#include "animation_system.h"

#include "components/animation_graph.h"
#include <string.h>

#include <math.h>
void AnimationGraphSystem(ecs_iter_t *it) {
    AnimationSet *anim_set = ecs_field(it, AnimationSet, 0);
    AnimationState *anim_state = ecs_field(it, AnimationState, 1);
    AnimationGraphComponent *graph_comp = ecs_field(it, AnimationGraphComponent, 2);
   
    for (int i = 0; i < it->count; i++) {
        if (!graph_comp[i].graph) continue;
       
        AnimationGraph *graph = graph_comp[i].graph;
        
        // Get current animation name from the clip index
        const char *current = anim_set[i].clip_names[anim_state[i].current_clip];
        const AnimationClip *current_clip = &anim_set[i].clips[anim_state[i].current_clip];
       
        AnimationTransition *best = NULL;
        int best_priority = -1;
       
        for (int t = 0; t < graph->transition_count; t++) {
            AnimationTransition *trans = &graph->transitions[t];
           
            // Check if transition applies to current state
            bool from_matches = (strcmp(trans->from, "*") == 0) ||
                               (strcmp(trans->from, current) == 0);
            if (!from_matches) continue;
           
            bool should_transition = false;
           
            // NULL condition = animation_complete
            if (trans->condition == NULL) {
                if (!current_clip->loop &&
                    anim_state[i].current_frame == current_clip->frame_count - 1) {
                    should_transition = true;
                }
            } else {
                should_transition = trans->condition(it->world, it->entities[i]);
            }
           
            if (should_transition && trans->priority > best_priority) {
                best = trans;
                best_priority = trans->priority;
            }
        }
       
        if (best && strcmp(best->to, current) != 0) {
            set_sprite_animation(it->world, it->entities[i], best->to);
        }
    }
}

void UpdateDirectionSystem(ecs_iter_t *it) {
   Velocity *vel = ecs_field(it, Velocity, 0);
    Direction *dir = ecs_field(it, Direction, 1);
    
    // Maps angle index to your sprite sheet row
    static const Direction angle_to_row[8] = {
        DIR_RIGHT,       // 0°   (East)
        DIR_DOWN_RIGHT,  // 45°  (Southeast)
        DIR_DOWN,        // 90°  (South)
        DIR_DOWN_LEFT,   // 135° (Southwest)
        DIR_LEFT,        // 180° (West)
        DIR_UP_LEFT,     // 225° (Northwest)
        DIR_UP,          // 270° (North)
        DIR_UP_RIGHT     // 315° (Northeast)
    };
    
    for (int i = 0; i < it->count; i++) {
        if (vel[i].x == 0 && vel[i].y == 0) {
            continue;
        }
        
        float angle = atan2f(vel[i].y, vel[i].x);
        if (angle < 0) {
            angle += 2.0f * M_PI;
        }
        
        int angle_index = (int)roundf(angle / (M_PI / 4.0f)) % 8;
        dir[i] = angle_to_row[angle_index];
    }
}