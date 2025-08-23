/* date = July 30th 2025 1:15 pm */
#ifndef SKATE_SOKOL_H

#define SKATE_INCLUDE_DEBUG_CUBE_AND_TEXTURE 1

#define SOKOL_D3D11
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

#include "shader/texture_static_mesh.glsl.h"

typedef u8 input_event_type;
#define INPUT_EVENT_TYPE_NONE     0
#define INPUT_EVENT_TYPE_PRESSED  1
#define INPUT_EVENT_TYPE_HELD     2
#define INPUT_EVENT_TYPE_RELEASED 4
#define INPUT_EVENT_TYPE_ANY      8

#define INPUT_KEY_NONE           -1

typedef u8 input_event_device;
#define INPUT_EVENT_DEVICE_KEYBOARD 0
#define INPUT_EVENT_DEVICE_MOUSE    1
#define INPUT_EVENT_DEVICE_TAP      2
#define INPUT_EVENT_DEVICE_NONE     255

typedef s16 input_mouse_button;
#define INPUT_MOUSE_BUTTON_NONE     -1
#define INPUT_MOUSE_BUTTON_LEFT     0
#define INPUT_MOUSE_BUTTON_RIGHT    1
#define INPUT_MOUSE_BUTTON_MIDDLE   2

struct input_mouse_pos_data_t {
    union {
        vec2 delta;
        struct {
            r32 dx, dy;
        };
    };
    
    union {
        vec2 pos;
        struct {
            r32 x, y;
        };
    };
    
    union {
        vec2 last_pos;
        struct {
            r32 lx, ly;
        };
    };
};

struct input_event_t {
    input_event_type type = INPUT_EVENT_TYPE_RELEASED;
    input_event_device device;
    union {
        input_mouse_button mb;
        s16 id = 0;
    };
    u64 frame_id = 0;
    input_mouse_pos_data_t mouse_pos_data;
    
};

struct input_event_list_t {
    static const u32 events_size = 32;
    input_event_t events[events_size];
};

bool is_key_pressed(sapp_keycode code);
bool is_key_held(sapp_keycode code);
bool is_key_released(sapp_keycode code);

struct input_event_filter_item_t {
    input_event_device device;
    union {
        sapp_keycode listen_key = SAPP_KEYCODE_INVALID;
        input_mouse_button listen_mb;
    };
    input_event_type event_type = INPUT_EVENT_TYPE_NONE; // if not none, filter by input type;
};

// if you want a input bind that just sends all events, make sure the filter being used to bind
// to the function is nullptr. if you pass in a "empty" one, youll get an error and no result(s).
struct input_event_list_filter_t {
    input_event_filter_item_t key_events[64];
    u32 idx = 0;
};

struct input_binding_key_or_button_t {
    union {
        sapp_keycode code;
        sapp_mousebutton mb;
    };
    
    input_event_device device;
    input_mouse_pos_data_t mouse_pos_data;
};

// this is the _response_ function for input. give code and filter back. filter because system(s) can hold
// what they want in it to pass between their system and input
typedef void (*input_response_binding)(const input_binding_key_or_button_t key_or_button, input_event_type input_type, input_event_list_filter_t *filter);

// this is the call to bind to input. as an example, a camera might bind with a filter list containing w,a,s,d,space,shift.
void bind_to_input(input_response_binding binding_func, input_event_list_filter_t *filter);

struct response_binding_handle_t {
    input_response_binding binding_func;
    input_event_list_filter_t filter;
};

#define DEFAULT_INPUT_RESPONSE_BINDING_ALLOCATION_COUNT 64

struct input_t {
    input_event_list_t list;
    response_binding_handle_t bindings[DEFAULT_INPUT_RESPONSE_BINDING_ALLOCATION_COUNT];
    int bindings_idx;
};

#define MAX_RENDER_BINDINGS 8
#define MAX_MESHES_PER_RENDER_PASS kilo(4)

struct skate_render_binding_t {
    sg_bindings sgb;
    u32 indice_count;
};

struct skate_render_bindings_t {
    skate_render_binding_t bindings[MAX_RENDER_BINDINGS];
    u32 bindings_count;
};

struct skate_render_mesh_t {
    skate_render_bindings_t bind;
    vec3 position = {0.f, 0.f, 0.f};
    vec3 rotation = {0.f, 0.f, 0.f};
    vec3 scale = {1.f, 1.f, 1.f};
    
    mat4 model;
    u8 ignore_shadow;
    skate_render_mesh_t *next;
};

