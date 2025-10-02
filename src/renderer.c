#include "renderer.h"
#include "window.h"
#include <stdio.h>

bool renderer_initialize(AppState* state) {
    return renderer_init(state);
}

void renderer_shutdown_wrapper(AppState* state) {
    renderer_shutdown(state);
}

void renderer_frame_begin(AppState* state) {
    renderer_begin_frame(state);
}

void renderer_frame_end(AppState* state) {
    renderer_end_frame(state);
}

void renderer_handle_resize(AppState* state, int width, int height) {
    renderer_resize(state, width, height);
}

void renderer_clear_color(float r, float g, float b, float a) {
    renderer_set_clear_color(r, g, b, a);
}

sg_swapchain renderer_get_swapchain(AppState* state) {
    return get_swapchain(state);
}