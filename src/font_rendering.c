#include "font_rendering.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <math.h>
#include <dirent.h>  // for directory scanning
#include "font.shader.glsl.h"


bool check_file_exists(const char* filepath) {
    if (!filepath) {
        return false;
    }
    
    if (access(filepath, R_OK) != 0) {
        printf("Font file '%s' is not accessible: %s\n", filepath, strerror(errno));
        return false;
    }
    
    return true;
}

void list_directory(const char* path) {
    printf("Contents of '%s':\n", path);
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("  Cannot open directory: %s\n", strerror(errno));
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            printf("  %s\n", entry->d_name);
        }
    }
    closedir(dir);
    printf("\n");
}

void debug_font_paths() {
    printf("=== DEBUGGING FONT PATHS ===\n");
    
    const char* asset_dirs[] = {
        "/Users/janelle/repos/cheesecake/assets",
        "/Users/janelle/repos/cheesecake/build/assets", 
        "../../../../assets",
        "../../../assets",
        "../../assets",
        "../assets",
        "assets",
        NULL
    };
    
    for (int i = 0; asset_dirs[i] != NULL; i++) {
        list_directory(asset_dirs[i]);
    }
    
    const char* font_paths[] = {
        "/Users/janelle/repos/cheesecake/assets/Roboto-Black.ttf",
        "/Users/janelle/repos/cheesecake/assets/fonts/Roboto-Black.ttf",
        "/Users/janelle/repos/cheesecake/build/assets/Roboto-Black.ttf",
        "/Users/janelle/repos/cheesecake/build/assets/fonts/Roboto-Black.ttf",
        "../../../../assets/Roboto-Black.ttf",
        "../../../../assets/fonts/Roboto-Black.ttf",
        NULL
    };
    
    for (int i = 0; font_paths[i] != NULL; i++) {
        check_file_exists(font_paths[i]);
    }
    
    printf("=== END DEBUG ===\n\n");
}

static bool generate_font_atlas(font_t* font) {
    font->atlas_width = 512;
    font->atlas_height = 512;
    
    uint8_t* atlas_data = calloc(font->atlas_width * font->atlas_height, 1);
    if (!atlas_data) {
        return false;
    }
    
    int x = 1, y = 1;
    int row_height = 0;
    
    for (int c = 32; c < 127; c++) {
        FT_Error error = FT_Load_Char(font->face, c, FT_LOAD_RENDER);
        if (error) {
            printf("Failed to load character %c\n", c);
            continue;
        }
        
        FT_GlyphSlot glyph = font->face->glyph;
        FT_Bitmap* bitmap = &glyph->bitmap;
        
        if (x + bitmap->width + 1 >= font->atlas_width) {
            x = 1;
            y += row_height + 1;
            row_height = 0;
            
            if (y + bitmap->rows >= font->atlas_height) {
                printf("Font atlas too small!\n");
                break;
            }
        }
        
        for (unsigned int row = 0; row < bitmap->rows; row++) {
            for (unsigned int col = 0; col < bitmap->width; col++) {
                int atlas_idx = (y + row) * font->atlas_width + (x + col);
                int bitmap_idx = row * bitmap->pitch + col;
                atlas_data[atlas_idx] = bitmap->buffer[bitmap_idx];
            }
        }
        
        glyph_info_t* glyph_info = &font->glyphs[c];
        glyph_info->tex_x = (float)x / font->atlas_width;
        glyph_info->tex_y = (float)y / font->atlas_height;
        glyph_info->tex_w = (float)bitmap->width / font->atlas_width;
        glyph_info->tex_h = (float)bitmap->rows / font->atlas_height;
        glyph_info->offset_x = glyph->bitmap_left;
        glyph_info->offset_y = glyph->bitmap_top;
        glyph_info->advance_x = glyph->advance.x >> 6;
        glyph_info->width = bitmap->width;
        glyph_info->height = bitmap->rows;
        
        x += bitmap->width + 1;
        row_height = fmax(row_height, bitmap->rows);
    }
    
    sg_image_data img_data = {0};
    size_t atlas_size = font->atlas_width * font->atlas_height;
    img_data.mip_levels[0] = (sg_range){ 
        .ptr = atlas_data, 
        .size = atlas_size 
    };

    font->atlas_texture = sg_make_image(&(sg_image_desc){
        .width = font->atlas_width,
        .height = font->atlas_height,
        .pixel_format = SG_PIXELFORMAT_R8,
        .data = img_data,
        .label = "font_atlas"
    });

    font->atlas_view = sg_make_view(&(sg_view_desc){
        .texture = {
            .image = font->atlas_texture,
            .mip_levels = { .base = 0, .count = 1 },
            .slices = { .base = 0, .count = 1 }
        },
        .label = "font_atlas_view"
    });
    
    free(atlas_data);
    return sg_query_image_state(font->atlas_texture) == SG_RESOURCESTATE_VALID;
}

