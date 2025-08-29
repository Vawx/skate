/* date = July 30th 2025 8:14 pm */
#ifndef SKATE_FBX_IMPORT_H

#define PART_ID                         "{mesh_part}"
#define PART_AABB_MIN_ID                "{aabb_min}"
#define PART_AABB_MAX_ID                "{aabb_max}"
#define PART_INDICES_PTR_ID             "{indices_ptr}:"
#define PART_VERTICES_PTR_ID            "{vertice_ptr}:"
#define PART_SKINNED_VERTEX_PTR_ID      "{skin_vertex_ptr}:"

#define INSTANCES_PTR_ID                "{instances_ptr}:"
#define BONE_INDICES_PTR_ID             "{bone_indices_ptr}:"
#define BONE_MATRICES_PTR_ID            "{bone_matrices_ptr}:"
#define BLEND_SHAPES_PTR_ID             "{blend_shapes_ptr}:"
#define BLEND_SHAPE_IMAGE_PTR_ID        "{blend_shape_image_ptr}:"
#define NODES_PTR_ID                    "{nodes_ptr}"

#define EXPECTED_VERTEX_SIZE 36

// EXPECTED MESH STATE ORDER, for parsing
enum MESH_STATE_ORDER {
    VERSION,
    MESH_PATH,
    NUM_NODES,
    NUM_PARTS,
    NUM_INSTANCES,
    SKINNED,
    NUM_BLEND_SHAPES,
    NUM_BONES,
    INSTANCES,
    BONE_INDICES,
    BONE_MATRICES,
    BLEND_SHAPES,
    BLEND_SHAPE_IMAGE,
    NODES,
};

// EXPECTED MESH PART STATE ORDER (THIS CAN CYCLE DEPENDING ON NUMBER OF MESH PARTS)
namespace MESH_PART_ORDER {
    const int ID                  = 0;
    const int MATERIAL_IDX        = 1;
    const int NUM_INDICES         = 2;
    const int VERTEX_BUFFER_SIZE  = 3;
    const int SKIN_BUFFER_SIZE    = 4;
    const int COUNT               = 5;
};

struct skate_model_blend_shape_image_t {
    uint16_t type;
	u32 width;
	u32 height;
	u32 num_slices;
	u32 pixel_format;
	uint64_t ptr_size;
	u8* ptr;
};

namespace BLEND_SHAPE_ORDER {
    const int TYPE          = 0;
    const int WIDTH         = 1;
    const int HEIGHT        = 2;
    const int NUM_SLICES    = 3;
    const int PIXEL_FORMAT  = 4;
    const int PTR_SIZE      = 5;
    const int PTR           = 6;
    const int COUNT         = 7;
};

namespace MODEL_VIEW_NODE_ORDER {
    const int GEOMETRY_TO_NODE    = 0;
    const int NODE_TO_PARENT      = 1;
    const int NODE_TO_WORLD       = 2;
    const int GEOMETRY_TO_WORLD   = 3;
    const int NORMAL_TO_WORLD     = 4;
    const int COUNT               = 5;
};

struct skate_model_import_part_vertice_t {
    r32 x, y, z;
    r32 nx, ny, nz;
    r32 ux, uy, vertex_idx;
};

struct skate_model_viewer_node_t {
    s32 parent_idx;
    
	mat4 geometry_to_node;
	mat4 node_to_parent;
	mat4 node_to_world;
	mat4 geometry_to_world;
	mat4 normal_to_world;
};

struct skate_model_anim_node_t {
    r32 time_begin;
    r32 framerate;
	s32 num_frames;
    vec3 const_rot;
	vec3 const_pos;
	vec3 const_scale;
	vec3 rot;
	vec3 *pos;
	vec3 *scale;
};

struct skate_model_import_part_t {
    u16 id;
    u32 mat_idx;
    u8 *vertices_ptr;
    u32 vertices_size;
    u8 *indices_ptr;
    u32 indices_count;
    u8 *skin_ptr;
    u32 skin_size;
    
    vec3 aabb_min;
    vec3 aabb_max;
};

struct skate_model_import_result_t {
    char name[255];
    u32 name_len;
    
    u8 *instances_ptr;
    u32 instances_size;
    u8 *bone_indices_ptr;
    u32 bone_indices_size;
    u8 *bone_matrices_ptr;
    u32 bone_matrices_size;
    u8 *blend_shapes_ptr;
    u32 blend_shapes_size;
    u8 *nodes_ptr;
    u32 nodes_ptr_size;
    
    u32 num_nodes;
    u32 num_bones;
    u32 num_blend_shapes;
    u32 num_instances;
    u8 skinned;
    
    skate_model_blend_shape_image_t blend_shape_image;
    
    skate_model_import_part_t *parts;
    u32 num_parts;
};

class skate_model_import_t {
    public:
    skate_model_import_t() {}
    skate_model_import_t(const skate_directory_t *source);
    
    MESH_STATE_ORDER mesh_import_state;
    
    skate_model_import_result_t import_result;
    
    u8 *hold_buffer;
    u8 *hold_buffer_ptr;
    u32 hold_buffer_size;
    
    skate_directory_t source_dir;
};

class skate_import_t {
    public:
    
    skate_import_t() {}
    skate_model_import_t *get_or_load_model(const skate_directory_t *dir);
    
    static skate_import_t *get() {
        static skate_import_t r;
        if(r.model_import_buffer.ptr == nullptr) {
            r.model_import_buffer = alloc_buffer(sizeof(skate_model_import_t), kilo(1));
        }
        return &r;
    }
    
    skate_buffer_t model_import_buffer;
};

#define SKATE_FBX_IMPORT_H
#endif //SKATE_FBX_IMPORT_H
