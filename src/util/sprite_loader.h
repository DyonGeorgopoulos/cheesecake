#ifndef SPRITE_LOADER_H
#define SPRITE_LOADER_H

#include "sokol_gfx.h"
#include <stdbool.h>
#include <cJSON.h>

#define INITIAL_SPRITE_ATLAS_CAPACITY 32

typedef struct {
    sg_image texture;
    int width, height;
    int frame_count;
    float origin_x, origin_y;
    char name[128];
} SpriteData;

typedef struct {
    SpriteData* sprites;
    int count;
    int capacity;
} SpriteAtlas;

bool sprite_atlas_init(SpriteAtlas* atlas);
bool sprite_atlas_load(SpriteAtlas* atlas, const char* path);
SpriteData* sprite_atlas_get(SpriteAtlas* atlas, const char* name); // gets the sprite data
void sprite_atlas_shutdown(SpriteAtlas* atlas);

#endif