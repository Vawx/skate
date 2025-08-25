/* date = August 23rd 2025 6:23 pm */

#ifndef SKATE_WORLD_H

// world and world entities

struct skate_entity_t {
    u8 rendered;
    union {
        skate_render_obj_t *obj;
        struct {
            // to keep tabs in case this is spawned and then populated
            // also can serve as origin for the render obj
            vec3 pos; 
        };
    };
    
    s32 id = -1; // >= 0 is valid
};

#define SKATE_WORLD_ENTITY_BUFFER_START_COUNT kilo(1)

struct skate_world_t {
    skate_buffer_t entity_buffer;
    
    bool init;
};

static skate_world_t *get_world();
static skate_entity_t *get_entity();
static skate_entity_t *get_entity_rendered(skate_render_obj_t *rendered_obj);
static bool destroy_entity(skate_entity_t *ent);
static void set_entity_rendered(skate_entity_t *ent, u8 in);
static void set_entity_render_obj(skate_entity_t *ent, skate_render_obj_t *in);
static void set_entity_pos(skate_entity_t *ent, vec3 pos);
static void set_entity_rendered_pos_x(skate_entity_t *ent, r32 x);
static void set_entity_rendered_pos_y(skate_entity_t *ent, r32 y);
static void set_entity_rendered_pos_z(skate_entity_t *ent, r32 z);
static void set_entity_rendered_pos(skate_entity_t *ent, vec3 pos);
static void set_entity_rendered_rot(skate_entity_t *ent, vec3 rot);
static void set_entity_rendered_scale(skate_entity_t *ent, vec3 scale);
static void get_entity_model(skate_entity_t *ent, mat4 out);

#define SKATE_WORLD_H
#endif //SKATE_WORLD_H
