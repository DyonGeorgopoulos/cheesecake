#include "window.h"
#include "renderer.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "util/sokol_imgui.h"
#include <stdio.h>
#include <stdbool.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Sokol + ImGui + SDL3 Demo"

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

    // Initialize ImGui
    if (!renderer_imgui_init(&renderer_ctx)) {
        fprintf(stderr, "Failed to initialize ImGui\n");
        renderer_shutdown(&renderer_ctx);
        window_shutdown(&window);
        return -1;
    }

    printf("Application initialized successfully\n");

    // Application state
    bool show_demo_window = true;
    bool show_another_window = false;
    float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};
    int frame_count = 0;
    // Main loop

    printf("window should close:\n");
    printf("%d", !window_should_close(&window));
    while (!window_should_close(&window)) {
        //simgui_new_frame()
        printf("in the main loop\n");
        // Poll events
        window_poll_events(&window);

        // Handle window resize
        int current_width, current_height;
        window_get_size(&window, &current_width, &current_height);
        if (current_width != renderer_ctx.width || current_height != renderer_ctx.height) {
            renderer_resize(&renderer_ctx, current_width, current_height);
        }
        printf("before begin frame\n");
        // Begin frame
        renderer_begin_frame(&renderer_ctx);
        printf("begin frame\n");

        // Clear screen
        renderer_clear(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);

        // Start ImGui frame
        renderer_imgui_new_frame();

        // ImGui Demo Window
        if (show_demo_window) {
            igShowDemoWindow(&show_demo_window);
        }

        // Custom ImGui window
        {
            static float f = 0.0f;
            static int counter = 0;

            igBegin("Hello, world!", NULL, 0);

            igText("This is some useful text.");
            igCheckbox("Demo Window", &show_demo_window);
            igCheckbox("Another Window", &show_another_window);

            igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
            igColorEdit3("clear color", clear_color, ImGuiColorEditFlags_None);

            if (igButton("Button", (ImVec2){0, 0})) {
                counter++;
            }
            igSameLine(0.0f, -1.0f);
            igText("counter = %d", counter);

            struct ImGuiIO* io = igGetIO();
            igText("Application average %.3f ms/frame (%.1f FPS)", 
                   1000.0f / io->Framerate, io->Framerate);

            igEnd();
        }

        // Another custom window
        if (show_another_window) {
            igBegin("Another Window", &show_another_window, 0);
            igText("Hello from another window!");
            if (igButton("Close Me", (ImVec2){0, 0})) {
                show_another_window = false;
            }
            igEnd();
        }

        // Platform info window
        {
            igBegin("Platform Info", NULL, 0);
            
            #ifdef SOKOL_D3D11
            igText("Backend: Direct3D 11");
            igText("Platform: Windows");
            #elif defined(SOKOL_METAL)
            igText("Backend: Metal");
            igText("Platform: macOS");
            #elif defined(SOKOL_GLCORE)
            igText("Backend: OpenGL Core");
            igText("Platform: Unix/Linux");
            #else
            igText("Backend: Unknown");
            igText("Platform: Unknown");
            #endif
            
            igText("Frame: %d", frame_count);
            igText("Window Size: %dx%d", current_width, current_height);
            
            igEnd();
        }

        // Render ImGui
        igRender();
        renderer_imgui_render();

        // End frame and present
        renderer_end_frame(&renderer_ctx);
        window_present(&window);

        frame_count++;
    }

    // Cleanup
    printf("Shutting down application...\n");
    renderer_imgui_shutdown();
    renderer_shutdown(&renderer_ctx);
    window_shutdown(&window);

    printf("Application shut down successfully\n");
    return 0;
}