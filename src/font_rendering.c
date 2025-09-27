#include "font_rendering.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <math.h>
#include <dirent.h>  // for directory scanning



bool check_file_exists(const char* filepath) {
    if (!filepath) {
        return false;
    }
    
    // Check if file exists and is readable
    if (access(filepath, R_OK) != 0) {
        printf("Font file '%s' is not accessible: %s\n", filepath, strerror(errno));
        return false;
    }
    
    return true;
}

// Function to list directory contents
void list_directory(const char* path) {
    printf("Contents of '%s':\n", path);
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("  Cannot open directory: %s\n", strerror(errno));
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {  // Skip hidden files
            printf("  %s\n", entry->d_name);
        }
    }
    closedir(dir);
    printf("\n");
}

// Add this to your main function for debugging
void debug_font_paths() {
    printf("=== DEBUGGING FONT PATHS ===\n");
    
    // Check if the assets directory exists in various locations
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
    
    // Check specific font file locations
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

// Generate font atlas
static bool generate_font_atlas(font_t* font) {
    // Calculate atlas size (simple approach)
    font->atlas_width = 512;
    font->atlas_height = 512;
    
    uint8_t* atlas_data = calloc(font->atlas_width * font->atlas_height, 1);
    if (!atlas_data) {
        return false;
    }
    
    int x = 1, y = 1;  // Start with 1px border
    int row_height = 0;
    
    // Render each ASCII character
    for (int c = 32; c < 127; c++) {
        FT_Error error = FT_Load_Char(font->face, c, FT_LOAD_RENDER);
        if (error) {
            printf("Failed to load character %c\n", c);
            continue;
        }
        
        FT_GlyphSlot glyph = font->face->glyph;
        FT_Bitmap* bitmap = &glyph->bitmap;
        
        // Check if we need to move to next row
        if (x + bitmap->width + 1 >= font->atlas_width) {
            x = 1;
            y += row_height + 1;
            row_height = 0;
            
            if (y + bitmap->rows >= font->atlas_height) {
                printf("Font atlas too small!\n");
                break;
            }
        }
        
        // Copy glyph bitmap to atlas
        for (unsigned int row = 0; row < bitmap->rows; row++) {
            for (unsigned int col = 0; col < bitmap->width; col++) {
                int atlas_idx = (y + row) * font->atlas_width + (x + col);
                int bitmap_idx = row * bitmap->pitch + col;
                atlas_data[atlas_idx] = bitmap->buffer[bitmap_idx];
            }
        }
        
        // Store glyph info
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

    // Create a view for the texture
    // Create a view for the texture
    font->atlas_view = sg_make_view(&(sg_view_desc){
        .texture = {
            .image = font->atlas_texture,
            .mip_levels = { .base = 0, .count = 1 },  // Use only the base mip level
            .slices = { .base = 0, .count = 1 }       // Single 2D texture slice
        },
        .label = "font_atlas_view"
    });
    
    free(atlas_data);
    return sg_query_image_state(font->atlas_texture) == SG_RESOURCESTATE_VALID;
}

// Initialize text renderer
bool text_renderer_init(text_renderer_t* renderer, int max_chars) {
    memset(renderer, 0, sizeof(text_renderer_t));
    renderer->max_chars = max_chars;
    
    // Initialize FreeType
    FT_Error error = FT_Init_FreeType(&renderer->ft_library);
    if (error) {
        printf("Failed to initialize FreeType: %d\n", error);
        return false;
    }
    
    // Allocate font array
    renderer->font_capacity = 16;
    renderer->fonts = malloc(renderer->font_capacity * sizeof(font_t));
    if (!renderer->fonts) {
        FT_Done_FreeType(renderer->ft_library);
        return false;
    }
    
    // Allocate vertex and index arrays
    renderer->vertices = malloc(max_chars * 4 * sizeof(text_vertex_t));
    renderer->indices = malloc(max_chars * 6 * sizeof(uint16_t));
    if (!renderer->vertices || !renderer->indices) {
        free(renderer->fonts);
        free(renderer->vertices);
        free(renderer->indices);
        FT_Done_FreeType(renderer->ft_library);
        return false;
    }
   
    const char* vs_source = 
    "#version 330\n"
    "layout(location=0) in vec2 pos;\n"
    "layout(location=1) in vec2 tex;\n"
    "layout(location=2) in vec4 color;\n"
    "uniform mat4 mvp;\n"
    "out vec2 uv;\n"
    "out vec4 vert_color;\n"
    "void main() {\n"
    "  gl_Position = mvp * vec4(pos, 0.0, 1.0);\n"
    "  uv = tex;\n"
    "  vert_color = color;\n"
    "}\n";

    const char* fs_source = 
        "#version 330\n"
        "in vec2 uv;\n"
        "in vec4 vert_color;\n"
        "uniform sampler2D u_texture;\n"
        "out vec4 frag_color;\n"
        "void main() {\n"
        "  float alpha = texture(u_texture, uv).r;\n"
        "  frag_color = vec4(vert_color.rgb, vert_color.a * alpha);\n"
        "}\n";

    renderer->sampler = sg_make_sampler(&(sg_sampler_desc){
    .min_filter = SG_FILTER_LINEAR,
    .mag_filter = SG_FILTER_LINEAR,
    .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
    .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    .label = "text_sampler"
});
renderer->shader = sg_make_shader(&(sg_shader_desc){
    .vertex_func = {
        .source = vs_source,
        .entry = "main"
    },
    .fragment_func = {
        .source = fs_source,
        .entry = "main"
    },
    .attrs[0] = { 
        .base_type = SG_SHADERATTRBASETYPE_FLOAT,
        .glsl_name = "pos"
    },
    .attrs[1] = { 
        .base_type = SG_SHADERATTRBASETYPE_FLOAT,
        .glsl_name = "tex"
    },
    .attrs[2] = { 
        .base_type = SG_SHADERATTRBASETYPE_FLOAT,
        .glsl_name = "color"
    },
    .uniform_blocks[0] = {
        .stage = SG_SHADERSTAGE_VERTEX,
        .layout = SG_UNIFORMLAYOUT_NATIVE,
        .size = 64,
        .glsl_uniforms[0] = { 
            .type = SG_UNIFORMTYPE_MAT4, 
            .array_count = 1,
            .glsl_name = "mvp"
        }
    },
    .views[0] = { 
        .texture = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .image_type = SG_IMAGETYPE_2D,
            .sample_type = SG_IMAGESAMPLETYPE_FLOAT,
            .multisampled = false
        }
    },
    .samplers[0] = { 
        .stage = SG_SHADERSTAGE_FRAGMENT,
        .sampler_type = SG_SAMPLERTYPE_FILTERING
    },
    .texture_sampler_pairs[0] = { 
        .stage = SG_SHADERSTAGE_FRAGMENT,
        .view_slot = 0, 
        .sampler_slot = 0,
        .glsl_name = "u_texture"
    },
    .label = "text_shader"
});

// Create pipeline for text rendering with INDEX BUFFER support
renderer->pipeline = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = renderer->shader,
    .layout = {
        .attrs = {
            [0] = { .format = SG_VERTEXFORMAT_FLOAT2 },  // pos
            [1] = { .format = SG_VERTEXFORMAT_FLOAT2 },  // tex
            [2] = { .format = SG_VERTEXFORMAT_FLOAT4 }   // color
        }
    },
    .index_type = SG_INDEXTYPE_UINT16,  // IMPORTANT: Enable index buffer
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

    // Create buffers - LATEST API
    // Create buffers - CORRECTED FOR LATEST API
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

// Load font
int text_renderer_load_font(text_renderer_t* renderer, const char* font_path, int size) {
    if (renderer->font_count >= renderer->font_capacity) {
        // Resize array
        renderer->font_capacity *= 2;
        renderer->fonts = realloc(renderer->fonts, 
                                 renderer->font_capacity * sizeof(font_t));
    }
    
    font_t* font = &renderer->fonts[renderer->font_count];
    memset(font, 0, sizeof(font_t));
    
    // Load font face
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    }
    debug_font_paths();  // Call the debug function to list directories and check files

    FT_Error error = FT_New_Face(renderer->ft_library, font_path, 0, &font->face);
    if (error) {
        printf("Failed to load font '%s': %d\n", font_path, error);
        return -1;
    }
    
    // Set pixel size
    error = FT_Set_Pixel_Sizes(font->face, 0, size);
    if (error) {
        printf("Failed to set font size: %d\n", error);
        FT_Done_Face(font->face);
        return -1;
    }
    
    // Store font metrics
    font->font_size = size;
    font->line_height = font->face->size->metrics.height >> 6;
    font->ascender = font->face->size->metrics.ascender >> 6;
    font->descender = font->face->size->metrics.descender >> 6;
    
    // Generate atlas
    if (!generate_font_atlas(font)) {
        printf("Failed to generate font atlas\n");
        FT_Done_Face(font->face);
        return -1;
    }
    
    printf("Loaded font '%s' (size %d, line height %d)\n", 
           font_path, size, font->line_height);
    
    return renderer->font_count++;
}

// Draw text (adds to vertex buffer)
void text_renderer_draw_text(text_renderer_t* renderer, int font_id, const char* text, 
                            float x, float y, float scale, float color[4]) {
    if (font_id < 0 || font_id >= renderer->font_count) {
        return;
    }
    
    font_t* font = &renderer->fonts[font_id];
    float cursor_x = x;
    float cursor_y = y;
    
    int vertex_count = 0;
    int index_count = 0;
    
    for (const char* p = text; *p; p++) {
        char c = *p;
        
        if (c == '\n') {
            cursor_x = x;
            cursor_y += font->line_height * scale;
            continue;
        }
        
        if (c < 32 || c >= 127) continue;  // Skip non-printable
        
        if (vertex_count + 4 > renderer->max_chars * 4) break;  // Buffer full
        
        glyph_info_t* glyph = &font->glyphs[c];
        
        if (glyph->width == 0 || glyph->height == 0) {
            cursor_x += glyph->advance_x * scale;
            continue;  // Skip whitespace
        }
        
        // Calculate quad positions
        float x0 = cursor_x + glyph->offset_x * scale;
        float y0 = cursor_y - glyph->offset_y * scale;
        float x1 = x0 + glyph->width * scale;
        float y1 = y0 + glyph->height * scale;
        
        // Create quad vertices
        text_vertex_t* verts = &renderer->vertices[vertex_count];
        
        // Top-left
        verts[0].pos[0] = x0; verts[0].pos[1] = y0;
        verts[0].tex[0] = glyph->tex_x; verts[0].tex[1] = glyph->tex_y;
        memcpy(verts[0].color, color, 4 * sizeof(float));
        
        // Top-right
        verts[1].pos[0] = x1; verts[1].pos[1] = y0;
        verts[1].tex[0] = glyph->tex_x + glyph->tex_w; verts[1].tex[1] = glyph->tex_y;
        memcpy(verts[1].color, color, 4 * sizeof(float));
        
        // Bottom-right
        verts[2].pos[0] = x1; verts[2].pos[1] = y1;
        verts[2].tex[0] = glyph->tex_x + glyph->tex_w; verts[2].tex[1] = glyph->tex_y + glyph->tex_h;
        memcpy(verts[2].color, color, 4 * sizeof(float));
        
        // Bottom-left
        verts[3].pos[0] = x0; verts[3].pos[1] = y1;
        verts[3].tex[0] = glyph->tex_x; verts[3].tex[1] = glyph->tex_y + glyph->tex_h;
        memcpy(verts[3].color, color, 4 * sizeof(float));
        
        // Create quad indices (two triangles)
        uint16_t base = vertex_count;
        renderer->indices[index_count++] = base + 0;
        renderer->indices[index_count++] = base + 1;
        renderer->indices[index_count++] = base + 2;
        renderer->indices[index_count++] = base + 0;
        renderer->indices[index_count++] = base + 2;
        renderer->indices[index_count++] = base + 3;
        
        vertex_count += 4;
        cursor_x += glyph->advance_x * scale;
    }
    
    if (vertex_count > 0) {
        // Update buffers
        sg_update_buffer(renderer->vertex_buffer, &(sg_range){
            .ptr = renderer->vertices,
            .size = vertex_count * sizeof(text_vertex_t)
        });
        
        sg_update_buffer(renderer->index_buffer, &(sg_range){
            .ptr = renderer->indices,
            .size = index_count * sizeof(uint16_t)
        });
        
    // Draw

    sg_bindings bindings = {
    .vertex_buffers[0] = renderer->vertex_buffer,
    .index_buffer = renderer->index_buffer,
    .views[0] = font->atlas_view,
    .samplers[0] = renderer->sampler
    };
        
        sg_apply_bindings(&bindings);
        sg_draw(0, index_count, 1);
    }
}

// Helper function to create orthographic projection matrix
static void create_ortho_matrix(float* matrix, float left, float right, float bottom, float top) {
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -1.0f;
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[15] = 1.0f;
}

// Render all text (call this after adding all text for the frame)
void text_renderer_render(text_renderer_t* renderer, int width, int height) {
    // Create orthographic projection matrix
    float mvp[16];
    create_ortho_matrix(mvp, 0, width, height, 0);  // Top-left origin
    
    sg_apply_pipeline(renderer->pipeline);
    sg_apply_uniforms(0, &(sg_range){mvp, 64});
}

// Get text dimensions
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

// Shutdown
void text_renderer_shutdown(text_renderer_t* renderer) {
    // Clean up fonts
    for (int i = 0; i < renderer->font_count; i++) {
        FT_Done_Face(renderer->fonts[i].face);
        sg_destroy_image(renderer->fonts[i].atlas_texture);
    }
    free(renderer->fonts);
    
    // Clean up FreeType
    FT_Done_FreeType(renderer->ft_library);
    
    // Clean up Sokol resources
    sg_destroy_buffer(renderer->vertex_buffer);
    sg_destroy_buffer(renderer->index_buffer);
    
    // Clean up arrays
    free(renderer->vertices);
    free(renderer->indices);
    
    memset(renderer, 0, sizeof(text_renderer_t));
}