//static skate_render_mesh_t render_mesh_from_import(const skate_model_import_result_t *result);
static void add_texture_to_render_mesh(skate_render_mesh_t *mesh, u8 *image_ptr, u32 image_width, u32 image_height, s32 image_idx, s32 sampler_idx);

const r32 CAMERA_NEAR_PLANE   = 10.f;
const r32 CAMERA_FAR_PLANE    = 2200.f;
const u32 CASCADE_LEVEL_COUNT = 4;
constexpr r32 shadow_cascade_levels[CASCADE_LEVEL_COUNT] = {2, 10, 25, 50};
const r32 map_sizes[CASCADE_LEVEL_COUNT] = {4096, 2048, 1024, 512};

struct skate_render_pass_t {
    sg_pass_action pass_action;
    sg_pipeline pipeline;
    skate_buffer_t mesh_buffer;
    skate_buffer_t *shared_buffer;
    
    // any extra data to render meshes + whatever
    u8 active;
    u8 use_atts_no_swapchain;
    
    sg_attachments atts;
    sg_image map;
    sg_sampler smp;
    
    vec3 light_pos;
    vec3 light_dir;
    mat4 light_mat;
    
    sg_bindings pass_bindings;
    sg_pass pass[CASCADE_LEVEL_COUNT];
};

namespace RENDER_PASS {
    const int TYPE_SHADOW  = 0;
    const int TYPE_STATIC  = 1;
    const int TYPE_DYNAMIC = 2;
    const int TYPE_COUNT   = 3;
};

class skate_model_import_t;
static skate_render_mesh_t *init_render_mesh(const skate_model_import_t *import, const int RENDER_PASS);
static void bind_image_data_to_render_mesh(skate_render_mesh_t *mesh, u8 *image_ptr, u32 image_width, u32 image_height, s32 image_idx, s32 sampler_idx, s8 binding_idx = -1);
static void bind_image_to_mesh(skate_render_mesh_t *mesh, const skate_image_file_t *file);

typedef void (*view_velocity_calc_func)(u8 *view);

#define DEFAULT_CAMERA_MASS 1.f

struct skate_view_t {
    vec3 position;
    
    vec3 velocity;
    vec3 frame_impulse;
    view_velocity_calc_func func;
    
    vec3 direction;
    vec3 right;
    vec3 up;
    
    r32 fov;
    
    mat4 view;
    
    u8 is_ortho = 0;
    union {
        mat4 perspective;
        mat4 ortho;
    };
    
    r32 yaw = -90.f;
    r32 pitch = 0.f;
    
    r32 movement_mass = DEFAULT_CAMERA_MASS;
    r32 movement_speed = 24.f;
    r32 movement_accel = 3.f;
    r32 movement_friction = 10.f; 
    r32 rotation_speed = 5.f;
    
    input_response_binding input_func;
    input_event_list_filter_t filter;
};

static skate_view_t make_view(vec3 pos, vec3 target, r32 fov);
static void update_view(skate_view_t *view);
static void view_tick(skate_view_t *view);

struct skate_sokol_t {
    input_t input;
    
    sg_pass_action default_pass_action; // default is just clear and clear color
    sg_pipeline default_pipeline; // for testing
    
    skate_render_pass_t render_pass[RENDER_PASS::TYPE_COUNT];
    
    sg_image default_image;
    sg_sampler default_texture;
    
#if SKATE_INCLUDE_DEBUG_CUBE_AND_TEXTURE
    skate_render_mesh_t debug_cube;
#endif
    
    skate_view_t default_view;
    skate_directory_t root_dir;
    
    const r32 aspect() const {return sapp_widthf() / sapp_heightf();}
    const r32 widthf() const {return sapp_widthf();}
    const r32 heightf() const {return sapp_heightf();}
    const s32 width() const {return sapp_width();}
    const s32 height() const {return sapp_height();}
    const r32 delta_time() const {return sapp_frame_duration();}
    
    mat4 light_view_projections[CASCADE_LEVEL_COUNT + 1];
    vec4 cascade_splits_shadow_map_size[CASCADE_LEVEL_COUNT];
    
    static constexpr s32 initial_window_size[2] = {1920, 1080};
    
    u64 last_frame_count;
    
    input_mouse_pos_data_t global_mouse_pos_data;
};

static skate_sokol_t *get_sokol();

static void sokol_init();
static void sokol_frame();
static void sokol_cleanup();
static void sokol_event(const sapp_event *event);

#define SKATE_SOKOL_H
#endif //SKATE_SOKOL_H
