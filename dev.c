#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <linux/input.h>
#include "xdg-shell-client-protocol.h"
#include <cairo.h>

#define MIN_WIDTH 1920
#define MIN_HEIGHT 1040

struct client_state
{
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct wl_shm *shm;
    struct wl_buffer *buffer;
    struct xdg_wm_base *xdg_wm_base;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct wl_seat *seat;
    struct wl_pointer *pointer;
    int pointer_x, pointer_y;
    int button_pressed;
    int running;
    void *shm_data;
    int width, height;
    char message[256];
    cairo_surface_t *image_surface;
    int image_drawn; // New field to track if the image has been drawn
};

static int create_shm_file(size_t size)
{
    char name[] = "/wl_shm-XXXXXX";
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0)
    {
        shm_unlink(name);
        if (ftruncate(fd, size) == -1)
        {
            perror("ftruncate failed");
            close(fd);
            return -1;
        }
    }
    return fd;
}

static struct wl_buffer *create_buffer(struct client_state *state)
{
    int stride = state->width * 4;
    int size = stride * state->height;

    int fd = create_shm_file(size);
    if (fd < 0)
    {
        fprintf(stderr, "Failed to create shared memory file\n");
        return NULL;
    }

    state->shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state->shm_data == MAP_FAILED)
    {
        fprintf(stderr, "Failed to mmap shared memory\n");
        close(fd);
        return NULL;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(state->shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0,
                                                         state->width, state->height,
                                                         stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    return buffer;
}

static void draw_text(struct client_state *state, const char *text, int x, int y, int scaleFactor)
{
    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        (unsigned char *)state->shm_data,
        CAIRO_FORMAT_ARGB32,
        state->width, state->height,
        state->width * 4);

    cairo_t *cr = cairo_create(surface);

    cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 20 * scaleFactor);
    cairo_set_source_rgb(cr, 1, 1, 1);

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static int load_image(struct client_state *state, const char *image_path)
{
    state->image_surface = cairo_image_surface_create_from_png(image_path);
    if (cairo_surface_status(state->image_surface) != CAIRO_STATUS_SUCCESS)
    {
        fprintf(stderr, "Failed to load image: %s\n", image_path);
        return -1;
    }
    return 0;
}

static const char *button_images[5] = {
    "data/images/antakshari.png", // Image for button 1
    "data/images/bgsub.png",      // Image for button 2
    "data/images/mag.png",        // Image for button 3
    "data/images/karaoke.png",    // Image for button 4
    "data/images/vr.png"          // Image for button 5
};

static void draw_button_1(struct client_state *state)
{
    const char *image_path = "data/images/play.png";
    int button_height = state->height / 5;
    int y_start = 0; // Position for button 1

    // Load the image
    cairo_surface_t *image_surface = cairo_image_surface_create_from_png(image_path);
    if (cairo_surface_status(image_surface) != CAIRO_STATUS_SUCCESS)
    {
        fprintf(stderr, "Failed to load image: %s\n", image_path);
        return;
    }

    // Draw the image onto the button
    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        (unsigned char *)state->shm_data,
        CAIRO_FORMAT_ARGB32,
        state->width, state->height,
        state->width * 4);

    cairo_t *cr = cairo_create(surface);
    cairo_set_source_surface(cr, image_surface, 30, y_start); // Adjust the position (30, y_start)
    cairo_rectangle(cr, 30, y_start, 270, button_height);     // Clip the image to the button area
    cairo_fill(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_surface_destroy(image_surface);

    // Draw the button text
    const char *button_text = "Antakshari";
    draw_text(state, button_text, 50, y_start + button_height / 2, 2);
}

static void draw_button_2(struct client_state *state)
{
    uint32_t button_color = 0xFF00FF00; // Green color
    const char *button_text = "BG-Sub";
    int button_height = state->height / 5;
    int y_start = button_height; // Position for button 2

    uint32_t *pixels = state->shm_data;
    for (int y = y_start; y < y_start + button_height; y++)
    {
        for (int x = 30; x < 300; x++)
        {
            pixels[y * state->width + x] = button_color;
        }
    }
    draw_text(state, button_text, 50, y_start + button_height / 2, 2);
}

