
#ifndef FBX_IMPORTER_H


static const float VERSION = 0.002;

#include "ufbx/ufbx.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "umath.h"

#include "fbx_types.h"

#define MAX_BONES 64
#define MAX_BLEND_SHAPES 64

typedef struct mesh_vertex {
	um_vec3 position;
	um_vec3 normal;
	um_vec2 uv;
	float f_vertex_index;
} mesh_vertex;

struct skin_vertex {
    u8 bone_index[4];
    u8 bone_weight[4];
};

typedef struct viewer_node_anim {
	float time_begin;
	float framerate;
	s32 num_frames;
	um_quat const_rot;
	um_vec3 const_pos;
	um_vec3 const_scale;
	um_quat* rot;
	um_vec3* pos;
	um_vec3* scale;
} viewer_node_anim;

typedef struct viewer_blend_channel_anim {
	float const_weight;
	float* weight;
} viewer_blend_channel_anim;

typedef struct viewer_anim {
	const char* name;
	float time_begin;
	float time_end;
	float framerate;
	s32 num_frames;
    
	viewer_node_anim* nodes;
	viewer_blend_channel_anim* blend_channels;
} viewer_anim;

typedef struct viewer_node {
    s32 parent_index;
    
	um_mat geometry_to_node;
	um_mat node_to_parent;
	um_mat node_to_world;
	um_mat geometry_to_world;
	um_mat normal_to_world;
} viewer_node;

typedef struct viewer_blend_channel {
	float weight;
} viewer_blend_channel;

typedef struct viewer_mesh_part {
	//sg_buffer vertex_buffer;
	//sg_buffer index_buffer;
	//sg_buffer skin_buffer; // Optional
    
    um_vec3 aabb_min;
    um_vec3 aabb_max; 
    
	mesh_vertex *vertex_buffer;
	u32 vertex_buffer_size;
    
	u32 *indice_buffer;
	u32 indice_buffer_size;
    
	skin_vertex *skin_buffer;
	u32 skin_buffer_size;
    
	s32 num_indices;
	s32 material_index;
} viewer_mesh_part;

struct fbx_image {
	uint16_t type;
	u32 width;
	u32 height;
	u32 num_slices;
	u32 pixel_format;
	uint64_t ptr_size;
	u8* ptr;
};

typedef struct viewer_mesh {
	s32* instance_node_indices;
	s32 num_instances;
    
	viewer_mesh_part* parts;
	s32 num_parts;
    
	bool aabb_is_local;
	um_vec3 aabb_min;
	um_vec3 aabb_max;
    
	// Skinning (optional)
	bool skinned;
	s32 num_bones;
	s32 bone_indices[MAX_BONES];
	um_mat bone_matrices[MAX_BONES];
    
	// Blend shapes (optional)
	s32 num_blend_shapes;
	fbx_image blend_shape_image;
	s32 blend_channel_indices[MAX_BLEND_SHAPES];
    
	const char path[255];
	u32 path_len;
    
} viewer_mesh;

typedef struct viewer_scene {
    
	viewer_node* nodes;
	s32 num_nodes;
    
	viewer_mesh* meshes;
	s32 num_meshes;
    
	viewer_blend_channel* blend_channels;
	s32 num_blend_channels;
    
	viewer_anim* animations;
	s32 num_animations;
    
	um_vec3 aabb_min;
	um_vec3 aabb_max;
    
} viewer_scene;

typedef struct viewer {
    
	viewer_scene scene;
	float anim_time;
    
	//sg_shader shader_mesh_lit_static;
	//sg_shader shader_mesh_lit_skinned;
	//sg_pipeline pipe_mesh_lit_static;
	//sg_pipeline pipe_mesh_lit_skinned;
	//sg_image empty_blend_shape_image;
    
	um_mat world_to_view;
	um_mat view_to_clip;
	um_mat world_to_clip;
    
	float camera_yaw;
	float camera_pitch;
	float camera_distance;
	u32 mouse_buttons;
} viewer;

// NOTE(Kyle) this is for _every_ mesh PART
// mesh id, material_idx, indices_count, vertices_size, skin_buffer_size, skin_buffer, vertices_ptr, indices_ptr
#define PART_ID                     "{mesh_part}"
#define PART_AABB_MIN_ID            "{aabb_min}"
#define PART_AABB_MAX_ID            "{aabb_max}"
#define PART_INDICES_PTR_ID         "{indices_ptr}"
#define PART_VERTICES_PTR_ID        "{vertice_ptr}"
#define PART_SKINNED_VERTEX_PTR_ID  "{skin_vertex_ptr}"

