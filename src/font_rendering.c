#include "font_rendering.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <math.h>
#include "font.shader.glsl.h"

static bool generate_font_atlas(font_t* font) {
    font->atlas_width = 512;
    font->atlas_height = 512;
    
    // Allocate RGBA data (4 bytes per pixel)
    uint8_t* atlas_data = calloc(font->atlas_width * font->atlas_height * 4, 1);
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
        
        // Copy bitmap data to RGBA atlas
        for (unsigned int row = 0; row < bitmap->rows; row++) {
            for (unsigned int col = 0; col < bitmap->width; col++) {
                int atlas_idx = ((y + row) * font->atlas_width + (x + col)) * 4;
                int bitmap_idx = row * bitmap->pitch + col;
                uint8_t alpha = bitmap->buffer[bitmap_idx];
                atlas_data[atlas_idx + 0] = 255;  // R
                atlas_data[atlas_idx + 1] = 255;  // G
                atlas_data[atlas_idx + 2] = 255;  // B
                atlas_data[atlas_idx + 3] = alpha; // A
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
    
    sg_image_desc img_desc = {0};
    img_desc.width = font->atlas_width;
    img_desc.height = font->atlas_height;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.data.subimage[0][0].ptr = atlas_data;
    img_desc.data.subimage[0][0].size = font->atlas_width * font->atlas_height * 4;
    img_desc.label = "font_atlas";

    font->atlas_texture = sg_make_image(&img_desc);
        
    free(atlas_data);
    return sg_query_image_state(font->atlas_texture) == SG_RESOURCESTATE_VALID;
}

bool text_renderer_init(text_renderer_t* renderer, int max_chars) {
    memset(renderer, 0, sizeof(text_renderer_t));
    
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

    renderer->sampler = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .label = "text_sampler"
    });

    // Create custom shader for text rendering
    renderer->text_shader = sg_make_shader(font_program_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(renderer->text_shader) != SG_RESOURCESTATE_VALID) {
        fprintf(stderr, "Failed to create text shader\n");
        sg_destroy_sampler(renderer->sampler);
        free(renderer->fonts);
        FT_Done_FreeType(renderer->ft_library);
        return false;
    }

    // Create custom pipeline using sokol_gp
    sgp_pipeline_desc pip_desc = {0};
    pip_desc.shader = renderer->text_shader;
    pip_desc.has_vs_color = true;
    pip_desc.blend_mode = SGP_BLENDMODE_BLEND;
    renderer->text_pipeline = sgp_make_pipeline(&pip_desc);

    if (sg_query_pipeline_state(renderer->text_pipeline) != SG_RESOURCESTATE_VALID) {
        fprintf(stderr, "Failed to create text pipeline\n");
        sg_destroy_shader(renderer->text_shader);
        sg_destroy_sampler(renderer->sampler);
        free(renderer->fonts);
        FT_Done_FreeType(renderer->ft_library);
        return false;
    }
    
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

void text_renderer_begin(text_renderer_t* renderer) {
    (void)renderer;
}

void text_renderer_draw_text(text_renderer_t* renderer, int font_id, const char* text, 
                            float x, float y, float scale, float color[4], text_anchor_t anchor) {
    if (font_id < 0 || font_id >= renderer->font_count) {
        return;
    }
    
    font_t* font = &renderer->fonts[font_id];
    
    float text_width, text_height;
    text_renderer_get_text_size(renderer, font_id, text, scale, &text_width, &text_height);
    
    float offset_x = 0, offset_y = 0;
    
    switch (anchor) {
        case TEXT_ANCHOR_TOP_LEFT:
            offset_x = 0;
            offset_y = font->ascender * scale;
            break;
        case TEXT_ANCHOR_TOP_CENTER:
            offset_x = -text_width / 2.0f;
            offset_y = font->ascender * scale;
            break;
        case TEXT_ANCHOR_TOP_RIGHT:
            offset_x = -text_width;
            offset_y = font->ascender * scale;
            break;
        case TEXT_ANCHOR_CENTER_LEFT:
            offset_x = 0;
            offset_y = -text_height / 2.0f + font->ascender * scale;
            break;
        case TEXT_ANCHOR_CENTER:
            offset_x = -text_width / 2.0f;
            offset_y = -text_height / 2.0f + font->ascender * scale;
            break;
        case TEXT_ANCHOR_CENTER_RIGHT:
            offset_x = -text_width;
            offset_y = -text_height / 2.0f + font->ascender * scale;
            break;
        case TEXT_ANCHOR_BOTTOM_LEFT:
            offset_x = 0;
            offset_y = -text_height + font->ascender * scale;
            break;
        case TEXT_ANCHOR_BOTTOM_CENTER:
            offset_x = -text_width / 2.0f;
            offset_y = -text_height + font->ascender * scale;
            break;
        case TEXT_ANCHOR_BOTTOM_RIGHT:
            offset_x = -text_width;
            offset_y = -text_height + font->ascender * scale;
            break;
    }
    
    float draw_x = x + offset_x;
    float draw_y = y + offset_y;
    float cursor_x = draw_x;
    float cursor_y = draw_y;
    
    sgp_set_pipeline(renderer->text_pipeline);
    sgp_set_color(color[0], color[1], color[2], color[3]);
    sgp_set_image(0, font->atlas_texture);
    sgp_set_sampler(0, renderer->sampler);
    sgp_set_blend_mode(SGP_BLENDMODE_BLEND);
    
    for (const char* p = text; *p; p++) {
        char c = *p;
        
        if (c == '\n') {
            cursor_x = draw_x;
            cursor_y += font->line_height * scale;
            continue;
        }
        
        if (c < 32 || c >= 127) continue;
        
        glyph_info_t* glyph = &font->glyphs[c];
        
        if (glyph->width == 0 || glyph->height == 0) {
            cursor_x += glyph->advance_x * scale;
            continue;
        }
        
        float x0 = cursor_x + glyph->offset_x * scale;
        float y0 = cursor_y - glyph->offset_y * scale;
        float glyph_width = glyph->width * scale;
        float glyph_height = glyph->height * scale;
        
        sgp_rect dest_rect = {
            .x = x0,
            .y = y0,
            .w = glyph_width,
            .h = glyph_height
        };
        
        sgp_rect src_rect = {
            .x = glyph->tex_x * font->atlas_width,
            .y = glyph->tex_y * font->atlas_height,
            .w = glyph->tex_w * font->atlas_width,
            .h = glyph->tex_h * font->atlas_height
        };
        
        sgp_draw_textured_rect(0, dest_rect, src_rect);
        
        cursor_x += glyph->advance_x * scale;
    }
    
    sgp_reset_pipeline();
    sgp_reset_sampler(0);
    sgp_reset_image(0);
    sgp_reset_blend_mode();
    sgp_reset_color();
}

void text_renderer_render(text_renderer_t* renderer, int width, int height) {
    (void)renderer;
    (void)width;
    (void)height;
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
    
    sg_destroy_pipeline(renderer->text_pipeline);
    sg_destroy_shader(renderer->text_shader);
    sg_destroy_sampler(renderer->sampler);
    
    FT_Done_FreeType(renderer->ft_library);
    
    memset(renderer, 0, sizeof(text_renderer_t));
}
