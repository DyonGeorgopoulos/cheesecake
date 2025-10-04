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
    atlas->entity_count = 0;
    return true;
}

bool sprite_atlas_load(SpriteAtlas* atlas, const char* path) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open sprite file at path: %s\n", path);
        return false;
    }
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* buffer = malloc(file_size + 1);
    fread(buffer, 1, file_size, fp);
    buffer[file_size] = '\0';
    fclose(fp);
    
    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    
    if (json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "CJSON Error: %s\n", error_ptr);
        }
        return false;
    }
    
    cJSON* sprites = cJSON_GetObjectItemCaseSensitive(json, "sprites");
    if (!cJSON_IsArray(sprites)) {
        fprintf(stderr, "JSON missing 'sprites' array\n");
        cJSON_Delete(json);
        return false;
    }
    
    // Count sprites to allocate
    int sprite_count = cJSON_GetArraySize(sprites);
    atlas->entities = malloc(sprite_count * sizeof(SpriteEntityData));
    atlas->entity_count = 0;
    
    cJSON *sprite = NULL;
    cJSON_ArrayForEach(sprite, sprites) {
        SpriteEntityData *entity = &atlas->entities[atlas->entity_count];
        
        // Parse entity-level properties
        cJSON* name = cJSON_GetObjectItemCaseSensitive(sprite, "name");
        cJSON* width = cJSON_GetObjectItemCaseSensitive(sprite, "width");
        cJSON* height = cJSON_GetObjectItemCaseSensitive(sprite, "height");
        cJSON* origin_x = cJSON_GetObjectItemCaseSensitive(sprite, "origin_x");
        cJSON* origin_y = cJSON_GetObjectItemCaseSensitive(sprite, "origin_y");
        cJSON* scale_x = cJSON_GetObjectItemCaseSensitive(sprite, "scale_x");
        cJSON* scale_y = cJSON_GetObjectItemCaseSensitive(sprite, "scale_y");
        cJSON* default_anim = cJSON_GetObjectItemCaseSensitive(sprite, "default_animation");
        
        if (!cJSON_IsString(name)) {
            fprintf(stderr, "Sprite missing 'name'\n");
            continue;
        }
        
        strncpy(entity->name, name->valuestring, sizeof(entity->name) - 1);
        entity->width = cJSON_IsNumber(width) ? width->valueint : 32;
        entity->height = cJSON_IsNumber(height) ? height->valueint : 32;
        entity->origin_x = cJSON_IsNumber(origin_x) ? origin_x->valuedouble : 16;
        entity->origin_y = cJSON_IsNumber(origin_y) ? origin_y->valuedouble : 16;
        entity->scale_x = cJSON_IsNumber(scale_x) ? scale_x->valuedouble : 1.0f;
        entity->scale_y = cJSON_IsNumber(scale_y) ? scale_y->valuedouble : 1.0f;
        
        if (cJSON_IsString(default_anim)) {
            strncpy(entity->default_animation, default_anim->valuestring, 
                    sizeof(entity->default_animation) - 1);
        } else {
            entity->default_animation[0] = '\0';
        }
        
        // Parse animations
        cJSON* animations = cJSON_GetObjectItemCaseSensitive(sprite, "animations");
        if (!animations) {
            fprintf(stderr, "Sprite '%s' has no animations\n", entity->name);
            continue;
        }
        
        int anim_count = cJSON_GetArraySize(animations);
        entity->animation_count = 0;
        entity->animations = malloc(anim_count * sizeof(AnimationData));
        entity->animation_names = malloc(anim_count * sizeof(char*));
        
        cJSON *anim = animations->child;
        while (anim) {
            const char* anim_name = anim->string;
            
            cJSON* texture = cJSON_GetObjectItemCaseSensitive(anim, "texture");
            cJSON* row = cJSON_GetObjectItemCaseSensitive(anim, "row");
            cJSON* frame_count = cJSON_GetObjectItemCaseSensitive(anim, "frame_count");
            cJSON* frame_time = cJSON_GetObjectItemCaseSensitive(anim, "frame_time");
            cJSON* loop = cJSON_GetObjectItemCaseSensitive(anim, "loop");
            
            if (!cJSON_IsString(texture) || !cJSON_IsNumber(frame_count)) {
                fprintf(stderr, "Invalid animation: %s:%s\n", entity->name, anim_name);
                anim = anim->next;
                continue;
            }
            
            AnimationData *anim_data = &entity->animations[entity->animation_count];
            anim_data->texture = load_texture(texture->valuestring);
            anim_data->row = cJSON_IsNumber(row) ? row->valueint : 0;
            anim_data->frame_count = frame_count->valueint;
            anim_data->frame_time = cJSON_IsNumber(frame_time) ? frame_time->valuedouble : 0.1f;
            anim_data->loop = cJSON_IsBool(loop) ? cJSON_IsTrue(loop) : true;
            
            if (sg_query_image_state(anim_data->texture) != SG_RESOURCESTATE_VALID) {
                fprintf(stderr, "Failed to load texture: %s\n", texture->valuestring);
                anim = anim->next;
                continue;
            }
            
            // Store animation name
            entity->animation_names[entity->animation_count] = malloc(strlen(anim_name) + 1);
            strcpy(entity->animation_names[entity->animation_count], anim_name);
            
            entity->animation_count++;
            printf("Loaded: %s:%s (%d frames)\n", entity->name, anim_name, anim_data->frame_count);
            
            anim = anim->next;
        }
        
        atlas->entity_count++;
    }
    
    cJSON_Delete(json);
    return atlas->entity_count > 0;
}

SpriteEntityData* sprite_atlas_get(SpriteAtlas *atlas, const char *name) {
    for (int i = 0; i < atlas->entity_count; i++) {
        if (strcmp(atlas->entities[i].name, name) == 0) {
            return &atlas->entities[i];
        }
    }
    // this could be a hashmap for o(1) lookup
    return NULL;
}
