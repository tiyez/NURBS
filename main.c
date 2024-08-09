
#include "essentials.c"
#include "memory.c"
#include "io.c"
#include "bezier.c"
#include "dx.c"
#include "fxc_bundle.h"
#include "fxc_bundle.c"
#include "windx.c"
#include "math.c"

#undef __File
#define __File "main.c"

struct camera {
    float eangle [2];
    float position [3];
};

struct top {
    struct shader nurbs_shader;
    DXt (ID3D11Buffer) *rbezier_patches_buffer;
    DXt (ID3D11Buffer) *nurbs_constant_buffer;
    DXt (ID3D11RasterizerState) *rasterizer;
    DXt (ID3D11BlendState) *blend;
    DXt (ID3D11DepthStencilState) *depth_state;
    DXt (ID3D11Texture2D) *depth_buffer;
    DXt (ID3D11DepthStencilView) *depth_view;
    struct nurbs nurbs;
    int number_of_rbezier_patches;
    int scroll;
    struct camera camera;
    struct continuous_keys keys;
    int is_wireframe;
    int is_nurbs_clamped;
};

struct nurbs_data {
    float   matrix [16];
    float   tess_factor;
    float   pad [3];
};

static void move_camera (_In_ struct camera *camera, _In_reads_ (3) float *delta) {
    float quat [4];
    float backward [3], right [3];

    make_q_pitch_yaw (quat, camera->eangle[0], camera->eangle[1]);
    q_plane_basis (quat, right, backward);
    v3_add_scalev (camera->position, right, delta[0]);
    camera->position[1] += delta[1];
    v3_add_scalev (camera->position, backward, -delta[2]);
}

static void make_view_from_camera (_In_ struct camera *camera, _Out_writes_ (16) float *view) {
    float quat [4];

    make_q_pitch_yaw (quat, camera->eangle[0], camera->eangle[1]);
    q_look (view, quat, camera->position);
}

static int init_debug_nurbs (_In_ DXt (ID3D11Device) *device, _In_ struct top *top) {
    int result;
    struct rbezier3d *_Stack patches;
    DXt (ID3D11Buffer) *buffer;

    init_debug_nurbs_configuration (&top->nurbs, top->is_nurbs_clamped);
    patches = allocate_stack (0);
    convert_nurbs_to_rbezier_patches (&top->nurbs, patches);
    buffer = make_vertex_buffer (device, patches, get_stack_size (patches));
    if (buffer) {
        release_buffer (&top->rbezier_patches_buffer);
        top->rbezier_patches_buffer = buffer;
        top->number_of_rbezier_patches = get_stack_size (patches) / (16 * sizeof *patches);
        result = 1;
    } else {
        Error ("could not make vertex buffer");
        result = 0;
    }
    release_stack (&patches);
    return (result);
}

static int handle_window_initialize (struct window *window) {
    struct top *top;

    top = window->userdata;
    if (0 == read_shader_from_fxc_bundle (window->dx.d3ddevice, "nurbs.hlsl.fxc", &top->nurbs_shader)) { Error ("could not load nurbs.hlsl.fxc"); return (0); }
    if (0 == init_debug_nurbs (window->dx.d3ddevice, top)) { Error ("could not init nurbs"); return (0); }
    top->nurbs_constant_buffer = make_constant_buffer_empty (window->dx.d3ddevice, sizeof (struct nurbs_data));
    if (0 == top->nurbs_constant_buffer) { Error ("could not make constant buffer"); return (0); }

    top->camera.position [0] = 2.5f;
    top->camera.position [1] = 2.5f;
    top->camera.position [2] = 0;
    top->camera.eangle [0] = 0;
    top->camera.eangle [1] = 0;

    top->rasterizer = make_rasterizer_state (window->dx.d3ddevice, !top->is_wireframe, 0);
    if (0 == top->rasterizer) { Error ("could not make rasterizer state"); return (0); }
    top->blend = make_blend_state (window->dx.d3ddevice);
    if (0 == top->blend) { Error ("could not make blend state"); return (0); }
    top->depth_state = make_depth_state (window->dx.d3ddevice);
    if (0 == top->depth_state) { Error ("could not make depth state"); return (0); }
    top->depth_buffer = make_depth_buffer (window->dx.d3ddevice, window->client_width, window->client_height);
    if (0 == top->depth_buffer) { Error ("could not make depth buffer"); return (0); }
    top->depth_view = make_depth_view (window->dx.d3ddevice, top->depth_buffer);
    if (0 == top->depth_view) { Error ("could not make depth view"); return (0); }
    return (1);
}

