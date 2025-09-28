#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include "sokol_gfx.h"
#include "sokol_log.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float tex_x, tex_y, tex_w, tex_h;  // Texture coordinates (0-1)
    float offset_x, offset_y;          // Glyph offset from baseline
    float advance_x;                   // How much to advance cursor
    int width, height;                 // Glyph size in pixels
} glyph_info_t;

// Font structure
typedef struct {
    FT_Face face;
    sg_image atlas_texture;
    sg_view atlas_view;
    glyph_info_t glyphs[128];  // ASCII characters 0-127
    int atlas_width, atlas_height;
    int font_size;
    int line_height;
    int ascender, descender;
} font_t;

// Text vertex structure
typedef struct {
    float pos[2];    // Screen position
    float tex[2];    // Texture coordinates
    float color[4];  // RGBA color
} text_vertex_t;

// Text renderer
typedef struct {
    FT_Library ft_library;
    font_t* fonts;
    int font_count;
    int font_capacity;
    
    sg_pipeline pipeline;
    sg_shader shader;
    sg_buffer vertex_buffer;
    sg_buffer index_buffer;
    sg_sampler sampler;
    
    text_vertex_t* vertices;
    uint16_t* indices;
    int max_chars;
} text_renderer_t;

// Function declarations
bool text_renderer_init(text_renderer_t* renderer, int max_chars);
void text_renderer_shutdown(text_renderer_t* renderer);
int text_renderer_load_font(text_renderer_t* renderer, const char* font_path, int size);
void text_renderer_draw_text(text_renderer_t* renderer, int font_id, const char* text, 
                            float x, float y, float scale, float color[4]);
void text_renderer_render(text_renderer_t* renderer, int width, int height);
void text_renderer_get_text_size(text_renderer_t* renderer, int font_id, const char* text, 
                                 float scale, float* width, float* height);

// Utility function for loading shader files
char* load_shader_file(const char* filepath);

#endif