static void draw_button_3(struct client_state *state)
{
    uint32_t button_color = 0xFF0000FF; // Blue color
    const char *button_text = "MAG";
    int button_height = state->height / 5;
    int y_start = 2 * button_height; // Position for button 3

    uint32_t *pixels = state->shm_data;
    for (int y = y_start; y < y_start + button_height; y++)
    {
        for (int x = 30; x < 300; x++)
        {
            pixels[y * state->width + x] = button_color;
        }
    }
    draw_text(state, button_text, 50, y_start + button_height / 2, 2);
}

static void draw_button_4(struct client_state *state)
{
    uint32_t button_color = 0xFFFFFF00; // Yellow color
    const char *button_text = "Karaoke";
    int button_height = state->height / 5;
    int y_start = 3 * button_height; // Position for button 4

    uint32_t *pixels = state->shm_data;
    for (int y = y_start; y < y_start + button_height; y++)
    {
        for (int x = 30; x < 300; x++)
        {
            pixels[y * state->width + x] = button_color;
        }
    }
    draw_text(state, button_text, 50, y_start + button_height / 2, 2);
}

static void draw_button_5(struct client_state *state)
{
    uint32_t button_color = 0xFFFF00FF; // Magenta color
    const char *button_text = "VR-360";
    int button_height = state->height / 5;
    int y_start = 4 * button_height; // Position for button 5

    uint32_t *pixels = state->shm_data;
    for (int y = y_start; y < y_start + button_height; y++)
    {
        for (int x = 30; x < 300; x++)
        {
            pixels[y * state->width + x] = button_color;
        }
    }
    draw_text(state, button_text, 50, y_start + button_height / 2, 2);
}