static void handle_window_release (struct window *window) {
}

static unsigned long long handle_window_resize (struct window *window, int is_dpi_changed) {
    struct top *top;

    top = window->userdata;
    release_depth_view (&top->depth_view);
    release_depth_buffer (&top->depth_buffer);
    top->depth_buffer = make_depth_buffer (window->dx.d3ddevice, window->client_width, window->client_height);
    if (0 == top->depth_buffer) { Error ("could not make depth buffer"); }
    top->depth_view = make_depth_view (window->dx.d3ddevice, top->depth_buffer);
    if (0 == top->depth_view) { Error ("could not make depth view"); }
    return (window->frame_version);
}

static unsigned long long handle_window_paint (struct window *window) {
    struct top *top;
    struct dx *dx;

    top = window->userdata;
    dx = &window->dx;
    if (begin_dx (dx)) {
        float proj [16], view [16];
        struct nurbs_data cb;

        cb.tess_factor = 16;
        init_fov_perspective_projection (proj, window->client_width, window->client_height, 90 * ToRad, 0.01f, 1000.f);
        if (is_continuous_keys (&top->keys)) {
            float delta [3];

            if (is_key_down (VK_SHIFT)) {
                delta [0] = 0.05f * (top->keys.right - top->keys.left);
                delta [1] = 0.05f * (top->keys.up - top->keys.down);
                delta [2] = 0.05f * (top->keys.forward - top->keys.backward);
            } else {
                delta [0] = 0.01f * (top->keys.right - top->keys.left);
                delta [1] = 0.01f * (top->keys.up - top->keys.down);
                delta [2] = 0.01f * (top->keys.forward - top->keys.backward);
            }
            move_camera (&top->camera, delta);
        }
        make_view_from_camera (&top->camera, view);
        m4_multiply (cb.matrix, view, proj);
        dx_update_buffer (dx, top->nurbs_constant_buffer, &cb, sizeof cb);

        setup_dx_render_target (dx, dx->render_target, top->depth_view);
        dx_clear_render_target (dx, dx->render_target, 0xFF0A0A0A);
        dx_clear_depth_view (dx, top->depth_view, 1);
        setup_dx_viewport (dx, 0, 0, window->client_width, window->client_height);
        setup_dx_rasterizer_state (dx, top->rasterizer);
        setup_dx_blend_state (dx, top->blend);
        setup_dx_depth_state (dx, top->depth_state);
        setup_dx_input_assembler (dx, top->nurbs_shader.input_layout, Topology (patch_16), top->rbezier_patches_buffer, sizeof (struct rbezier3d), 0);
        setup_dx_vertex_shader (dx, top->nurbs_shader.vs);
        setup_dx_pixel_shader (dx, top->nurbs_shader.ps);
        setup_dx_domain_shader (dx, top->nurbs_shader.ds);
        setup_dx_constant_buffer_for_domain_shader (dx, 0, top->nurbs_constant_buffer);
        setup_dx_hull_shader (dx, top->nurbs_shader.hs);
        setup_dx_constant_buffer_for_hull_shader (dx, 0, top->nurbs_constant_buffer);
        dx_draw (dx, 16 * top->number_of_rbezier_patches, 0);

        end_dx (dx);
    }
    return (window->frame_version + is_continuous_keys (&top->keys));
}

static unsigned long long handle_window_on_char (struct window *window, int codepoint) {
    return (window->frame_version);
}

