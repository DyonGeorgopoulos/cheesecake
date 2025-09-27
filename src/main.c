#include "window.h"
#include "renderer.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "font_rendering.h"

#include "shader.glsl.h"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Sokol + SDL3 Demo"

static sg_shader g_shader;
sg_pipeline g_pipeline;
sg_bindings g_bind;

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv;

    printf("Starting %s...\n", WINDOW_TITLE);

    // Initialize window
    window_t window = {0};
    if (!window_init(&window, WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        fprintf(stderr, "Failed to initialize window\n");
        return -1;
    }

    // Initialize renderer
    renderer_context_t renderer_ctx = {0};
    if (!renderer_init(&renderer_ctx, window_get_handle(&window), WINDOW_WIDTH, WINDOW_HEIGHT)) {
        fprintf(stderr, "Failed to initialize renderer\n");
        window_shutdown(&window);
        return -1;
    }

    printf("Application initialized successfully\n");

    // Application state
    int frame_count = 0;

    // create the shader
    // initialise the shader
    g_shader = sg_make_shader(sgp_program_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(g_shader) != SG_RESOURCESTATE_VALID)
    {
        fprintf(stderr, "failed to make custom pipeline shader\n");
        exit(-1);
    }

    g_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        // if the vertex layout doesn't have gaps, don't need to provide strides and offsets
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT3,
                [1].format = SG_VERTEXFORMAT_FLOAT4
            }
        },
        .shader = g_shader,
    });

    // a vertex buffer with the triangle vertices
    const float vertices[] = {
        // positions            colors
         0.0f, 0.5f, 0.5f,      1.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f
    };
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices)
    });

    // define resource bindings
    g_bind = (sg_bindings){
        .vertex_buffers[0] = vbuf
    };

    text_renderer_t renderer;
    text_renderer_init(&renderer, 1000);
    int font = text_renderer_load_font(&renderer, "../../../assets/Roboto-Black.ttf", 24);
    // Main loop
    while (!window_should_close(&window)) {
        // Poll events
        window_poll_events(&window);

        // Handle window resize
        int current_width, current_height;
        window_get_size(&window, &current_width, &current_height);
        if (current_width != renderer_ctx.width || current_height != renderer_ctx.height) {
            renderer_resize(&renderer_ctx, current_width, current_height);
        }

        // Begin frame
        renderer_begin_frame(&renderer_ctx);

        // update this for drawing all the entities
        sg_apply_pipeline(g_pipeline);
        sg_apply_bindings(&g_bind);
        sg_draw(0, 3, 1);
        
        
        // text / ui code should go here
        sg_apply_pipeline(renderer.pipeline);
        text_renderer_render(&renderer, current_width, current_height); 
        // Draw text
        text_renderer_draw_text(&renderer, font, "TEST FONT!", 0, 20, 1.0f, (float[4]){1.0f, 1.0f, 1.0f, 1.0f});


        // End frame and present
        renderer_end_frame(&renderer_ctx);
        window_present(&window);

        frame_count++;

        // Print frame info every 60 frames
        if (frame_count % 60 == 0) {
            printf("Frame %d - Window: %dx%d\n", frame_count, current_width, current_height);
        }
    }

    // Cleanup
    printf("Shutting down application...\n");
    renderer_shutdown(&renderer_ctx);
    window_shutdown(&window);

    printf("Application shut down successfully\n");
    return 0;
}