static void draw_button(struct client_state *state)
{
    // Clear the buffer
    uint32_t *pixels = state->shm_data;
    for (int y = 0; y < state->height; y++)
    {
        for (int x = 0; x < state->width; x++)
        {
            pixels[y * state->width + x] = 0xFF000000; // Black background
        }
    }

    // Draw the buttons directly
    printf("button 1\n");
    draw_button_1(state);
    printf("button 2\n");
    draw_button_2(state);
    draw_button_3(state);
    draw_button_4(state);
    draw_button_5(state);

    // Draw the message on the right side
    draw_text(state, state->message, 300, 100, 1); // Display message at (300, 100)

    // Draw the image on the right side
    if (state->image_surface)
    {
        cairo_surface_t *surface = cairo_image_surface_create_for_data(
            (unsigned char *)state->shm_data,
            CAIRO_FORMAT_ARGB32,
            state->width, state->height,
            state->width * 4);

        cairo_t *cr = cairo_create(surface);
        cairo_set_source_surface(cr, state->image_surface, 300, 0); // Adjust position to start higher
        cairo_paint(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
    }

    // Draw a large cursor
    int cursor_size = 20; // Size of the cursor
    int cursor_x = state->pointer_x;
    int cursor_y = state->pointer_y;

    // Draw cursor as a filled circle
    for (int y = -cursor_size; y <= cursor_size; y++)
    {
        for (int x = -cursor_size; x <= cursor_size; x++)
        {
            if (x * x + y * y <= cursor_size * cursor_size)
            { // Circle equation
                int draw_x = cursor_x + x;
                int draw_y = cursor_y + y;
                if (draw_x >= 0 && draw_x < state->width && draw_y >= 0 && draw_y < state->height)
                {
                    pixels[draw_y * state->width + draw_x] = 0xFFFFFFFF; // White cursor
                }
            }
        }
    }

    wl_surface_attach(state->surface, state->buffer, 0, 0);
    wl_surface_damage(state->surface, 0, 0, state->width, state->height);
    wl_surface_commit(state->surface);
}

static void pointer_enter(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface,
                          wl_fixed_t sx, wl_fixed_t sy)
{
    struct client_state *state = data;
    state->pointer_x = wl_fixed_to_int(sx);
    state->pointer_y = wl_fixed_to_int(sy);
    (void)pointer;
    (void)serial;
    (void)surface;
}

static void pointer_leave(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface)
{
    (void)data;
    (void)pointer;
    (void)serial;
    (void)surface;
}

static void pointer_motion(void *data, struct wl_pointer *pointer,
                           uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    struct client_state *state = data;
    int new_x = wl_fixed_to_int(sx);
    int new_y = wl_fixed_to_int(sy);

    // Only update and redraw if the cursor position has changed
    if (new_x != state->pointer_x || new_y != state->pointer_y)
    {
        state->pointer_x = new_x;
        state->pointer_y = new_y;
        draw_button(state); // Redraw the buttons and cursor
    }
}

static void pointer_button(void *data, struct wl_pointer *pointer,
                           uint32_t serial, uint32_t time, uint32_t button,
                           uint32_t state_btn)
{
    struct client_state *state = data;
    (void)pointer;
    (void)serial;
    (void)time;

    if (button == BTN_LEFT && state_btn == WL_POINTER_BUTTON_STATE_PRESSED)
    {
        for (int i = 0; i < 5; i++)
        {
            if (state->pointer_y >= (i * (state->height / 5)) && state->pointer_y < ((i + 1) * (state->height / 5)) &&
                state->pointer_x >= 30 && state->pointer_x < 300)
            {
                // Load the image when a button is clicked
                const char *image_path = button_images[i]; // Use the corresponding image path
                printf("Loading image from: %s\n", image_path);
                load_image(state, image_path); // Load the image based on button clicked

                // Instead of drawing the message, draw the image directly
                state->button_pressed = i + 1;         // Track which button is pressed
                printf("Button %d clicked!\n", i + 1); // Log button click

                // Draw the button to update the display immediately
                draw_button(state); // Ensure the new image is rendered immediately
                break;              // Exit loop after handling the click
            }
        }
    }
}

static void pointer_axis(void *data, struct wl_pointer *pointer,
                         uint32_t time, uint32_t axis, wl_fixed_t value)
{
    (void)data;
    (void)pointer;
    (void)time;
    (void)axis;
    (void)value;
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
};

static void touch_down(void *data, struct wl_touch *touch,
                       uint32_t serial, uint32_t time, struct wl_surface *surface,
                       int32_t id, wl_fixed_t sx, wl_fixed_t sy)
{
    struct client_state *state = data;
    int touch_x = wl_fixed_to_int(sx);
    int touch_y = wl_fixed_to_int(sy);

    // Determine which button was touched based on coordinates
    for (int i = 0; i < 5; i++)
    {
        int button_height = state->height / 5;
        int y_start = i * button_height;

        if (touch_y >= y_start && touch_y < y_start + button_height &&
            touch_x >= 30 && touch_x < 300)
        {
            // Button touched, load and display corresponding image
            const char *image_path = button_images[i];
            printf("Touch detected on button %d, loading image: %s\n", i + 1, image_path);

            if (load_image(state, image_path) == 0)
            {                                  // Check if the image loaded successfully
                state->button_pressed = i + 1; // Track the pressed button
                draw_button(state);            // Redraw the UI with the image
            }
            break; // Exit after handling the touch
        }
    }
}

static void touch_motion(void *data, struct wl_touch *touch,
                         uint32_t time, int32_t id, wl_fixed_t sx, wl_fixed_t sy)
{
    // Optional: Implement motion functionality if needed
    // For this implementation, touch motion doesn't trigger any UI update
    (void)data;
    (void)touch;
    (void)time;
    (void)id;
    (void)sx;
    (void)sy;
}

static void touch_up(void *data, struct wl_touch *touch,
                     uint32_t serial, uint32_t time, int32_t id)
{
    // Optional: Handle touch release if any specific functionality is required
    (void)data;
    (void)touch;
    (void)serial;
    (void)time;
    (void)id;
}

static void touch_frame(void *data, struct wl_touch *touch)
{
    // No-op: Required to avoid NULL listener errors
    (void)data;
    (void)touch;
}

static void touch_cancel(void *data, struct wl_touch *touch)
{
    // No-op: Required to avoid NULL listener errors
    (void)data;
    (void)touch;
}

static const struct wl_touch_listener touch_listener = {
    .down = touch_down,     // Already implemented
    .up = touch_up,         // Already implemented
    .motion = touch_motion, // Already implemented
    .frame = touch_frame,   // Add a no-op frame handler
    .cancel = touch_cancel, // Add a no-op cancel handler
};

static void seat_capabilities(void *data, struct wl_seat *seat,
                              uint32_t capabilities)
{
    struct client_state *state = data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        state->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(state->pointer, &pointer_listener, state);
    }
    if (capabilities & WL_SEAT_CAPABILITY_TOUCH)
    {
        struct wl_touch *touch = wl_seat_get_touch(seat);
        wl_touch_add_listener(touch, &touch_listener, state); // Ensure listener is added
    }
}