static unsigned long long handle_window_on_keycommand (struct window *window, enum keycommand command) {
    struct top *top;
    int do_redraw;

    do_redraw = 0;
    top = window->userdata;
    if (command == KeyComm (f1)) {
        DXt (ID3D11RasterizerState) *rasterizer;

        top->is_wireframe = !top->is_wireframe;
        rasterizer = make_rasterizer_state (window->dx.d3ddevice, !top->is_wireframe, 0);
        if (rasterizer) {
            release_rasterizer_state (&top->rasterizer);
            top->rasterizer = rasterizer;
            do_redraw = 1;
        }
    } else if (command == KeyComm (f2)) {
        top->is_nurbs_clamped = !top->is_nurbs_clamped;
        do_redraw = init_debug_nurbs (window->dx.d3ddevice, top);
    }
    return (window->frame_version + do_redraw);
}

static unsigned long long handle_window_on_key_down (struct window *window, int virtkey, enum controlkey control) {
    struct top *top;

    top = window->userdata;
    return (window->frame_version + update_continuous_keys (&top->keys, virtkey, 1));
}

static unsigned long long handle_window_on_key_up (struct window *window, int virtkey, enum controlkey control) {
    struct top *top;

    top = window->userdata;
    return (window->frame_version + update_continuous_keys (&top->keys, virtkey, 0));
}

static unsigned long long handle_window_on_mouse_move (struct window *window, struct mouseinfo *mouseinfo) {
    struct top *top;
    int do_redraw;

    top = window->userdata;
    if (window->is_mouse_captured) {
        top->camera.eangle [0] -= 0.1f * mouseinfo->delta [0] * ToRad;
        top->camera.eangle [1] -= 0.1f * mouseinfo->delta [1] * ToRad;
        do_redraw = 1;
    } else {
        do_redraw = 0;
    }
    return (window->frame_version + do_redraw);
}

static unsigned long long handle_window_on_mouse_wheel (struct window *window, struct mouseinfo *mouseinfo) {
    struct top  *top;

    top = window->userdata;
    top->scroll += mouseinfo->wheel_delta;
    return (window->frame_version + 1);
}

static unsigned long long handle_window_on_activate (struct window *window, int is_active, int is_active_by_click, int mousepos[2]) {
    struct top *top;

    top = window->userdata;
    if (is_active) {
        reload_continuous_keys (&top->keys);
    } else {
        clear_continuous_keys (&top->keys);
    }
    return (window->frame_version + is_active);
}

static unsigned long long handle_window_on_mouse_capture (struct window *window, int is_captured, struct mouseinfo *mouseinfo, enum mousekey mousekeys, enum controlkey controlkeys) {
    return (window->frame_version);
}

static unsigned long long handle_window_on_mouse_button (struct window *window, struct mouseinfo *mouseinfo, enum mousekey mousekeys, enum controlkey controlkeys) {
    return (window->frame_version);
}

static unsigned long long handle_window_on_file_change (struct window *window, const char *filename) {
    return (window->frame_version);
}

int WinMain (_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev_instance, _In_ LPSTR cmdline, _In_ int showcmd) {
    int result;
    struct window_handlers handlers;
    struct top top;

    Zero (&top);
    Zero (&handlers);
    handlers.initialize = handle_window_initialize;
    handlers.release = handle_window_release;
    handlers.on_resize = handle_window_resize;
    handlers.on_paint = handle_window_paint;
    handlers.on_char = handle_window_on_char;
    handlers.on_keycommand = handle_window_on_keycommand;
    handlers.on_key_down = handle_window_on_key_down;
    handlers.on_key_up = handle_window_on_key_up;
    handlers.on_mouse_move = handle_window_on_mouse_move;
    handlers.on_mouse_wheel = handle_window_on_mouse_wheel;
    handlers.on_activate = handle_window_on_activate;
    handlers.on_mouse_capture = handle_window_on_mouse_capture;
    handlers.on_mouse_button = handle_window_on_mouse_button;
    handlers.on_file_change = handle_window_on_file_change;
    result = dxwinmain (instance, prev_instance, cmdline, showcmd, __FileW, &handlers, &top);
    Debug ("OK");
    return (0 == result);
}
















