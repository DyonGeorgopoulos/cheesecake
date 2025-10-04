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
    sgp_viewport(0, 0, width, height);


    // draw background tiles when we start loading them in
    //draw_background_tiles(renderer->queries.background_tiles);
    
    // draw entities on ground when we start tracking the entities
    // draw_ground_entities(renderer->queries.ground_entities);

    // draw sprites / shapes
    sgp_set_pipeline(state->renderer.sprite_pipeline);
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
     while (ecs_query_next(&it)) {
        Position *pos = ecs_field(&it, Position, 0);
        Sprite* spr = ecs_field(&it, Sprite, 1);
        
        for (int i = 0; i < it.count; i++) {
            //sgp_set_color(col[i].r, col[i].g, col[i].b, col[i].a);
            sgp_set_image(0, spr[i].texture);

            sgp_push_transform();
            sgp_translate(pos[i].x, pos[i].y);
            sgp_rotate(spr[i].rotation);
            sgp_scale(spr[i].scale_x, spr[i].scale_y);
            sgp_rect src = {spr[i].src_x, spr[i].src_y, spr[i].src_w, spr[i].src_h};
            sgp_rect dst = {0, 0, spr[i].src_w, spr[i].src_h};
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