enum PART_HEADER_IDS {
    AABB_MIN,
    AABB_MAX,
	INDICES,
	VERTICES,
	SKIN,
	PART_HEADER_IDS_COUNT
};

#define PART_HEADER "%d,%d,%d,%d,%d,"

// NOTE(Kyle) thi is for _every_ MESH
// mesh_path, nodes_count, instances_count, skinned?, num_blend_shapes, num_bones, instances_ptr, bone_indices_ptr, bone_matrices_ptr, blend_shapes_ptr, blend_shape_image_ptr, 
#define INSTANCES_PTR_ID          "{instances_ptr}"
#define BONE_INDICES_PTR_ID       "{bone_indices_ptr}"
#define BONE_MATRICES_PTR_ID      "{bone_matrices_ptr}"
#define BLEND_SHAPES_PTR_ID       "{blend_shapes_ptr}"
#define BLEND_SHAPE_IMAGE_PTR_ID  "{blend_shape_image_ptr}"
#define NODES_PTR_ID              "{nodes_ptr}"

#define MESH_HEADER "%s,%d,%d,%d,%d,%d,%d, " 

enum MESH_HEADER_IDS {
	INSTANCES,
	BONE_INDICES,
	BONE_MATRICES,
	BLEND_SHAPES,
	BLEND_SHAPE_IMAGE,
    NODES,
    
	MESH_HEADER_IDS_COUNT
};

class fbx_string {
    public:
    fbx_string() : ptr(nullptr), len(0) {}
    fbx_string(const int size);
    fbx_string(const char *in_ptr, const int size);
    
    static void init_strings();
    
    char *ptr;
    u32 len;
    
    static char *BACK_BUFFER;
    static char *BACK_BUFFER_PTR;
    static int BACK_BUFFER_SIZE;
};

#define COMMON_CONTENT_DIR "/content/"
#define COMMON_MESH_DIR "/content/mesh/"
#define COMMON_TEXTURE_DIR "/content/texture/"

struct fbx_dir {
    fbx_dir() : len(0) {memset(ptr, 0, 255);}
    fbx_dir(const char *in_ptr) {
        len = (u32)strlen(in_ptr);
        memset(ptr, 0, 255);
        memcpy(ptr, in_ptr, len);
    }
    char ptr[255];
    u32 len;
};

class fbx_importer {
    public:
    
	fbx_importer() : temp_buffer(nullptr), temp_buffer_ptr(nullptr), temp_buffer_size(0), hold_buffer(nullptr), hold_buffer_ptr(nullptr), hold_buffer_size(0), serialized_buffer(nullptr), serialized_buffer_ptr(nullptr), serialized_buffer_size(0), id_count(0), strings(nullptr) {}
	fbx_importer(const fbx_dir *source_dir, const fbx_dir *output_dir, bool output_debug);
    
    bool output_uncompressed;
    
	private:
    
    fbx_dir source;
    fbx_dir output;
    
	viewer_scene _scene;
    
    fbx_string transform_part_header(viewer_mesh_part* part);
    fbx_string transform_mesh(viewer_mesh *mesh);
    
	void read_node(viewer_node* vnode, ufbx_node* node);
	fbx_image pack_blend_channels_to_image(ufbx_mesh* mesh, ufbx_blend_channel** channels, s32 num_channels);
	void read_mesh(viewer_mesh* vmesh, ufbx_mesh* mesh, const char* in_path);
	void read_blend_channel(viewer_blend_channel* vchan, ufbx_blend_channel* chan);
	void read_node_anim(viewer_anim* va, viewer_node_anim* vna, ufbx_anim_stack* stack, ufbx_node* node);
	void read_blend_channel_anim(viewer_anim* va, viewer_blend_channel_anim* vbca, ufbx_anim_stack* stack, ufbx_blend_channel* chan);
	void read_anim_stack(viewer_anim* va, ufbx_anim_stack* stack, ufbx_scene* scene);
	void read_scene(ufbx_scene* scene, const char* path);
	void update_animation(viewer_anim* va, float time);
	void update_hierarchy();
	void init_pipelines(viewer* view);
	void load_scene();		
    
	void serialize();
    
	char *temp_buffer;
	char *temp_buffer_ptr;
	uint64_t temp_buffer_size;
    
	char *hold_buffer;
	char *hold_buffer_ptr;
	uint64_t hold_buffer_size;
    
    char *serialized_buffer;
    char *serialized_buffer_ptr;
	u32 serialized_buffer_size;
    
    u32 id_count;
    
    fbx_string *strings;
};

#define FBX_IMPORTER_H
#endif