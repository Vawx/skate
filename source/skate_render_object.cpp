
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
    obj->mesh->pos[0] = x;
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

// um_mat_trs @ Line: 388 - umath.h
static void mat_trs(vec3 translation, quat rotation, vec3 scale, mat4 out) {
	quat q;
    glm_quat_copy(rotation, q);
    
	float xx = q[0]*q[0], xy = q[0]*q[1], xz = q[0]*q[2], xw = q[0]*q[3];
	float yy = q[1]*q[1], yz = q[1]*q[2], yw = q[1]*q[3];
	float zz = q[2]*q[2], zw = q[2]*q[3];
	float sx = 2.0f * scale[0], sy = 2.0f * scale[1], sz = 2.0f * scale[2];
    
    glm_mat4_identity(out);
    out[0][0] = sx * (- yy - zz + 0.5f);
    out[0][1] = sy * (- zw + xy);
    out[0][2] = sz * (+ xz + yw);
    out[0][3] = translation[0];
    out[1][0] = sx * (+ xy + zw);
    out[1][1] = sy * (- xx - zz + 0.5f);
    out[1][2] = sz * (- xw + yz);
    out[1][3] = translation[1];
    out[2][0] = sx * (- yw + xz);
    out[2][1] = sy * (+ xw + yz);
    out[2][2] = sz * (- xx - yy + 0.5f);
    out[2][3] = translation[2];
    out[3][0] = 0.f;
    out[3][1] = 0.f;
    out[3][2] = 0.f;
    out[3][3] = 1.f;
}

static void tick_animation_track(skate_render_animation_t *anim, const float dt) {
    const r32 frame_time = (dt - anim->time_begin) * anim->framerate;
    s32 f0 = ((size_t)frame_time + 0, anim->num_frames - 1);
	s32 f1 = s_min((size_t)frame_time + 1, anim->num_frames - 1);
    r32 t = s_min(frame_time - (float)f0, 1.0f);
    
	for (s32  i = 0; i < anim->num_nodes; ++i) {
        skate_model_viewer_node_t *mesh_node = &anim->mesh_nodes[i];
        skate_model_anim_node_t *anim_node = &anim->anim_nodes[i];
        
        quat rot;
        if(anim_node->rot) {
            glm_quat_lerp(anim_node->rot[f0], anim_node->rot[f1], dt, rot);
        } else {
            glm_quat_copy(anim_node->const_rot, rot);
        }
        
        quat pos;
        if(anim_node->pos) {
            glm_vec3_lerp(anim_node->pos[f0], anim_node->pos[f1], dt, pos);
        } else {
            glm_vec3_copy(anim_node->const_pos, pos);
        }
        
        quat scale;
        if(anim_node->scale) {
            glm_vec3_lerp(anim_node->scale[f0], anim_node->scale[f1], dt, scale);
        } else {
            glm_vec3_copy(anim_node->const_scale, scale);
        }
        
        mat_trs(pos, rot, scale, mesh_node->node_to_parent);
	}
}