static void seat_name(void *data, struct wl_seat *seat,
                      const char *name)
{
    (void)data;
    (void)seat;
    (void)name;
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
                                  uint32_t serial)
{
    struct client_state *state = data;
    xdg_surface_ack_configure(xdg_surface, serial);
    draw_button(state);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                   int32_t width, int32_t height,
                                   struct wl_array *states)
{
    struct client_state *state = data;
    if (width > 0 && height > 0)
    {
        state->width = width;
        state->height = height;
    }
    (void)xdg_toplevel;
    (void)states;
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
    struct client_state *state = data;
    state->running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
                             uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void registry_global(void *data, struct wl_registry *registry,
                            uint32_t name, const char *interface, uint32_t version)
{
    struct client_state *state = data;

    if (strcmp(interface, "wl_compositor") == 0)
    {
        state->compositor = wl_registry_bind(registry, name,
                                             &wl_compositor_interface, 4);
    }
    else if (strcmp(interface, "xdg_wm_base") == 0)
    {
        state->xdg_wm_base = wl_registry_bind(registry, name,
                                              &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base,
                                 &xdg_wm_base_listener, state);
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        state->seat = wl_registry_bind(registry, name,
                                       &wl_seat_interface, 1);
        wl_seat_add_listener(state->seat, &seat_listener, state);
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        state->shm = wl_registry_bind(registry, name,
                                      &wl_shm_interface, 1);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
                                   uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

int main(int argc, char **argv)
{
    struct client_state state = {0};
    state.running = 1;
    state.button_pressed = 0;
    state.width = MIN_WIDTH;
    state.height = MIN_HEIGHT;

    // Connect to the display
    state.display = wl_display_connect(NULL);
    if (!state.display)
    {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }

    // Get the registry
    state.registry = wl_display_get_registry(state.display);
    wl_registry_add_listener(state.registry, &registry_listener, &state);
    wl_display_roundtrip(state.display);

    if (!state.compositor || !state.shm)
    {
        fprintf(stderr, "No compositor or shm global\n");
        return 1;
    }

    // Create surface
    state.surface = wl_compositor_create_surface(state.compositor);
    if (!state.surface)
    {
        fprintf(stderr, "Failed to create surface\n");
        return 1;
    }

    // Create XDG surface
    state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base,
                                                    state.surface);
    if (!state.xdg_surface)
    {
        fprintf(stderr, "Failed to create XDG surface\n");
        return 1;
    }

    xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);

    // Create toplevel surface
    state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
    if (!state.xdg_toplevel)
    {
        fprintf(stderr, "Failed to create toplevel surface\n");
        return 1;
    }

    // Create and attach buffer
    state.buffer = create_buffer(&state);
    if (!state.buffer)
    {
        fprintf(stderr, "Failed to create buffer\n");
        return 1;
    }

    xdg_toplevel_add_listener(state.xdg_toplevel, &xdg_toplevel_listener, &state);
    xdg_toplevel_set_title(state.xdg_toplevel, "Wayland Button");
    xdg_toplevel_set_min_size(state.xdg_toplevel, MIN_WIDTH, MIN_HEIGHT);

    wl_surface_commit(state.surface);

    printf("Created Wayland window with clickable button\n");
    printf("Click in the window to see it change color\n");

    // Main event loop
    while (state.running)
    {
        wl_display_dispatch(state.display);
    }

    // Cleanup
    if (state.buffer)
        wl_buffer_destroy(state.buffer);
    if (state.pointer)
        wl_pointer_destroy(state.pointer);
    if (state.seat)
        wl_seat_destroy(state.seat);
    if (state.xdg_toplevel)
        xdg_toplevel_destroy(state.xdg_toplevel);
    if (state.xdg_surface)
        xdg_surface_destroy(state.xdg_surface);
    if (state.surface)
        wl_surface_destroy(state.surface);
    if (state.xdg_wm_base)
        xdg_wm_base_destroy(state.xdg_wm_base);
    if (state.shm)
        wl_shm_destroy(state.shm);
    if (state.compositor)
        wl_compositor_destroy(state.compositor);
    if (state.registry)
        wl_registry_destroy(state.registry);
    if (state.shm_data)
        munmap(state.shm_data, state.width * state.height * 4);

    return 0;
}
