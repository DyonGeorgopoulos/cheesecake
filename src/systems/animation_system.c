#include "animation_system.h"

#include "components/rendering.h"
#include "components/animation_graph.h"
#include <string.h>

void AnimationGraphSystem(ecs_iter_t *it) {
    SpriteAnimation *anim = ecs_field(it, SpriteAnimation, 0);
    AnimationGraphComponent *graph_comp = ecs_field(it, AnimationGraphComponent, 1);
    
    for (int i = 0; i < it->count; i++) {
        if (!graph_comp[i].graph) continue;
        
        AnimationGraph *graph = graph_comp[i].graph;
        const char *current = anim[i].anim_name;
        
        AnimationTransition *best = NULL;
        int best_priority = -1;
        
        for (int t = 0; t < graph->transition_count; t++) {
            AnimationTransition *trans = &graph->transitions[t];
            
            // Check if transition applies
            bool from_matches = (strcmp(trans->from, "*") == 0) || 
                               (strcmp(trans->from, current) == 0);
            if (!from_matches) continue;
            
            bool should_transition = false;
            
            // NULL condition = animation_complete
            if (trans->condition == NULL) {
                if (!anim[i].loop && 
                    anim[i].current_frame == anim[i].frame_count - 1) {
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