bool text_renderer_init(text_renderer_t* renderer, int max_chars) {
    memset(renderer, 0, sizeof(text_renderer_t));
    renderer->max_chars = max_chars;
    
    FT_Error error = FT_Init_FreeType(&renderer->ft_library);
    if (error) {
        printf("Failed to initialize FreeType: %d\n", error);
        return false;
    }
    
    renderer->font_capacity = 16;
    renderer->fonts = malloc(renderer->font_capacity * sizeof(font_t));
    if (!renderer->fonts) {
        FT_Done_FreeType(renderer->ft_library);
        return false;
    }
    
    renderer->vertices = malloc(max_chars * 4 * sizeof(text_vertex_t));
    renderer->indices = malloc(max_chars * 6 * sizeof(uint16_t));
    if (!renderer->vertices || !renderer->indices) {
        free(renderer->fonts);
        free(renderer->vertices);
        free(renderer->indices);
        FT_Done_FreeType(renderer->ft_library);
        return false;
    }

    renderer->sampler = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .label = "text_sampler"
    });

    renderer->shader = sg_make_shader(font_program_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(renderer->shader) != SG_RESOURCESTATE_VALID)
    {
        fprintf(stderr, "failed to make custom pipeline shader\n");
        exit(-1);
    }

    renderer->pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = renderer->shader,
        .layout = {
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT2 },
                [1] = { .format = SG_VERTEXFORMAT_FLOAT2 },
                [2] = { .format = SG_VERTEXFORMAT_FLOAT4 }
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .colors[0] = {
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
            }
        },
        .depth = { .write_enabled = false },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "text_pipeline"
    });

    renderer->vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = max_chars * 4 * sizeof(text_vertex_t),
        .usage.dynamic_update = true,
        .usage.vertex_buffer = true,
    });

    renderer->index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = max_chars * 6 * sizeof(uint16_t),
        .usage.index_buffer = true,
        .usage.dynamic_update = true,
    });
    
    printf("Text renderer initialized successfully\n");
    return true;
}

int text_renderer_load_font(text_renderer_t* renderer, const char* font_path, int size) {
    if (renderer->font_count >= renderer->font_capacity) {
        renderer->font_capacity *= 2;
        renderer->fonts = realloc(renderer->fonts, 
                                 renderer->font_capacity * sizeof(font_t));
    }
    
    font_t* font = &renderer->fonts[renderer->font_count];
    memset(font, 0, sizeof(font_t));
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    }
    debug_font_paths();

    FT_Error error = FT_New_Face(renderer->ft_library, font_path, 0, &font->face);
    if (error) {
        printf("Failed to load font '%s': %d\n", font_path, error);
        return -1;
    }
    
    error = FT_Set_Pixel_Sizes(font->face, 0, size);
    if (error) {
        printf("Failed to set font size: %d\n", error);
        FT_Done_Face(font->face);
        return -1;
    }
    
    font->font_size = size;
    font->line_height = font->face->size->metrics.height >> 6;
    font->ascender = font->face->size->metrics.ascender >> 6;
    font->descender = font->face->size->metrics.descender >> 6;
    
    if (!generate_font_atlas(font)) {
        printf("Failed to generate font atlas\n");
        FT_Done_Face(font->face);
        return -1;
    }
    
    printf("Loaded font '%s' (size %d, line height %d)\n", 
           font_path, size, font->line_height);
    
    return renderer->font_count++;
}

// NEW: Begin collecting text draws for the frame
void text_renderer_begin(text_renderer_t* renderer) {
    renderer->vertex_count = 0;
    renderer->index_count = 0;
}

