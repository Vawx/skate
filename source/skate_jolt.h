/* date = August 22nd 2025 1:13 pm */

#ifndef SKATE_JOLT_H


#include "JoltC/JoltC.h"

#ifdef _MSC_VER
#define unreachable() __assume(0)
#else
#define unreachable() __builtin_unreachable()
#endif

typedef JPC_PhysicsSystem                               j_physics_system;
typedef JPC_BodyInterface                               j_body_interface;
typedef JPC_BodyCreationSettings                        j_body_creation_settings;
typedef JPC_Body                                        j_body;
typedef JPC_BodyID                                      j_body_id;
typedef JPC_Shape                                       j_shape;
typedef JPC_BroadPhaseLayer                             j_broadphase_layer;
typedef JPC_BroadPhaseLayerInterfaceFns                 j_broad_phase_layer_interface_func;
typedef JPC_ObjectVsBroadPhaseLayerFilterFns            j_obj_vs_broadphase_layer_filter_func;
typedef JPC_ObjectLayerPairFilterFns                    j_object_layer_Pair_filter_func;
typedef JPC_DebugRendererSimpleFns                      j_debug_render_func;
typedef JPC_TempAllocatorImpl                           j_temp_allocator;
typedef JPC_JobSystemThreadPool                         j_job_system_thread_pool;
typedef JPC_BroadPhaseLayerInterface                    j_broad_phase_layer_interface;
typedef JPC_ObjectVsBroadPhaseLayerFilter               j_object_vs_broadphase_layer_filter;
typedef JPC_ObjectLayerPairFilter                       j_obj_vs_obj_layer_pair_filter;

#define JOLT_MAX_BODIES                kilo(1)
#define JOLT_NUM_BODY_MUTEX            0
#define JOLT_MAX_BODY_PAIRS            kilo(1)
#define JOLT_MAX_CONTACT_CONSTRAINTS   kilo(1)

#define JOLT_TARGET_FPS                60.f
#define JOLT_FPS_MS                    1.f/JOLT_TARGET_FPS
#define JOLT_COLLISION_STEPS           1

#define JOLT_DONT_ACTIVATE JPC_ACTIVATION_DONT_ACTIVATE 
#define JOLT_ACTIVATE      JPC_ACTIVATION_ACTIVATE      

#define JOLT_STATIC        JPC_MOTION_TYPE_STATIC
#define JOLT_DYNAMIC       JPC_MOTION_TYPE_DYNAMIC

namespace SkateJoltObjectLayers {
	enum Type {
        NON_MOVING,
        MOVING,
        COUNT,
    };
};

namespace SkateJoltBroadPhaseLayers {
	enum Type {
        NON_MOVING,
        MOVING,
        COUNT,
    };
};

namespace SkateJoltShapeType {
    enum Type {
        NONE,
        SPHERE,
        BOX,
        PLANE,
        MESH,
        
        COUNT
    };
};

struct jolt_obj_t {
    mat4 transform;
    vec3 origin;
    
    j_body_id id;
    
    u32 activation;
    u8 motion;
    u8 object_layer;
    r32 density;
    
    SkateJoltShapeType::Type type;
    union {
        struct {
            r32 rad;
        };
        struct {
            vec3 whd;
        };
        struct {
            r32 dist;
        };
    };
};

struct skate_jolt {
    j_physics_system *physics_system;
    j_temp_allocator *temp_allocator;
    j_job_system_thread_pool *job_system;
    j_broad_phase_layer_interface *broad_phase_layer_interface;
    j_object_vs_broadphase_layer_filter *object_vs_broad_phase_layer_filter;
    j_obj_vs_obj_layer_pair_filter *object_vs_object_layer_filter;
    
    bool init;
};

static skate_jolt *get_jolt();
static void skate_jolt_init(skate_jolt *jolt);
static void skate_jolt_frame(skate_jolt *jolt);
static void skate_jolt_shutdown(skate_jolt *jolt);

static j_body_interface *jolt_get_body_interface();
static u32 jolt_get_num_of_broadphase_layers(const void *self);
static j_broadphase_layer jolt_get_broadphase_layer(const void *self, JPC_ObjectLayer in_layer);
static j_broad_phase_layer_interface_func jolt_get_broad_phase_layer_interface();
static bool jolt_object_broadphase_should_collide(const void *self, JPC_ObjectLayer ol, JPC_BroadPhaseLayer bpl);
static j_obj_vs_broadphase_layer_filter_func get_obj_vs_broadphase_layer_filter();
static bool jolt_obj_vs_obj_layer_should_collide(const void *self, JPC_ObjectLayer ol1, JPC_ObjectLayer ol2);
static j_object_layer_Pair_filter_func jolt_get_object_layer_pair_filter();
static void jolt_draw_debug_line(const void *self, JPC_RVec3 inFrom, JPC_RVec3 inTo, JPC_Color inColor);
static j_debug_render_func *jolt_get_debug_render();

static bool jolt_push_shape_sphere(vec3 pos, const r32 rad, u32 activation, u8 motion_type, u8 object_layer, j_body_id *out);
static bool jolt_push_shape_plane(vec3 pos, const r32 dist, u32 activation,  u8 motion_type, u8 object_layer, j_body_id *out);
static bool jolt_push_shape_box(vec3 pos, vec3 whd, u32 activation, u8 motion_type, u8 object_layer, j_body_id *out);
static void jolt_get_model_transform(j_body_id *id, mat4 out);
static void jolt_get_body_com_position(j_body_id *id, vec3 out);
#define SKATE_JOLT_H
#endif //SKATE_JOLT_H
