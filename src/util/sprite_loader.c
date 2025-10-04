#include "util/sprite_loader.h"
#include <stdlib.h>
#include <string.h>
#include "util/stb_image.h"

sg_image load_texture(char* path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(0);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);

    if (!data) {
        fprintf(stderr, "Failed to load texture: %s\n", path);
        return (sg_image){0};
    }

    sg_image img = sg_make_image(&(sg_image_desc) {
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.subimage[0][0] = {
            .ptr = data,
            .size = (size_t)(width * height * 4)
        }
    });

    stbi_image_free(data);
    return img;
}
// need to initialise the sprite atlas. 
bool sprite_atlas_init(SpriteAtlas* atlas) {
    atlas->capacity = INITIAL_SPRITE_ATLAS_CAPACITY;
    atlas->count = 0;
    atlas->sprites = malloc(atlas->capacity * sizeof(SpriteData));
    return atlas->sprites != NULL;;
}

bool sprite_atlas_load(SpriteAtlas* atlas, const char* path) {
    // read the json file
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open sprite file at path: %s\n", path);
        return false;
    }

    char buffer[1024];
    int len = fread(buffer, 1, sizeof(buffer), fp);
    fclose(fp);

    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "CJSON Error: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        return false;
    }

    // access cJSON data
    cJSON* sprites = NULL;
    cJSON *sprite = NULL;

    sprites = cJSON_GetObjectItemCaseSensitive(json, "sprites");
    cJSON_ArrayForEach(sprite, sprites)
    {
        if (atlas->count >= atlas->capacity) {
            atlas->capacity *= 2;
            atlas->sprites = realloc(atlas->sprites, atlas->capacity * sizeof(SpriteData));
        }

        SpriteData* data = &atlas->sprites[atlas->count];

        cJSON* name = cJSON_GetObjectItemCaseSensitive(sprite, "name");
        if (!cJSON_IsString(name)) {
            goto end;
        }

        cJSON* texture = cJSON_GetObjectItemCaseSensitive(sprite, "texture");
        if (!cJSON_IsString(texture)) {
                    goto end;
        }
        
        cJSON* width = cJSON_GetObjectItemCaseSensitive(sprite, "width");
        cJSON* height = cJSON_GetObjectItemCaseSensitive(sprite, "height");

        if (!cJSON_IsNumber(width) || !cJSON_IsNumber(height)) {
            goto end;
        }

        cJSON* origin_x = cJSON_GetObjectItemCaseSensitive(sprite, "origin_x");
        cJSON* origin_y = cJSON_GetObjectItemCaseSensitive(sprite, "origin_y");

        if (!cJSON_IsNumber(origin_x) || !cJSON_IsNumber(origin_y)) {
                    goto end;
        }
        strncpy(data->name, name->valuestring, sizeof(data->name) - 1);
        data->texture = load_texture(texture->valuestring);
        data->width = width->valueint;
        data->height = height->valueint;
        data->origin_x = origin_x->valueint;
        data->origin_y = origin_y->valueint;

        if (sg_query_image_state(data->texture) == SG_RESOURCESTATE_VALID) {
            atlas->count++;
            printf("Loaded sprite: %s (%dx%d)\n", name->valuestring, width->valueint, height->valueint);
        } else {
            fprintf(stderr, "Failed to create texture for: %s\n", name->valuestring);
        }
    }
end:
    cJSON_Delete(json);    
    return true;
}

SpriteData* sprite_atlas_get(SpriteAtlas* atlas, const char* name) {
    for (int i = 0; i < atlas->count; i++) {
        if (strcmp(atlas->sprites[i].name, name) == 0) {
            return &atlas->sprites[i];
        }
    }

    // this could be a hashmap for o(1) lookup
    return NULL;
}