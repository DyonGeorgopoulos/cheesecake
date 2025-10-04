#include "renderer.h"
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

    state->renderer.queries.animations = ecs_query(state->ecs, {
        .terms = {{ ecs_id(SpriteAnimation)},
                  { ecs_id(Sprite)}, 
                  { ecs_id(SpriteEntityRef)},
                  { ecs_id(AnimationController)},
                  { ecs_id(Velocity)}
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
        SpriteAnimation *anim = ecs_field(&it, SpriteAnimation, 0);
        Sprite *sprite = ecs_field(&it, Sprite, 1);
        SpriteEntityRef *entity_ref = ecs_field(&it, SpriteEntityRef, 2);
        AnimationController  *ctrl = ecs_field(&it, AnimationController , 3);
        Velocity *vel = ecs_field(&it, Velocity, 4);
        
        // this is only for player animations, i'll need to fix for different objects
        //For a future-proof animation system with hitboxes and more complex state, use a data-driven approach where animation metadata lives alongside sprite data:
        // see notes.txt
        for (int i = 0; i < it.count; i++) {
            // Determine desired animation based on state
            if (vel[i].x < 0 && vel[i].y < 0) {
                strcpy(ctrl[i].desired_animation, "walk_up_left");
            } else if (vel[i].x > 0 && vel[i].y < 0) {
                strcpy(ctrl[i].desired_animation, "walk_up_right");
            } else if (vel[i].x > 0 && vel[i].y > 0) {
                strcpy(ctrl[i].desired_animation, "walk_down_right");
            } else if (vel[i].x < 0 && vel[i].y > 0) {
                strcpy(ctrl[i].desired_animation, "walk_down_left");
            } else if (vel[i].x > 0) {
                strcpy(ctrl[i].desired_animation, "walk_right");
            } else if (vel[i].x < 0) {
                strcpy(ctrl[i].desired_animation, "walk_left");
            } else if (vel[i].y > 0) {
                strcpy(ctrl[i].desired_animation, "walk_down");
            } else if (vel[i].y < 0) {
                strcpy(ctrl[i].desired_animation, "walk_up");
            } else {
                strcpy(ctrl[i].desired_animation, "idle_left");
            }
            
            // Only change if different
            if (strcmp(ctrl[i].desired_animation, anim[i].anim_name) != 0) {
                set_sprite_animation(it.world, it.entities[i], ctrl[i].desired_animation);
            }
        }

        for (int i = 0; i < it.count; i++) {
            anim[i].elapsed += dt;
            
            if (anim[i].elapsed >= anim[i].frame_time) {
                printf("Advancing frame: %d -> %d (elapsed: %f, frame_time: %f)\n", 
           anim[i].current_frame, anim[i].current_frame + 1, 
           anim[i].elapsed, anim[i].frame_time);
    
            anim[i].elapsed -= anim[i].frame_time;
            anim[i].current_frame++;
                
                if (anim[i].current_frame >= anim[i].frame_count) {
                    if (anim[i].loop) {
                        anim[i].current_frame = 0;
                    } else {
                        anim[i].current_frame = anim[i].frame_count - 1;
                    }
                }
                
                // Add the 16-pixel offset to frame position
                sprite[i].src_x = anim[i].current_frame * entity_ref[i].entity_data->width;            
            }
            printf("Frame time: %f, elapsed: %f, current_frame: %d\n", 
            anim[i].frame_time, anim[i].elapsed, anim[i].current_frame);
        }
    }
}
void set_sprite_animation(ecs_world_t *world, ecs_entity_t entity, const char *anim_name) {
    const SpriteEntityRef *ref = ecs_get(world, entity, SpriteEntityRef);
    SpriteAnimation *anim_comp = ecs_get_mut(world, entity, SpriteAnimation);
    Sprite *sprite = ecs_get_mut(world, entity, Sprite);
    
    if (!ref || !anim_comp || !sprite) {
        fprintf(stderr, "Entity missing required components\n");
        return;
    }
    
    // Find the animation by name
    for (int i = 0; i < ref->entity_data->animation_count; i++) {
        if (strcmp(ref->entity_data->animation_names[i], anim_name) == 0) {
            AnimationData *anim = &ref->entity_data->animations[i];
            
            // Update sprite texture and source rect
            sprite->texture = anim->texture;
            sprite->src_x = 0;
            sprite->src_y = anim->row * ref->entity_data->height;
            
            // Update animation component
            strncpy(anim_comp->anim_name, anim_name, sizeof(anim_comp->anim_name) - 1);
            anim_comp->frame_count = anim->frame_count;
            anim_comp->current_frame = 0;
            anim_comp->frame_time = anim->frame_time;
            anim_comp->elapsed = 0;
            anim_comp->loop = anim->loop;
            
            printf("Changed animation to: %s\n", anim_name);
            return;
        }
    }
    
    fprintf(stderr, "Animation '%s' not found\n", anim_name);
}