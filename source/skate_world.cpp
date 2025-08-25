
static skate_world_t *get_world() {
    static skate_world_t world;
    if(!world.init) {
        world.entity_buffer = alloc_buffer(sizeof(skate_entity_t), SKATE_WORLD_ENTITY_BUFFER_START_COUNT);
        for(int i = 0; i < SKATE_WORLD_ENTITY_BUFFER_START_COUNT; ++i) {
            skate_entity_t *ent = world.entity_buffer.as_idx<skate_entity_t>(i);
            ent->id = -1;
        }
        world.init = true;
    }
    return &world;
}

static skate_entity_t *get_entity() {
    skate_world_t *world = get_world();
    skate_entity_t *ent = nullptr;
    
    u32 cc = 0;
    for(;;) {
        ent = world->entity_buffer.as_idx<skate_entity_t>(cc++);
        if(ent && ent->id < 0) {break;}
    }
    
    memset(ent, 0, sizeof(skate_entity_t));
    ent->id = cc;
    return ent;
}

static skate_entity_t *get_entity_rendered(skate_render_obj_t *rendered_obj) {
    skate_entity_t *ent = get_entity();
    rendered_obj->outer = ent;
    set_entity_render_obj(ent, rendered_obj);
    return ent;
}

static bool destroy_entity(skate_entity_t *ent) {
    LOG_YELL_COND(ent && ent->id >= 0, "trying to destroy an entity that is already destroyed"); 
    
    skate_entity_t *ee = get_world()->entity_buffer.as_idx<skate_entity_t>(ent->id);
    LOG_PANIC_COND(ent == ee, "entity passed in to be destroyed does not match what is in the entity buffer");
    
    ee->id = -1;
}

static void set_entity_rendered(skate_entity_t *ent, u8 in) {
    LOG_YELL_COND(ent && ent->id >= 0, "trying to set invalid entity to render"); 
    ent->rendered = in;
}

static void set_entity_render_obj(skate_entity_t *ent, skate_render_obj_t *in) {
    LOG_YELL_COND(ent && ent->id >= 0, "trying to set invalid entity to have a render object"); 
    ent->obj = in;
    set_entity_rendered(ent, true);
}

static void set_entity_pos(skate_entity_t *ent, vec3 pos) {
    LOG_YELL_COND(ent && ent->id >= 0, "trying to set invalid entity to a new position"); 
    glm_vec3_copy(pos, ent->pos);
}

static void set_entity_rendered_pos_x(skate_entity_t *ent, r32 x) {
    LOG_YELL_COND(ent && ent && ent->id >= 0, "trying to set invalid entity to a new rendered position"); 
    set_render_obj_pos_x(ent->obj, x);
}

static void set_entity_rendered_pos_y(skate_entity_t *ent, r32 y) {
    LOG_YELL_COND(ent && ent && ent->id >= 0, "trying to set invalid entity to a new rendered position"); 
    set_render_obj_pos_y(ent->obj, y);
}

static void set_entity_rendered_pos_z(skate_entity_t *ent, r32 z) {
    LOG_YELL_COND(ent && ent && ent->id >= 0, "trying to set invalid entity to a new rendered position"); 
    set_render_obj_pos_z(ent->obj, z);
}

static void set_entity_rendered_pos(skate_entity_t *ent, vec3 pos){ 
    LOG_YELL_COND(ent && ent && ent->id >= 0, "trying to set invalid entity to a new rendered position"); 
    set_render_obj_pos(ent->obj, pos);
}

static void set_entity_rendered_rot(skate_entity_t *ent, vec3 rot) {
    LOG_YELL_COND(ent && ent->id >= 0, "trying to set invalid entity to a new rendered rotation"); 
    set_render_obj_rot(ent->obj, rot);
}

static void set_entity_rendered_scale(skate_entity_t *ent, vec3 scale) {
    LOG_YELL_COND(ent && ent->id >= 0, "trying to set invalid entity to a new rendered scale"); 
    set_render_obj_scale(ent->obj, scale);
} 

static void get_entity_model(skate_entity_t *ent, mat4 out) {
    LOG_PANIC_COND(ent && ent->rendered, "either invalid entity or entity is not set to rendered");
    
    skate_world_t *world = get_world();
    
    if(ent->obj->physics_enabled) {
        s_assert(ent->obj->jolt_obj.id >= 0);
        jolt_get_model_transform(&ent->obj->jolt_obj.id, out);
    } else {
        mat4 xm;
        mat4 ym;
        mat4 zm;
        mat4 rotation_a;
        mat4 rotation_b;
        glm_mat4_identity(xm);
        glm_mat4_identity(ym);
        glm_mat4_identity(zm);
        glm_mat4_identity(rotation_a);
        glm_mat4_identity(rotation_b);
        
        skate_render_mesh_t *mesh = ent->obj->mesh;
        LOG_PANIC_COND(mesh, "invalid mesh in entity object");
        
        glm_rotated_x(xm, mesh->rot[0], xm);
        glm_rotated_y(ym, mesh->rot[1], ym);
        glm_rotated_z(zm, mesh->rot[2], zm);
        glm_mat4_mul(xm, ym, rotation_a);
        glm_mat4_mul(rotation_a, zm, rotation_b);
        
        mat4 position;
        glm_mat4_identity(position);
        glm_translate_to(position, mesh->pos, position);
        
        mat4 scale;
        glm_mat4_identity(scale);
        glm_scale_to(scale, mesh->scale, scale);
        
        mat4 model;
        glm_mat4_identity(model);
        
        // L = T * R * S (right-left processesing by order of operation)
        // model = ((scale * rotation) * position)
        
        glm_mat4_mul(scale, rotation_b, model);
        glm_mat4_mul(model, position, out);
    }
}