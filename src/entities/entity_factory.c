#include "entity_factory.h"


ecs_entity_t entity_factory_spawn_sprite(AppState* state, const char* sprite_name, float x, float y) {

    SpriteData *data = sprite_atlas_get(&state->sprite_atlas, sprite_name);

    ecs_entity_t e = ecs_new(state->ecs);
    ecs_set(state->ecs, e, Position, {x, y});
    ecs_set(state->ecs, e, Sprite, {
        .texture = data->texture,
        .src_x = 16, .src_y = 16, 
        .scale_x = 1.0f, .scale_y = 1.0f,
        .rotation = 0
    });
    //ecs_set(state->ecs, e, Colour, {.r = 1.0f, .g = 0.4f, .b = 0.2f, .a = 1.0f});

    return e;
}