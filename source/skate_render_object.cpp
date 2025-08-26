
static skate_render_t *get_render() {
    static skate_render_t render;
    if(render.obj_buffer.ptr == nullptr) {
        render.obj_buffer = alloc_buffer(sizeof(skate_render_obj_t), SKATE_RENDER_DEFAULT_RENDER_OBJECT_COUNT);
        for(int i = 0; i < SKATE_RENDER_DEFAULT_RENDER_OBJECT_COUNT; ++i) {
            skate_render_obj_t *obj = render.obj_buffer.as_idx<skate_render_obj_t>(i);
            obj->id = -1;
        }
    }
    return &render;
}

static skate_render_obj_t *get_render_obj(const skate_directory_t *dir) {
    LOG_PANIC_COND(dir->get() != nullptr, "Invalid directory for render object");
    
    skate_render_t *render = get_render();
    skate_render_obj_t *obj = nullptr;
    
    s32 cc = 0;
    do {
        obj = render->obj_buffer.as_idx<skate_render_obj_t>(cc++);
        if(cc >= SKATE_RENDER_DEFAULT_RENDER_OBJECT_COUNT) __debugbreak(); // out of memory
    } while(obj->id >= 0);
    
    skate_import_t *im = skate_import_t::get();
    skate_model_import_t *model_import = im->get_or_load_model(dir);
    obj->mesh = init_render_mesh(model_import, RENDER_PASS::TYPE_STATIC);
    obj->mesh->outer = obj; 
    obj->id = cc;
    return obj;
}

static void set_render_obj_pos_x(skate_render_obj_t *obj, r32 x) {
    
    LOG_PANIC_COND(obj && obj->id >= 0, "INVALID render object trying to set physics");obj->mesh->pos[0] = x;
}

static void set_render_obj_pos_y(skate_render_obj_t *obj, r32 y) {
    LOG_PANIC_COND(obj && obj->id >= 0, "INVALID render object trying to set physics");
    obj->mesh->pos[1] = y;
}

static void set_render_obj_pos_z(skate_render_obj_t *obj, r32 z) {
    LOG_PANIC_COND(obj && obj->id >= 0, "INVALID render object trying to set physics");
    obj->mesh->pos[2] = z;
}

static void set_render_obj_physics_enabled(skate_render_obj_t *obj, u8 in) {
    LOG_PANIC_COND(obj && obj->id >= 0, "INVALID render object trying to set physics");
    obj->physics_enabled = in;
}

static void set_render_obj_pos(skate_render_obj_t *obj, vec3 in) {
    LOG_PANIC_COND(obj && obj->id >= 0, "INVALID render object trying to set position");
    glm_vec3_copy(in, obj->mesh->pos);
}

static void set_render_obj_rot(skate_render_obj_t *obj, vec3 in) {
    LOG_PANIC_COND(obj && obj->id >= 0, "INVALID render object trying to set rotation");
    glm_vec3_copy(in, obj->mesh->rot);
}

static void set_render_obj_scale(skate_render_obj_t *obj, vec3 in) {
    LOG_PANIC_COND(obj && obj->id >= 0, "INVALID render object trying to set scale");
    glm_vec3_copy(in, obj->mesh->scale);
}

static void get_render_obj_pos(skate_render_obj_t *obj, vec3 out) {
    LOG_PANIC_COND(obj && obj->id >= 0, "INVALID render object trying to get pos");
    if(obj->physics_enabled) {
        jolt_get_body_com_position(&obj->jolt_obj.id, out);
    } else {
        glm_vec3_copy(obj->mesh->pos, out);
    }
}