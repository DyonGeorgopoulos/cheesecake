#include "util/sprite_loader.h"
#include <stdlib.h>
#include <string.h>
#include "util/stb_image.h"
#include "cJSON.h"

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

bool sprite_atlas_init(SpriteAtlas* atlas) {
    atlas->entity_count = 0;
    atlas->entities = NULL;
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
    
    int sprite_count = cJSON_GetArraySize(sprites);
    atlas->entities = malloc(sprite_count * sizeof(LoadedSpriteData));
    atlas->entity_count = 0;
    
    cJSON *sprite = NULL;
    cJSON_ArrayForEach(sprite, sprites) {
        LoadedSpriteData *entity = &atlas->entities[atlas->entity_count];
        memset(entity, 0, sizeof(LoadedSpriteData));
        
        // Parse entity-level properties
        cJSON* name = cJSON_GetObjectItemCaseSensitive(sprite, "name");
        cJSON* width = cJSON_GetObjectItemCaseSensitive(sprite, "width");
        cJSON* height = cJSON_GetObjectItemCaseSensitive(sprite, "height");
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
        
        entity->clip_count = 0;
        cJSON *anim = animations->child;
        while (anim && entity->clip_count < 16) {
            const char* anim_name = anim->string;
            
            cJSON* texture = cJSON_GetObjectItemCaseSensitive(anim, "texture");
            cJSON* row = cJSON_GetObjectItemCaseSensitive(anim, "row");
            cJSON* frame_count = cJSON_GetObjectItemCaseSensitive(anim, "frame_count");
            cJSON* frame_time = cJSON_GetObjectItemCaseSensitive(anim, "frame_time");
            cJSON* loop = cJSON_GetObjectItemCaseSensitive(anim, "loop");
            cJSON* direction_count = cJSON_GetObjectItemCaseSensitive(anim, "direction_count");
            
            if (!cJSON_IsString(texture) || !cJSON_IsNumber(frame_count)) {
                fprintf(stderr, "Invalid animation: %s:%s\n", entity->name, anim_name);
                anim = anim->next;
                continue;
            }
            
            LoadedAnimationClip *clip = &entity->clips[entity->clip_count];
            clip->texture = load_texture(texture->valuestring);
            clip->frame_count = frame_count->valueint;
            clip->frame_time = cJSON_IsNumber(frame_time) ? frame_time->valuedouble : 0.1f;
            clip->loop = cJSON_IsBool(loop) ? cJSON_IsTrue(loop) : true;
            
            // Handle direction_count
            if (cJSON_IsNumber(direction_count)) {
                clip->direction_count = direction_count->valueint;
                clip->row = -1;
            } else {
                clip->row = cJSON_IsNumber(row) ? row->valueint : 0;
                clip->direction_count = 1;
            }
            
            if (sg_query_image_state(clip->texture) != SG_RESOURCESTATE_VALID) {
                fprintf(stderr, "Failed to load texture: %s\n", texture->valuestring);
                anim = anim->next;
                continue;
            }
            
            strncpy(entity->clip_names[entity->clip_count], anim_name, 63);
            entity->clip_count++;
            
            printf("Loaded: %s:%s (%d frames, %d directions)\n", 
                   entity->name, anim_name, clip->frame_count, clip->direction_count);
            
            anim = anim->next;
        }

        // Parse animation graph
        cJSON *graph_json = cJSON_GetObjectItemCaseSensitive(sprite, "animation_graph");
        if (graph_json) {
            cJSON *transitions_json = cJSON_GetObjectItemCaseSensitive(graph_json, "transitions");
            if (cJSON_IsArray(transitions_json)) {
                int trans_count = cJSON_GetArraySize(transitions_json);
                
                entity->transitions = malloc(trans_count * sizeof(LoadedTransition));
                entity->transition_count = 0;
                
                cJSON *trans = NULL;
                cJSON_ArrayForEach(trans, transitions_json) {
                    cJSON *from = cJSON_GetObjectItemCaseSensitive(trans, "from");
                    cJSON *to = cJSON_GetObjectItemCaseSensitive(trans, "to");
                    cJSON *condition = cJSON_GetObjectItemCaseSensitive(trans, "condition");
                    cJSON *priority = cJSON_GetObjectItemCaseSensitive(trans, "priority");
                    
                    if (cJSON_IsString(from) && cJSON_IsString(to) && cJSON_IsString(condition)) {
                        LoadedTransition *t = &entity->transitions[entity->transition_count];
                        strncpy(t->from, from->valuestring, 63);
                        strncpy(t->to, to->valuestring, 63);
                        t->condition = lookup_condition_function(condition->valuestring);
                        t->priority = cJSON_IsNumber(priority) ? priority->valueint : 1;
                        entity->transition_count++;
                    }
                }
            }
        } else {
            entity->transitions = NULL;
            entity->transition_count = 0;
        }
        
        atlas->entity_count++;
    }
    
    cJSON_Delete(json);
    return atlas->entity_count > 0;
}

LoadedSpriteData* sprite_atlas_get(SpriteAtlas *atlas, const char *name) {
    for (int i = 0; i < atlas->entity_count; i++) {
        if (strcmp(atlas->entities[i].name, name) == 0) {
            return &atlas->entities[i];
        }
    }
    return NULL;
}

void sprite_atlas_free(SpriteAtlas *atlas) {
    for (int i = 0; i < atlas->entity_count; i++) {
        if (atlas->entities[i].transitions) {
            free(atlas->entities[i].transitions);
        }
    }
    free(atlas->entities);
    atlas->entities = NULL;
    atlas->entity_count = 0;
}