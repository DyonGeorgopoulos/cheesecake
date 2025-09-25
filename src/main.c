#include "window.h"
#include "renderer.h"
#include <stdio.h>
#include <stdbool.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Sokol + SDL3 Demo"

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
    float clear_color[4] = {0.2f, 0.3f, 0.8f, 1.0f}; // Nice blue color
    int frame_count = 0;

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

        // Set clear color
        renderer_clear(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);

        // Begin frame
        renderer_begin_frame(&renderer_ctx);

        // TODO: Add your rendering code here
        // For now, just clear to the specified color

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
