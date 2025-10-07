#include "render_system.h"
#include "window.h"
#include <stdio.h>

// move this to an entity
// FPS counter state

fps_counter_t fps_counter = {0};
void fps_counter_update(AppState* state) {
    fps_counter.frame_count++;
    
    // Update FPS display every second (1000ms)
    Uint64 elapsed = state->current_tick - fps_counter.last_fps_update;
    if (elapsed >= 1000) {
        fps_counter.current_fps = (float)fps_counter.frame_count / (elapsed / 1000.0f);
        snprintf(fps_counter.fps_text, sizeof(fps_counter.fps_text), 
                 "FPS: %.0f", fps_counter.current_fps);
        
        fps_counter.frame_count = 0;
        fps_counter.last_fps_update = state->current_tick;
    }
}

bool renderer_initialize(AppState* state) {
    if (!renderer_init(state)) {
        return false;
    }
    
    state->renderer.queries.ground_entities = ecs_query(state->ecs, {
    .terms = {{ ecs_id(Position) }, { ecs_id(Colour) }},
    });

    state->renderer.queries.sprites = ecs_query(state->ecs, {
        .terms = {{ ecs_id(Position) }, { ecs_id(Sprite) }}
    });

    // Updated: AnimationSet + AnimationState instead of SpriteAnimation + SpriteEntityRef
    state->renderer.queries.animations = ecs_query(state->ecs, {
        .terms = {
            { .id = ecs_id(AnimationSet) },
            { .id = ecs_id(AnimationState) },
            { .id = ecs_id(Sprite) },
            { .id = ecs_id(Direction) }
        }
    });

    // Updated: AnimationSet + AnimationState instead of SpriteAnimation
    state->renderer.queries.animation_graphs = ecs_query(state->ecs, {
        .terms = {
            { .id = ecs_id(AnimationSet) },
            { .id = ecs_id(AnimationState) },
            { .id = ecs_id(AnimationGraphComponent) }
        }
    });

    state->renderer.queries.direction = ecs_query(state->ecs, {
        .terms = {
            { .id = ecs_id(Velocity) },
            { .id = ecs_id(Direction) }
        }
    });

    printf("creating shader");
    // Initialise shaders and pipelines here
    state->renderer.sprite_shader = sg_make_shader(sgp_program_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(state->renderer.sprite_shader) != SG_RESOURCESTATE_VALID)
    {
        fprintf(stderr, "failed to make custom pipeline shader\n");
        exit(-1);
    }
    sgp_pipeline_desc pip_desc = {0};
    pip_desc.shader = state->renderer.sprite_shader;
    pip_desc.has_vs_color = true;
    pip_desc.blend_mode = SGP_BLENDMODE_BLEND;
    state->renderer.sprite_pipeline = sgp_make_pipeline(&pip_desc);
    return true;
}

void renderer_shutdown_wrapper(AppState* state) {
    renderer_shutdown(state);
}

void renderer_handle_resize(AppState* state, int width, int height) {
    renderer_resize(state, width, height);
}

sg_swapchain renderer_get_swapchain(AppState* state) {
    return get_swapchain(state);
}

