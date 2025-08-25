/* date = August 23rd 2025 6:11 pm */

#ifndef SKATE_RENDER_OBJECT_H

// api for making render objects with or without physics

struct skate_render_obj_t {
    u8 physics_enabled;
    skate_render_mesh_t *mesh; // probably union when skate_anim_render_mesh_t is in
    
    union {
        struct {
            jolt_obj_t jolt_obj;
        };
    };
    
    s32 id = -1;
    void *outer;
};

#define SKATE_RENDER_DEFAULT_RENDER_OBJECT_COUNT kilo(2)

struct skate_render_t {
    skate_buffer_t obj_buffer;
};

static skate_render_t *get_render();
static skate_render_obj_t *get_render_obj(const skate_directory_t *dir);
static void set_render_obj_physics_enabled(skate_render_obj_t *obj, u8 in);
static void set_render_obj_pos_x(skate_render_obj_t *obj, r32 x);
static void set_render_obj_pos_y(skate_render_obj_t *obj, r32 y);
static void set_render_obj_pos_z(skate_render_obj_t *obj, r32 z);
static void set_render_obj_pos(skate_render_obj_t *obj, vec3 in);
static void set_render_obj_rot(skate_render_obj_t *obj, vec3 in);
static void set_render_obj_scale(skate_render_obj_t *obj, vec3 in);


#define SKATE_RENDER_OBJECT_H
#endif //SKATE_RENDER_OBJECT_H
