#include "rendering_system.h"
#include "sokol_gfx.h"
#include "sokol_gp.h"

void render_sprites(ecs_iter_t* it) {
    Position* pos = ecs_field(it, Position, 0);
    Sprite* spr = ecs_field(it, Sprite, 1);
    Colour* col = ecs_field(it, Colour, 2);

    for (int i=0; i < it->count; i++) {
        sgp_set_color(col[i].r, col[i].g, col[i].b, col[i].a);
        sgp_set_image(0, spr[i].texture);

        sgp_push_transform();
        sgp_translate(pos[i].x, pos[i].y);
        sgp_rotate(spr[i].rotation);
        sgp_scale(spr[i].scale_x, spr[i].scale_y);

        sgp_rect src = { 
            spr[i].src_x,
            spr[i].src_y,
            spr[i].src_w,
            spr[i].src_h
        };

        sgp_rect dst = {
            0,
            0,
            spr[i].src_w, 
            spr[i].src_h
        };

        sgp_draw_textured_rect(0, dst, src);
        sgp_pop_transform();
    }
}

void render_square(ecs_iter_t* it) {
    Position* pos = ecs_field(it, Position, 0);
    Colour* col = ecs_field(it, Colour, 1);

    for (int i=0; i < it->count; i++) {
        sgp_set_color(col[i].r, col[i].g, col[i].b, col[i].a);
        sgp_push_transform();
        sgp_translate(pos[i].x, pos[i].y);

        sgp_draw_filled_rect(
            pos[i].x,
            pos[i].y,
            10,
            10
        );
        sgp_pop_transform();
    }
}