void renderer_draw_frame(void* appstate) {
    AppState* state = (AppState*) appstate;

    int width = state->width;
    int height = state->height;
    renderer_begin_frame(state);
    sg_pass pass = {.swapchain = renderer_get_swapchain(state)};
    sg_begin_pass(&pass);
    sgp_begin(width, height);
    sgp_project(0, (float)width, 0, (float)height);


    // draw background tiles when we start loading them in
    //draw_background_tiles(renderer->queries.background_tiles);
    
    // draw entities on ground when we start tracking the entities
    // draw_ground_entities(renderer->queries.ground_entities);

    // draw sprites / shapes
    //sgp_set_pipeline(state->renderer.sprite_pipeline);
    ecs_iter_t it = ecs_query_iter(state->ecs, state->renderer.queries.ground_entities);
     while (ecs_query_next(&it)) {
        Position *pos = ecs_field(&it, Position, 0);
        Colour *col = ecs_field(&it, Colour, 1);
        
        for (int i = 0; i < it.count; i++) {
            sgp_set_color(col[i].r, col[i].g, col[i].b, col[i].a);
            sgp_draw_filled_rect(pos[i].x, pos[i].y, 50, 50);
        }
    }

    it = ecs_query_iter(state->ecs, state->renderer.queries.sprites);
    sgp_set_blend_mode(SGP_BLENDMODE_BLEND);
     while (ecs_query_next(&it)) {
        Position *pos = ecs_field(&it, Position, 0);
        Sprite* spr = ecs_field(&it, Sprite, 1);
        
        for (int i = 0; i < it.count; i++) {
            sgp_set_color(1.0f, 1.0f, 1.0f, 1.0f);
            sgp_set_image(0, spr[i].texture);
            
            sgp_push_transform();
            sgp_translate(pos[i].x, pos[i].y);
            sgp_rotate(spr[i].rotation);
            printf("Applying scale: %f, %f\n", 5.0f, 5.0f);
            sgp_scale(5.0f, 5.0f);
            
            sgp_rect src = {spr[i].src_x, spr[i].src_y, spr[i].src_w, spr[i].src_h};
            sgp_rect dst = {0, 0, spr[i].src_w, spr[i].src_h};  // Keep this at base size
            sgp_draw_textured_rect(0, dst, src);
            
            sgp_pop_transform();
        }
    }

    // draw text
    // Draw FPS counter in top-left corner
    text_renderer_draw_text(state->renderer.text_renderer, state->font[0], fps_counter.fps_text, 
                            0, 0, 1.0f, (float[4])SG_WHITE, TEXT_ANCHOR_TOP_LEFT);

    text_renderer_draw_text(state->renderer.text_renderer, state->font[1], "WORK IN PROGRESS", 
    width / 2, 0, 1.0f, (float[4])SG_WHITE, TEXT_ANCHOR_TOP_CENTER);

    sgp_flush();
    sgp_end();
    sg_end_pass();
    sg_commit();
    renderer_end_frame(state);
}

void update_animations(AppState *state, float dt) {
    ecs_iter_t it = ecs_query_iter(state->ecs, state->renderer.queries.animations);
   
    while (ecs_query_next(&it)) {
        AnimationSet *anim_set = ecs_field(&it, AnimationSet, 0);
        AnimationState *anim_state = ecs_field(&it, AnimationState, 1);
        Sprite *sprite = ecs_field(&it, Sprite, 2);
        Direction *dir = ecs_field(&it, Direction, 3);
        
        for (int i = 0; i < it.count; i++) {
            AnimationClip *clip = &anim_set[i].clips[anim_state[i].current_clip];
            
            anim_state[i].elapsed += dt;
            
            if (anim_state[i].elapsed >= clip->frame_time) {
                anim_state[i].elapsed -= clip->frame_time;
                anim_state[i].current_frame++;
                
                if (anim_state[i].current_frame >= clip->frame_count) {
                    anim_state[i].current_frame = clip->loop ? 0 : clip->frame_count - 1;
                }
                
                int row = clip->direction_count > 1 ? dir[i].direction : 
                         (clip->row >= 0 ? clip->row : 0);
                
                sprite[i].src_x = anim_state[i].current_frame * anim_set[i].width;
                sprite[i].src_y = row * anim_set[i].height;
            }
        }
    }
}

void set_sprite_animation(ecs_world_t *world, ecs_entity_t entity, const char *anim_name) {
    const AnimationSet *anim_set = ecs_get(world, entity, AnimationSet);
    AnimationState *anim_state = ecs_get_mut(world, entity, AnimationState);
    Sprite *sprite = ecs_get_mut(world, entity, Sprite);
    const Direction *dir = ecs_get(world, entity, Direction);
   
    if (!anim_set || !anim_state || !sprite) {
        fprintf(stderr, "Entity missing required components\n");
        return;
    }
   
    // Find the animation by name
    for (int i = 0; i < anim_set->clip_count; i++) {
        if (strcmp(anim_set->clip_names[i], anim_name) == 0) {
            const AnimationClip *clip = &anim_set->clips[i];
           
            // Determine row based on animation type
            int row = 0;
            if (clip->direction_count > 1) {
                row = dir ? dir->direction : 2;
            } else if (clip->row >= 0) {
                row = clip->row;
            }
           
            // Update sprite texture and source rect IMMEDIATELY
            sprite->texture = clip->texture;
            sprite->src_x = 0;
            sprite->src_y = row * anim_set->height;
           
            // Update animation state
            anim_state->current_clip = i;
            anim_state->current_frame = 0;
            anim_state->elapsed = 0;
           
            printf("Changed animation to: %s (clip %d, row: %d)\n", anim_name, i, row);
            return;
        }
    }
   
    fprintf(stderr, "Animation '%s' not found in entity's AnimationSet\n", anim_name);
}