// MODIFIED: Just add to buffers, don't draw yet
void text_renderer_draw_text(text_renderer_t* renderer, int font_id, const char* text, 
                            float x, float y, float scale, float color[4]) {
    if (font_id < 0 || font_id >= renderer->font_count) {
        return;
    }
    
    font_t* font = &renderer->fonts[font_id];
    float cursor_x = x;
    float cursor_y = y;
    
    for (const char* p = text; *p; p++) {
        char c = *p;
        
        if (c == '\n') {
            cursor_x = x;
            cursor_y += font->line_height * scale;
            continue;
        }
        
        if (c < 32 || c >= 127) continue;
        
        if (renderer->vertex_count + 4 > renderer->max_chars * 4) break;
        
        glyph_info_t* glyph = &font->glyphs[c];
        
        if (glyph->width == 0 || glyph->height == 0) {
            cursor_x += glyph->advance_x * scale;
            continue;
        }
        
        float x0 = cursor_x + glyph->offset_x * scale;
        float y0 = cursor_y - glyph->offset_y * scale;
        float x1 = x0 + glyph->width * scale;
        float y1 = y0 + glyph->height * scale;
        
        text_vertex_t* verts = &renderer->vertices[renderer->vertex_count];
        
        verts[0].pos[0] = x0; verts[0].pos[1] = y0;
        verts[0].tex[0] = glyph->tex_x; verts[0].tex[1] = glyph->tex_y;
        memcpy(verts[0].color, color, 4 * sizeof(float));
        
        verts[1].pos[0] = x1; verts[1].pos[1] = y0;
        verts[1].tex[0] = glyph->tex_x + glyph->tex_w; verts[1].tex[1] = glyph->tex_y;
        memcpy(verts[1].color, color, 4 * sizeof(float));
        
        verts[2].pos[0] = x1; verts[2].pos[1] = y1;
        verts[2].tex[0] = glyph->tex_x + glyph->tex_w; verts[2].tex[1] = glyph->tex_y + glyph->tex_h;
        memcpy(verts[2].color, color, 4 * sizeof(float));
        
        verts[3].pos[0] = x0; verts[3].pos[1] = y1;
        verts[3].tex[0] = glyph->tex_x; verts[3].tex[1] = glyph->tex_y + glyph->tex_h;
        memcpy(verts[3].color, color, 4 * sizeof(float));
        
        uint16_t base = renderer->vertex_count;
        renderer->indices[renderer->index_count++] = base + 0;
        renderer->indices[renderer->index_count++] = base + 1;
        renderer->indices[renderer->index_count++] = base + 2;
        renderer->indices[renderer->index_count++] = base + 0;
        renderer->indices[renderer->index_count++] = base + 2;
        renderer->indices[renderer->index_count++] = base + 3;
        
        renderer->vertex_count += 4;
        cursor_x += glyph->advance_x * scale;
    }
}

static void create_ortho_matrix(float* matrix, float left, float right, float bottom, float top) {
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -1.0f;
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[15] = 1.0f;
}

// MODIFIED: Now actually renders everything at once
void text_renderer_render(text_renderer_t* renderer, int width, int height) {
    if (renderer->vertex_count == 0) {
        return;  // Nothing to draw
    }
    
    // Update buffers ONCE per frame
    sg_update_buffer(renderer->vertex_buffer, &(sg_range){
        .ptr = renderer->vertices,
        .size = renderer->vertex_count * sizeof(text_vertex_t)
    });
    
    sg_update_buffer(renderer->index_buffer, &(sg_range){
        .ptr = renderer->indices,
        .size = renderer->index_count * sizeof(uint16_t)
    });
    
    // Create orthographic projection
    float mvp[16];
    create_ortho_matrix(mvp, 0, width, height, 0);
    
    sg_apply_pipeline(renderer->pipeline);
    sg_apply_uniforms(0, &(sg_range){mvp, 64});
    
    // Use the first font's atlas (you might want to make this more flexible)
    if (renderer->font_count > 0) {
        sg_bindings bindings = {
            .vertex_buffers[0] = renderer->vertex_buffer,
            .index_buffer = renderer->index_buffer,
            .views[0] = renderer->fonts[0].atlas_view,
            .samplers[0] = renderer->sampler
        };
        
        sg_apply_bindings(&bindings);
        sg_draw(0, renderer->index_count, 1);
    }
}

void text_renderer_get_text_size(text_renderer_t* renderer, int font_id, const char* text, 
                                 float scale, float* width, float* height) {
    *width = 0;
    *height = 0;
    
    if (font_id < 0 || font_id >= renderer->font_count) {
        return;
    }
    
    font_t* font = &renderer->fonts[font_id];
    float cursor_x = 0;
    float max_width = 0;
    int line_count = 1;
    
    for (const char* p = text; *p; p++) {
        char c = *p;
        
        if (c == '\n') {
            max_width = fmax(max_width, cursor_x);
            cursor_x = 0;
            line_count++;
            continue;
        }
        
        if (c >= 32 && c < 127) {
            cursor_x += font->glyphs[c].advance_x * scale;
        }
    }
    
    *width = fmax(max_width, cursor_x);
    *height = font->line_height * scale * line_count;
}

void text_renderer_shutdown(text_renderer_t* renderer) {
    for (int i = 0; i < renderer->font_count; i++) {
        FT_Done_Face(renderer->fonts[i].face);
        sg_destroy_image(renderer->fonts[i].atlas_texture);
    }
    free(renderer->fonts);
    
    FT_Done_FreeType(renderer->ft_library);
    
    sg_destroy_buffer(renderer->vertex_buffer);
    sg_destroy_buffer(renderer->index_buffer);
    
    free(renderer->vertices);
    free(renderer->indices);
    
    memset(renderer, 0, sizeof(text_renderer_t));
}
