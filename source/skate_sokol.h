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
#define INPUT_EVENT_TYPE_RELEASED 3

typedef u8 input_event_device;
#define INPUT_EVENT_DEVICE_KEYBOARD 0
#define INPUT_EVENT_DEVICE_MOUSE    1
#define INPUT_EVENT_DEVICE_TAP      2

struct input_event_t {
    input_event_type type;
    input_event_device device;
    u16 id;
};

struct input_event_list_t {
    input_event_t events[32];
    u32 events_idx;
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
};

//static skate_render_mesh_t render_mesh_from_import(const skate_model_import_result_t *result);
static void add_texture_to_render_mesh(skate_render_mesh_t *mesh, u8 *image_ptr, u32 image_width, u32 image_height, s32 image_idx, s32 sampler_idx);
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
    
    sg_pass pass;
    sg_attachments_desc att_desc;
};

namespace RENDER_PASS {
    const int TYPE_SHADOW  = 0;
    const int TYPE_STATIC  = 1;
    const int TYPE_DYNAMIC = 2;
    const int TYPE_COUNT   = 3;
};

const r32 CAMERA_NEAR_PLANE   = 0.1f;
const r32 CAMERA_FAR_PLANE    = 500.f;
const u32 CASCADE_LEVEL_COUNT = 4;
constexpr r32 shadow_cascade_levels[CASCADE_LEVEL_COUNT] = {50.f, 25.f, 10.f, 2.f};
const r32 map_sizes[CASCADE_LEVEL_COUNT] = {4096, 2048, 1024, 512};

class skate_model_import_t;
static skate_render_mesh_t *init_render_mesh(const skate_model_import_t *import, const int RENDER_PASS);
static void bind_image_data_to_render_mesh(skate_render_mesh_t *mesh, u8 *image_ptr, u32 image_width, u32 image_height, s32 image_idx, s32 sampler_idx, s8 binding_idx = -1);
static void bind_image_to_mesh(skate_render_mesh_t *mesh, const skate_image_file_t *file);

struct skate_sokol_t {
    input_event_list_t input_list;
    
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
};

static skate_sokol_t *get_sokol();

static void sokol_init();
static void sokol_frame();
static void sokol_cleanup();
static void sokol_event(const sapp_event *event);

#define SKATE_SOKOL_H
#endif //SKATE_SOKOL_H
