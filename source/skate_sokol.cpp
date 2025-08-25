
static void input_frame() {
    skate_sokol_t *sokol = get_sokol();
    for(int i = 0; i < input_event_list_t::events_size; ++i) {
        input_event_t *ev = &sokol->input.list.events[i];
        
        if(ev->type == INPUT_EVENT_TYPE_PRESSED) {
            ev->type = INPUT_EVENT_TYPE_HELD;
        }
        
        if(ev->type == INPUT_EVENT_TYPE_RELEASED) {
            ev->type = INPUT_EVENT_TYPE_NONE;
            ev->id = INPUT_KEY_NONE;
            ev->mb = INPUT_MOUSE_BUTTON_NONE;
            ev->device = INPUT_EVENT_DEVICE_NONE;
            ev->frame_id = sokol->last_frame_count;
        }
    }
}

bool is_key_pressed(sapp_keycode code) {
    skate_sokol_t *sokol = get_sokol();
    for(int i = 0; i < input_event_list_t::events_size; ++i) {
        input_event_t *ev = &sokol->input.list.events[i];
        if(ev->type & INPUT_EVENT_TYPE_PRESSED && ev->id == code) {
            return true;
        }
    }
    return false;
}

bool is_key_held(sapp_keycode code) {
    skate_sokol_t *sokol = get_sokol();
    for(int i = 0; i < input_event_list_t::events_size; ++i) {
        input_event_t *ev = &sokol->input.list.events[i];
        if(ev->type & INPUT_EVENT_TYPE_HELD && ev->id == code) {
            return true;
        }
    }
    return false;
}

bool is_key_released(sapp_keycode code) {
    skate_sokol_t *sokol = get_sokol();
    for(int i = 0; i < input_event_list_t::events_size; ++i) {
        input_event_t *ev = &sokol->input.list.events[i];
        if(ev->type & INPUT_EVENT_TYPE_RELEASED && ev->id == code) {
            return true;
        }
    }
    return false;
}

static skate_entity_t *get_entity_from_render_mesh(skate_render_mesh_t *mesh) {
    skate_render_obj_t *render_obj = (skate_render_obj_t*)mesh->outer;
    s_assert(render_obj != nullptr);
    
    skate_entity_t *ent = (skate_entity_t*)render_obj->outer;
    s_assert(ent != nullptr);
    return ent;
}

static skate_render_mesh_t *init_render_mesh(const skate_model_import_t *import, const int RENDER_PASS, void *outer) {
    skate_sokol_t *sokol = get_sokol();
    
    skate_render_mesh_t mm = {};
    for(int i = 0; i < import->import_result.num_parts; ++i) {
        sg_buffer_desc vert_buffer = {};
        vert_buffer.usage.vertex_buffer = true;
        vert_buffer.data.ptr = (skate_model_import_part_vertice_t*)import->import_result.parts[i].vertices_ptr;
        vert_buffer.data.size = import->import_result.parts[i].vertices_size;
        mm.bind.bindings[i].sgb.vertex_buffers[0] = sg_make_buffer(&vert_buffer);
        
        sg_buffer_desc index_buffer = {};
        index_buffer.usage.index_buffer = true;
        index_buffer.data.ptr = (u16*)import->import_result.parts[i].indices_ptr;
        index_buffer.data.size = import->import_result.parts[i].indices_count * sizeof(u32);
        mm.bind.bindings[i].sgb.index_buffer = sg_make_buffer(&index_buffer);
        mm.bind.bindings[i].indice_count = import->import_result.parts[i].indices_count;
        mm.bind.bindings_count = import->import_result.num_parts;
    }
    u32 idx = sokol->render_pass[RENDER_PASS].mesh_buffer.count();
    mm.outer = outer; // probably render_object
    push_buffer(&sokol->render_pass[RENDER_PASS].mesh_buffer, (u8*)&mm, sizeof(skate_render_mesh_t));
    return (skate_render_mesh_t*)&sokol->render_pass[RENDER_PASS].mesh_buffer.ptr[idx * sokol->render_pass[RENDER_PASS].mesh_buffer.type_size];
}

static void bind_image_data_to_render_mesh(skate_render_mesh_t *mesh, u8 *image_ptr, u32 image_width, u32 image_height, s32 image_idx, s32 sampler_idx, s8 binding_idx) {
    sg_image_desc image_desc = {};
    image_desc.width = image_width;
    image_desc.height = image_height;
    image_desc.data.subimage[0][0].ptr = image_ptr;
    image_desc.data.subimage[0][0].size = image_width * image_height * 4;
    if(binding_idx == -1) {
        for(int i = 0; i < mesh->bind.bindings_count; ++i) {
            mesh->bind.bindings[i].sgb.images[image_idx] = sg_make_image(&image_desc);
            sg_sampler_desc desc = {};
            mesh->bind.bindings[i].sgb.samplers[sampler_idx] = sg_make_sampler(&desc);
        }
    } else {
        LOG_PANIC_COND(binding_idx >= 0 && binding_idx < mesh->bind.bindings_count, "binding_idx out of range when trying to set image to sg_buffer");
        mesh->bind.bindings[binding_idx].sgb.images[image_idx] = sg_make_image(&image_desc);
        sg_sampler_desc desc = {};
        mesh->bind.bindings[binding_idx].sgb.samplers[sampler_idx] = sg_make_sampler(&desc);
    }
}

static void bind_image_to_mesh(skate_render_mesh_t *mesh, const skate_image_file_t *file) {
    bind_image_data_to_render_mesh(mesh, file->ptr, file->w, file->h, 0, 0);
}

static skate_view_t make_view(vec3 pos, vec3 dir, r32 fov) {
    skate_view_t result = {};
    result.fov = fov;
    glm_vec3_copy(pos, result.position);
    
    vec3 to_z;
    glm_vec3_sub(vec3{0,0,0}, result.position, to_z);
    glm_vec3_normalize_to(to_z, result.direction);
    update_view(&result);
    return result;
}

static void update_view(skate_view_t *view) {
    glm_vec3_normalize_to(view->direction, view->direction);
    glm_vec3_cross(vec3{0.f, 1.f, 0.f}, view->direction, view->right);
    glm_vec3_cross(view->direction, view->right, view->up);
    
    skate_sokol_t *sokol = get_sokol();
    
    if(view->is_ortho) {
        glm_ortho(0.f, sokol->widthf(), 0.f, sokol->heightf(), -1.f, 1.f, view->ortho);
    } else {
        glm_perspective(view->fov, sokol->aspect(), 1.f, 100.f, view->perspective);
    }
    
    vec3 fwd;
    glm_vec3_mulf(view->direction, view->movement_speed, fwd);
    glm_vec3_add(view->position, fwd, fwd);
    
    if(is_key_pressed(SKATE_KEYCODE_LOOK_ORIGIN) || is_key_held(SKATE_KEYCODE_LOOK_ORIGIN)) {
        vec3 to_z;
        glm_vec3_sub(vec3{0,0,0}, view->position, to_z);
        glm_vec3_normalize_to(to_z, view->direction);
        glm_lookat(view->position, view->direction, view->up, view->view);
    } else {
        glm_lookat(view->position, fwd, view->up, view->view);
    }
    
    if(view->func) {
        view->func((u8*)view);
    }
}

static void view_tick(skate_view_t *view) {
    skate_sokol_t *sokol = get_sokol();
    update_view(&sokol->default_view);
    
    // TODO(Kyle) all views, if needed...
}

static skate_sokol_t *get_sokol() {
    static skate_sokol_t sst;
    return &sst;
}

void bind_to_input(input_response_binding binding_func, input_event_list_filter_t *filter) {
    skate_sokol_t *sokol = get_sokol();
    
    sokol->input.bindings[sokol->input.bindings_idx].binding_func = binding_func;
    memcpy(sokol->input.bindings[sokol->input.bindings_idx].filter.key_events, filter->key_events, sizeof(filter->key_events));
    sokol->input.bindings[sokol->input.bindings_idx].filter.idx = filter->idx;
    ++sokol->input.bindings_idx;
}

#include <ctime>

static void init_default_level() {
    skate_directory_t tfdir = skate_directory_t("grid_64_64.png", SKATE_DIRECTORY_TEXTURE);
    skate_image_file_t tf = skate_image_file_t(&tfdir);
    
    {
        skate_directory_t dir = get_directory_for("plane.model", SKATE_DIRECTORY_MESH);
        
        for(int i = 0; i < 4; ++i) {
            skate_render_obj_t *obj = get_render_obj(&dir);
            skate_entity_t *ent = get_entity_rendered(obj);
            skate_render_mesh_t *mesh = obj->mesh;
            if(i+1 % 2 == 0) {
                set_entity_rendered_pos_z(ent, 30 * i);
            } else {
                set_entity_rendered_pos_x(ent, 30 * -i);
            }
            mesh->ignore_shadow = true;
            bind_image_to_mesh(mesh, &tf);
        }
    }
    
    {
        vec3 def_loc;
        def_loc[0] = 0.f;
        def_loc[1] = 5.f;
        def_loc[2] = 0.f;
        
        srand(time(0));
        
        for(int i = 0; i < 30; ++i) {
            r32 x = rand() % 10;
            r32 y = rand() % 5;
            r32 z = rand() % 10;
            r32 nx = rand() % 100;
            r32 nz = rand() % 100;
            
            skate_directory_t dir = get_directory_for("cube.model", SKATE_DIRECTORY_MESH);
            skate_render_obj_t *obj = get_render_obj(&dir);
            skate_entity_t *ent = get_entity_rendered(obj);
            
            r32 xp = def_loc[0] + nx > 49 ? x : -x;
            r32 yp = def_loc[1] + y;
            r32 zp = def_loc[2] + nz > 49 ? z : -z;;
            set_entity_rendered_pos(ent, vec3{xp, yp, zp});
            
            r32 xr = -nx;
            r32 yr = y;
            r32 zr = -nz;
            set_entity_rendered_rot(ent, vec3{xr, yr, zr});
            set_entity_rendered_scale(ent, vec3{1, 1, 1});
            
            bind_image_to_mesh(obj->mesh, &tf);
        }
    }
}

static void default_view_input_callback(const input_binding_key_or_button_t key_or_button, input_event_type input_type, input_event_list_filter_t *filter) {
    skate_sokol_t *sokol = get_sokol();
    skate_view_t *view = &sokol->default_view;
    
    r32 dt = sapp_frame_duration();
    
    if(key_or_button.device == INPUT_EVENT_DEVICE_KEYBOARD) {
        
        vec3 movement_len_fwd = {0};
        if(glm_vec3_len(view->velocity)) {
            glm_vec3_mulf(view->direction, view->movement_speed * view->movement_accel, movement_len_fwd);
        } else {
            glm_vec3_mulf(view->direction, view->movement_speed * view->movement_accel, movement_len_fwd);
        }
        
        vec3 movement_len_rgt = {0};
        glm_vec3_mulf(view->right, view->movement_speed, movement_len_rgt);
        
        switch(key_or_button.code) {
            case SAPP_KEYCODE_W: {
                glm_vec3_add(view->frame_impulse, movement_len_fwd, view->frame_impulse);
            } break;
            case SAPP_KEYCODE_A: {
                glm_vec3_add(view->frame_impulse, movement_len_rgt, view->frame_impulse);
            } break;
            case SAPP_KEYCODE_S: {
                glm_vec3_sub(view->frame_impulse, movement_len_fwd, view->frame_impulse);
            } break;
            case SAPP_KEYCODE_D: {
                glm_vec3_sub(view->frame_impulse, movement_len_rgt, view->frame_impulse);
            } break;
        }
    }
    if(key_or_button.device == INPUT_EVENT_DEVICE_MOUSE) {
        switch(key_or_button.mb) {
            case SAPP_MOUSEBUTTON_LEFT: {
                
            } break;
            case SAPP_MOUSEBUTTON_MIDDLE: {
                
            } break;
            case SAPP_MOUSEBUTTON_RIGHT: {
                r32 xoffset = sokol->global_mouse_pos_data.x - sokol->global_mouse_pos_data.lx;
                r32 yoffset = sokol->global_mouse_pos_data.ly - sokol->global_mouse_pos_data.y; 
                
                xoffset *= view->rotation_speed * sapp_frame_duration();
                yoffset *= view->rotation_speed * sapp_frame_duration();
                
                view->yaw   += xoffset;
                view->pitch += yoffset;
                
                if(view->pitch > 89.0f) {view->pitch = 89.0f;}
                if(view->pitch < -89.0f) {view->pitch = -89.0f;}
                
                vec3 direction = {0,0,0};
                direction[0] = cos(glm_rad(view->yaw)) * cos(glm_rad(view->pitch));
                direction[1] = sin(glm_rad(view->pitch));
                direction[2] = sin(glm_rad(view->yaw)) * cos(glm_rad(view->pitch));
                glm_normalize_to(direction, view->direction);
            } break;
        }
    }
}

static void init_input() {
    skate_sokol_t *sokol = get_sokol();
    for(int i = 0; i < input_event_list_t::events_size; ++i) {
        input_event_t *ev = &sokol->input.list.events[i];
        ev->type = INPUT_EVENT_TYPE_NONE;
        ev->id = INPUT_KEY_NONE;
        ev->mb = INPUT_MOUSE_BUTTON_NONE;
        ev->device = INPUT_EVENT_DEVICE_NONE;
        ev->frame_id = 0;
    }
}

static void default_view_velocity_tick(u8 *ptr) {
    skate_view_t *svt = (skate_view_t*)ptr;
    r32 dt = sapp_frame_duration();
    
    r32 new_vx = 0.f;
    r32 new_vy = 0.f;
    r32 new_vz = 0.f;
    
    r32 ll = glm_vec3_len(svt->frame_impulse);
    if(ll) {
        new_vx = svt->frame_impulse[0] * dt; 
        new_vy = svt->frame_impulse[1] * dt;
        new_vz = svt->frame_impulse[2] * dt;
        
        svt->velocity[0] += new_vx;
        svt->velocity[1] += new_vy;
        svt->velocity[2] += new_vz;
    } else {
        // friction to vel, when not moving from input
        svt->velocity[0] -= svt->movement_friction * svt->velocity[0] * dt;
        svt->velocity[1] -= svt->movement_friction * svt->velocity[1] * dt;
        svt->velocity[2] -= svt->movement_friction * svt->velocity[2] * dt;
        
    }
    
    // set pos
    vec3 new_pos = {0,0,0};
    new_pos[0] += svt->position[0] + (svt->velocity[0] * dt);
    new_pos[1] += svt->position[1] + (svt->velocity[1] * dt);
    new_pos[2] += svt->position[2] + (svt->velocity[2] * dt);
    
    svt->position[0] = new_pos[0];
    svt->position[1] = new_pos[1];
    svt->position[2] = new_pos[2];
    
    // max speed
    r32 len = glm_vec3_len(svt->velocity);
    if (len > svt->movement_speed) {
        r32 scale = svt->movement_speed / len;
        svt->velocity[0] *= scale;
        svt->velocity[1] *= scale;
        svt->velocity[2] *= scale;
    } 
    glm_vec3_zero(svt->frame_impulse);
}

static void init_default_view() {
    skate_sokol_t *sokol = get_sokol();
    sokol->default_view = make_view(vec3{0.f, 30.f, 20.f}, vec3{0.f, 0.f, 1.f}, 70);
    
    input_event_list_filter_t *flt = &sokol->default_view.filter;
    
    flt->key_events[flt->idx].listen_key = SKATE_KEYCODE_FWD;
    flt->key_events[flt->idx++].event_type = PRESSED_AND_HELD;
    
    flt->key_events[flt->idx].listen_key = SKATE_KEYCODE_LFT;
    flt->key_events[flt->idx++].event_type = PRESSED_AND_HELD;
    
    flt->key_events[flt->idx].listen_key = SKATE_KEYCODE_BCK;
    flt->key_events[flt->idx++].event_type = PRESSED_AND_HELD;
    
    flt->key_events[flt->idx].listen_key = SKATE_KEYCODE_RGT;
    flt->key_events[flt->idx++].event_type = PRESSED_AND_HELD;
    
    flt->key_events[flt->idx].listen_mb = SKATE_BUTTON_RIGHT;
    flt->key_events[flt->idx].event_type = PRESSED_AND_HELD;
    flt->key_events[flt->idx++].device = INPUT_EVENT_DEVICE_MOUSE;
    
    bind_to_input((input_response_binding)default_view_input_callback, &sokol->default_view.filter);
    sokol->default_view.func = default_view_velocity_tick;
}

static void sokol_init() {
    skate_sokol_t *sokol = get_sokol();
    
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);
    
    sokol->default_pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    sokol->default_pass_action.colors[0].clear_value = {0.4f, 0.4f, 0.4f, 1.f};
    
    // render pass static
    {
        sg_shader shd = sg_make_shader(texture_static_mesh_shader_desc(sg_query_backend()));
        sg_pipeline_desc pipeline_desc = {};
        pipeline_desc.layout.attrs[ATTR_texture_static_mesh_a_position].format = SG_VERTEXFORMAT_FLOAT3;
        pipeline_desc.layout.attrs[ATTR_texture_static_mesh_a_normal].format = SG_VERTEXFORMAT_FLOAT3;
        pipeline_desc.layout.attrs[ATTR_texture_static_mesh_a_uv].format = SG_VERTEXFORMAT_FLOAT2;
        pipeline_desc.layout.attrs[ATTR_texture_static_mesh_a_vertex_index].format = SG_VERTEXFORMAT_FLOAT;
        pipeline_desc.shader = shd;
        pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
        pipeline_desc.cull_mode = SG_CULLMODE_BACK;
        pipeline_desc.face_winding = SG_FACEWINDING_CCW;
        pipeline_desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
        pipeline_desc.depth.write_enabled = true;
        pipeline_desc.sample_count = 2;
        pipeline_desc.label = "static_pass";
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].pipeline = sg_make_pipeline(&pipeline_desc);
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].pass_action.colors[0].clear_value = {0.4f, 0.4f, 0.4f, 1.f};
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].mesh_buffer = alloc_buffer(sizeof(skate_render_mesh_t), MAX_MESHES_PER_RENDER_PASS);
        glm_vec3_copy(vec3{-15, 40, -12}, sokol->render_pass[RENDER_PASS::TYPE_STATIC].light_pos);
        glm_vec3_sub(vec3{0,0,0}, sokol->render_pass[RENDER_PASS::TYPE_STATIC].light_pos, sokol->render_pass[RENDER_PASS::TYPE_STATIC].light_dir);
        
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].pass[0].action = sokol->render_pass[RENDER_PASS::TYPE_STATIC].pass_action;
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].pass[0].swapchain = sglue_swapchain();;
        
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].active = 1;
    }
    
    // render pass shadow
    {
        sg_shader sshd = sg_make_shader(shadow_shader_desc(sg_query_backend()));
        // images
        sg_image_desc shadow_image_desc = {};
        shadow_image_desc.usage.render_attachment = true;
        shadow_image_desc.type = SG_IMAGETYPE_ARRAY;
        shadow_image_desc.width = map_sizes[0];
        shadow_image_desc.height = map_sizes[0];
        shadow_image_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
        shadow_image_desc.num_slices = CASCADE_LEVEL_COUNT;
        shadow_image_desc.sample_count = 1;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].map = sg_make_image(&shadow_image_desc);
        sg_sampler_desc shadow_smp_desc = {};
        shadow_smp_desc.min_filter = SG_FILTER_NEAREST;
        shadow_smp_desc.mag_filter = SG_FILTER_NEAREST;
        shadow_smp_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
        shadow_smp_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].smp = sg_make_sampler(&shadow_smp_desc);
        
        // pipeline
        sg_pipeline_desc shadow_pipeline_desc = {};
        
        shadow_pipeline_desc.layout.attrs[ATTR_shadow_a_position].format = SG_VERTEXFORMAT_FLOAT3;
        shadow_pipeline_desc.layout.attrs[ATTR_shadow_a_normal].format = SG_VERTEXFORMAT_FLOAT3;
        shadow_pipeline_desc.layout.attrs[ATTR_shadow_a_uv].format = SG_VERTEXFORMAT_FLOAT2;
        shadow_pipeline_desc.layout.attrs[ATTR_shadow_a_vertex_index].format = SG_VERTEXFORMAT_FLOAT;
        shadow_pipeline_desc.shader = sshd;
        shadow_pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
        shadow_pipeline_desc.cull_mode = SG_CULLMODE_BACK;
        shadow_pipeline_desc.sample_count = 1;
        shadow_pipeline_desc.depth.pixel_format = SG_PIXELFORMAT_DEPTH;
        shadow_pipeline_desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
        shadow_pipeline_desc.depth.write_enabled = true;
        shadow_pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
        
        // CRITICAL: if this is not set to NONE, then color_count is overriden to 1 causing an assert/crash
        shadow_pipeline_desc.colors[0].pixel_format = SG_PIXELFORMAT_NONE; 
        shadow_pipeline_desc.color_count = 0;
        
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pipeline = sg_make_pipeline(&shadow_pipeline_desc);
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action.depth.load_action = SG_LOADACTION_CLEAR;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action.depth.store_action = SG_STOREACTION_STORE;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action.depth.clear_value = 1.f;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action.colors[0].load_action = SG_LOADACTION_DONTCARE;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action.colors[0].store_action = SG_STOREACTION_DONTCARE;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action.colors[0].clear_value = sg_color{0,0,0};
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].shared_buffer = &sokol->render_pass[RENDER_PASS::TYPE_STATIC].mesh_buffer;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].use_atts_no_swapchain = true;
        
        glm_vec3_copy(vec3{-15, 40, -12}, sokol->render_pass[RENDER_PASS::TYPE_SHADOW].light_pos);
        glm_vec3_sub(vec3{0,0,0}, sokol->render_pass[RENDER_PASS::TYPE_SHADOW].light_pos, sokol->render_pass[RENDER_PASS::TYPE_SHADOW].light_dir);
        
        for(int i = 0; i < CASCADE_LEVEL_COUNT; ++i) {
            sg_attachments_desc att_desc = {0};
            att_desc.depth_stencil.image = sokol->render_pass[RENDER_PASS::TYPE_SHADOW].map;
            att_desc.depth_stencil.slice = i;
            sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass[i].action = sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action;
            sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass[i].attachments = sg_make_attachments(&att_desc);
        }
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].active = 1;
    }
    
    // create a checkerboard texture, default texture (whats shown when no image data exists on a mesh binding)
    const u32 pixels[4*4] = {
        0xFFa0a0a0, 0xFF1c1c1c, 0xFFa0a0a0, 0xFF1c1c1c,
        0xFF1c1c1c, 0xFFa0a0a0, 0xFF1c1c1c, 0xFFa0a0a0,
        0xFFa0a0a0, 0xFF1c1c1c, 0xFFa0a0a0, 0xFF1c1c1c,
        0xFF1c1c1c, 0xFFa0a0a0, 0xFF1c1c1c, 0xFFa0a0a0,
    };
    
    sg_image_desc image_desc = {};
    image_desc.width = 4;
    image_desc.height = 4;
    image_desc.data.subimage[0][0] = SG_RANGE(pixels);
    sokol->default_image = sg_make_image(&image_desc);
    sg_sampler_desc sampler = {};
    sokol->default_texture = sg_make_sampler(&sampler);
    
    init_input();
    init_default_view();
    
    skate_jolt_init(get_jolt());
    
    // default test level
    init_default_level();
}

static void render_mesh_for(int PASS, int binding_idx, int slice_id, skate_render_mesh_t *mesh) {
    skate_sokol_t *sokol = get_sokol();
    switch(PASS) {
        case RENDER_PASS::TYPE_STATIC: {
            if(mesh->bind.bindings[binding_idx].sgb.images[IMG_tex].id == 0) {
                mesh->bind.bindings[binding_idx].sgb.images[IMG_tex] = sokol->default_image;
                mesh->bind.bindings[binding_idx].sgb.samplers[SMP_smp] = sokol->default_texture;
            }
            
            mesh->bind.bindings[binding_idx].sgb.images[IMG_shadow_tex] = sokol->render_pass[RENDER_PASS::TYPE_SHADOW].map;
            mesh->bind.bindings[binding_idx].sgb.samplers[SMP_shadow_smp] = sokol->render_pass[RENDER_PASS::TYPE_SHADOW].smp;
            
            sg_apply_bindings(mesh->bind.bindings[binding_idx].sgb);
            sg_draw(0, mesh->bind.bindings[binding_idx].indice_count, 1);
        } break;
        case RENDER_PASS::TYPE_SHADOW: {
            vs_shadow_params_t shadow_params = {};
            
            mat4 model;
            glm_mat4_identity(model);
            
            skate_entity_t *ent = get_entity_from_render_mesh(mesh);
            get_entity_model(ent, model);
            
            glm_mat4_mul(sokol->light_view_projections[slice_id], model, shadow_params.light_view_mvp);
            sg_apply_uniforms(UB_vs_shadow_params, &SG_RANGE(shadow_params));
            
            sg_apply_bindings(mesh->bind.bindings[binding_idx].sgb);
            sg_draw(0, mesh->bind.bindings[binding_idx].indice_count, 1);
        } break;
    }
}

static bool apply_uniforms_for(const int PASS, int idx, int i, skate_render_mesh_t *mesh) {
    skate_sokol_t *sokol = get_sokol();
    
    bool result = false;
    
    mat4 view_projection;
    glm_mat4_identity(view_projection);
    {
        glm_mat4_mul(sokol->default_view.perspective, sokol->default_view.view, view_projection);
    }
    
    sg_pass pass = {};
    switch(PASS) {
        case RENDER_PASS::TYPE_STATIC: {
            texture_static_mesh_vs_params_t params = {};
            
            skate_entity_t *ent = get_entity_from_render_mesh(mesh);
            get_entity_model(ent, params.model);
            
            glm_mat4_mul(view_projection, params.model, params.mvp);
            sg_apply_uniforms(UB_texture_static_mesh_vs_params, &SG_RANGE(params));
            
            // fs
            texture_static_mesh_fs_params_t fs_params = {};
            glm_vec3_copy(sokol->render_pass[PASS].light_pos, fs_params.eye_pos);
            glm_vec3_copy(sokol->render_pass[PASS].light_dir, fs_params.light_dir);
            glm_vec3_copy(vec3{1.f,1.f,1.f}, fs_params.light_color);
            glm_vec3_copy(vec3{0.5f, 0.5f, 0.5f}, fs_params.light_ambient);
            
            for(int i = 0; i < CASCADE_LEVEL_COUNT; ++i) {
                glm_mat4_copy(sokol->light_view_projections[i], fs_params.light_view_proj[i]);
                
                vec4 cssms = {0};
                cssms[0] = map_sizes[0];
                cssms[1] = map_sizes[0];
                cssms[2] = CAMERA_FAR_PLANE;
                cssms[3] = shadow_cascade_levels[i];
                glm_vec4_copy(cssms, fs_params.cascade_splits_shadow_map_size[i]);
            }
            
            glm_mat4_copy(sokol->default_view.view, fs_params.view);
            glm_normalize_to(sokol->render_pass[PASS].light_dir, fs_params.light_dir);
            sg_apply_uniforms(UB_texture_static_mesh_fs_params, &SG_RANGE(fs_params));
            
            render_mesh_for(PASS, idx, /*slice_id*/0, mesh);
            
            result = true;
        } break;
        case RENDER_PASS::TYPE_SHADOW: {
            
            result = true;
        } break;
        case RENDER_PASS::TYPE_DYNAMIC: {
        } break;
    }
    
    return result;
}

static skate_buffer_t *get_buffer_for_pass(int PASS) {
    skate_sokol_t *sokol = get_sokol();
    
    skate_buffer_t *mesh_buffer = nullptr;
    if(sokol->render_pass[PASS].shared_buffer != nullptr) {
        mesh_buffer = sokol->render_pass[PASS].shared_buffer;
    } else {
        mesh_buffer = &sokol->render_pass[PASS].mesh_buffer;
    }
    LOG_PANIC_COND(mesh_buffer != nullptr, "render_pass has no mesh buffer");
    return mesh_buffer;
}

static vec4 *get_frustrum_corners_world_space(mat4 proj, vec4 result[6]) {
    skate_sokol_t *sokol = get_sokol();
    
    mat4 proj_view;
    glm_mat4_identity(proj_view);
    glm_mat4_mul(sokol->default_view.view, proj, proj_view);
    
    mat4 inv;
    glm_mat4_identity(inv);
    glm_mat4_inv(proj_view, inv);
    
    int idx = 0;
    for (u32 x = 0; x < 2; ++x) {
        for (u32 y = 0; y < 2; ++y) {
            for (u32 z = 0; z < 2; ++z) {
                vec4 val = {2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f};
                glm_mat4_mulv(inv, val, result[idx++]);
            }
        }
    }
    
    return result;
}

static void get_light_view_projection_matrix(mat4 out, const r32 np, const r32 fp, const int i) {
    skate_sokol_t *sokol = get_sokol();
    
    vec4 result[6] = {0};
    vec4 *result_ptr = result;
    
    mat4 proj;
    glm_mat4_identity(proj);
    glm_perspective(glm_rad(40), map_sizes[i] / map_sizes[i], np, fp, proj); 
    
    result_ptr = get_frustrum_corners_world_space(proj, result);
    
    vec3 center = {0.f,0.f,0.f};
    for(int i = 0; i < 6; ++i) {
        vec3 rr = {result[i][0], result[i][1], result[i][2]};
        glm_vec3_add(center, rr, center);
    }
    glm_vec3_divs(center, 6.f, center);
    
    vec3 offset;
    glm_vec3_add(center, sokol->render_pass[RENDER_PASS::TYPE_SHADOW].light_dir, offset);
    
    mat4 light_view;
    glm_mat4_identity(light_view);
    glm_lookat(offset, center, vec3{0,1,0}, light_view);
    
    r32 min_x = 100000;
    r32 max_x = -100000;
    r32 min_y = 100000;
    r32 max_y = -100000;
    r32 min_z = 100000;
    r32 max_z = -100000;
    for (int i = 0; i < 6; ++i) {
        vec4 trf;
        glm_mat4_mulv(light_view, result[i], trf);
        
        min_x = glm_min(min_x, trf[0]);
        max_x = glm_max(max_x, trf[0]);
        min_y = glm_min(min_y, trf[1]);
        max_y = glm_max(max_y, trf[1]);
        min_z = glm_min(min_z, trf[2]);
        max_z = glm_max(max_z, trf[2]);
    }
    
    // NOTE(Kyle): Tune this parameter according to the scene
    r32 mult_z = 10.0f;
    if (min_z < 0) {
        min_z *= mult_z;
    } else {
        min_z /= mult_z;
    } if (max_z < 0) {
        max_z /= mult_z;
    } else {
        max_z *= mult_z;
    }
    
    mat4 light_projection;
    glm_mat4_identity(light_projection);
    glm_ortho(min_x, max_x, min_y, max_y, min_z, max_z, light_projection);
    
    glm_mat4_identity(result);
    glm_mat4_mul(light_projection, light_view, out);
}

static void render_loop() {
    skate_sokol_t *sokol = get_sokol();
    
    // shadow cascades
    for(int i = 0; i < CASCADE_LEVEL_COUNT + 1; ++i) {
        glm_mat4_identity(sokol->light_view_projections[i]);
        if(i == 0) {
            get_light_view_projection_matrix(sokol->light_view_projections[i], CAMERA_NEAR_PLANE, shadow_cascade_levels[i], i);
        } else if(i < CASCADE_LEVEL_COUNT) {
            get_light_view_projection_matrix(sokol->light_view_projections[i], shadow_cascade_levels[i - 1], shadow_cascade_levels[i], i);
        } else {
            get_light_view_projection_matrix(sokol->light_view_projections[i], shadow_cascade_levels[i - 1], CAMERA_FAR_PLANE, i);
        }
    }
    
    for(int pass_idx = 0; pass_idx < RENDER_PASS::TYPE_COUNT; ++pass_idx) {
        if(!sokol->render_pass[pass_idx].active) {continue;} // not active? skip
        
        // shadow
        if(pass_idx == RENDER_PASS::TYPE_SHADOW) {
            for(int i = 0; i < CASCADE_LEVEL_COUNT; ++i) {
                sg_begin_pass(&sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass[i]);
                sg_apply_pipeline(sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pipeline);
                
                skate_buffer_t *mesh_buffer = get_buffer_for_pass(pass_idx);
                for(int m = 0; m < mesh_buffer->count(); ++m) { 
                    skate_render_mesh_t *mesh = mesh_buffer->as_idx<skate_render_mesh_t>(m);
                    if(mesh->ignore_shadow) {continue;}
                    
                    for(int p = 0; p < mesh->bind.bindings_count; ++p) {
                        if(apply_uniforms_for(pass_idx, p, i, mesh)) {
                            render_mesh_for(RENDER_PASS::TYPE_SHADOW, p, i, mesh); 
                        }
                    }
                }
                
                sg_end_pass();
            }
        }
        if(pass_idx == RENDER_PASS::TYPE_STATIC) {
            sg_begin_pass(sokol->render_pass[pass_idx].pass);
            sg_apply_pipeline(sokol->render_pass[pass_idx].pipeline);
            
            skate_buffer_t *mesh_buffer = get_buffer_for_pass(pass_idx);
            for(int m = 0; m < mesh_buffer->count(); ++m) {
                skate_render_mesh_t *mesh = mesh_buffer->as_idx<skate_render_mesh_t>(m);
                for(int p = 0; p < mesh->bind.bindings_count; ++p) {
                    if(apply_uniforms_for(pass_idx, p, -1, mesh)) {}
                }
            }
            
            sg_end_pass();
            
        }
    }
    sg_commit();
}

static bool passes_filter(input_event_list_filter_t *filter, input_binding_key_or_button_t key_or_button, input_event_type event_type) {
    if(!filter) {return true;} /// passing null is no filter.
    
    if(filter->idx == 0) {
        LOG_YELL("passing in a filter that has no elements. pass nullptr if for some reason this has to be called with an empty one");
        return false;
    }
    
    // loop all filter events up to its idx
    // each filter has 1 key and 1 event type (pressed, held, etc)
    // ie: filter for 'w' on pressed and 'a' on released is easy
    
    /* TODO(Kyle): 
    1): pressing 1 key / button cancels the input of another key / button 
     2): input modifiers (alt, shift, control + some key)
 controller input modifiers (any_button + any_other_button (or maybe dpad + any_other_button))
*/
    
    for(int i = 0; i < filter->idx; ++i) {
        input_event_filter_item_t *item = &filter->key_events[i];
        if(item->device == INPUT_EVENT_DEVICE_KEYBOARD) {
            if(item->listen_key == key_or_button.code) {
                if (item->event_type == INPUT_EVENT_TYPE_NONE) {
                    return false;
                }
                if (item->event_type & event_type || item->event_type & INPUT_EVENT_TYPE_ANY) {
                    return true;
                }
            }
        }
        if(item->device == INPUT_EVENT_DEVICE_MOUSE) {
            if(item->listen_mb == key_or_button.mb) {
                if(item->event_type == INPUT_EVENT_TYPE_NONE) {
                    return false;
                }
                if(item->event_type & event_type || item->event_type & INPUT_EVENT_TYPE_ANY) {
                    return true;
                }
            }
        }
    }
    return false;
}

static void sokol_frame() {
    skate_sokol_t *sokol = get_sokol();
    
    for(int i = 0; i < sokol->input.bindings_idx; ++i) {
        response_binding_handle_t *bind = &sokol->input.bindings[i];
        for(int j = 0; j < input_event_list_t::events_size; ++j) {
            input_event_t *ev = &sokol->input.list.events[j];
            if(ev->id == INPUT_KEY_NONE) {continue;}
            input_binding_key_or_button_t kob = {};
            kob.device = ev->device;
            kob.code = ev->device == INPUT_EVENT_DEVICE_KEYBOARD ? (sapp_keycode)ev->id : (sapp_keycode)ev->mb;
            memcpy(&kob.mouse_pos_data, &ev->mouse_pos_data, sizeof(input_mouse_pos_data_t));
            
            if(passes_filter(&bind->filter, kob, ev->type)) {
                bind->binding_func(kob, ev->type, &bind->filter);
            }
        }
    }
    
    skate_jolt_frame(get_jolt());
    
    view_tick(&sokol->default_view);
    render_loop();
    
    input_frame(); // this is not polling for input, it is cleanup 
}

static void sokol_cleanup() {
    skate_sokol_t *sokol = get_sokol();
    free_buffer(&sokol->render_pass[RENDER_PASS::TYPE_STATIC].mesh_buffer);
    free_buffer(&sokol->render_pass[RENDER_PASS::TYPE_SHADOW].mesh_buffer);
    free_buffer(&sokol->render_pass[RENDER_PASS::TYPE_DYNAMIC].mesh_buffer);
    
    skate_jolt_shutdown(get_jolt());
    
    logger_shutdown();
    sg_shutdown();
}

static void sokol_event(const sapp_event *event) {
    skate_sokol_t *sokol = get_sokol();
    sokol->last_frame_count = event->frame_count;
    
    switch(event->type) {
        case SAPP_EVENTTYPE_KEY_DOWN: {
            if(!is_key_pressed(event->key_code) && !is_key_held(event->key_code) && !is_key_released(event->key_code)) {
                for(int i = 0; i < input_event_list_t::events_size; ++i) {
                    input_event_t *ev = &sokol->input.list.events[i];
                    if(ev->id == INPUT_KEY_NONE) {
                        ev->type = INPUT_EVENT_TYPE_PRESSED;
                        ev->id = event->key_code;
                        ev->device = INPUT_EVENT_DEVICE_KEYBOARD;
                        ev->frame_id = sokol->last_frame_count;
                        break;
                    }
                }
                break;
            }
        } break;
        case SAPP_EVENTTYPE_KEY_UP: {
            if(is_key_pressed(event->key_code) || is_key_held(event->key_code)) {
                for(int i = 0; i < input_event_list_t::events_size; ++i) {
                    input_event_t *ev = &sokol->input.list.events[i];
                    if(ev->id == event->key_code && ev->frame_id != sokol->last_frame_count) {
                        ev->type = INPUT_EVENT_TYPE_RELEASED;
                    }
                }
            } else {
                LOG_YELL("releasing a key that doesnt appear to be pressed or held!");
            }
        } break;
        case SAPP_EVENTTYPE_CHAR: {
            
        } break;
        case SAPP_EVENTTYPE_MOUSE_DOWN: {
            for(int i = 0; i < input_event_list_t::events_size; ++i) {
                input_event_t *ev = &sokol->input.list.events[i];
                if(ev->mb == event->mouse_button) {
                    return;
                }
            }
            
            for(int i = 0; i < input_event_list_t::events_size; ++i) {
                input_event_t *ev = &sokol->input.list.events[i];
                if(ev->mb == INPUT_KEY_NONE) {
                    ev->type = INPUT_EVENT_TYPE_PRESSED;
                    ev->mb = (input_mouse_button)event->mouse_button;
                    ev->device = INPUT_EVENT_DEVICE_MOUSE;
                    ev->frame_id = sokol->last_frame_count;
                    ev->mouse_pos_data.dx = sokol->global_mouse_pos_data.dx;
                    ev->mouse_pos_data.dy = sokol->global_mouse_pos_data.dy;
                    ev->mouse_pos_data.x = sokol->global_mouse_pos_data.x;
                    ev->mouse_pos_data.y = sokol->global_mouse_pos_data.y;
                    break;
                }
            }
        } break;
        case SAPP_EVENTTYPE_MOUSE_UP: {
            bool found = false;
            for(int i = 0; i < input_event_list_t::events_size; ++i) {
                input_event_t *ev = &sokol->input.list.events[i];
                if(ev->mb == event->mouse_button) {
                    found = true;
                    break;
                }
            }
            
            if(found) {
                for(int i = 0; i < input_event_list_t::events_size; ++i) {
                    input_event_t *ev = &sokol->input.list.events[i];
                    if(ev->mb == event->mouse_button) {
                        ev->type = INPUT_EVENT_TYPE_RELEASED;
                        ev->frame_id = sokol->last_frame_count;
                        break;
                    }
                }
            }
            
            memset(&sokol->global_mouse_pos_data, 0, sizeof(input_mouse_pos_data_t));
        } break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL: {
            
        } break;
        case SAPP_EVENTTYPE_MOUSE_MOVE: {
            sokol->global_mouse_pos_data.dx = sokol->global_mouse_pos_data.x - event->mouse_x;
            sokol->global_mouse_pos_data.dy = sokol->global_mouse_pos_data.y - event->mouse_y;
            sokol->global_mouse_pos_data.lx = sokol->global_mouse_pos_data.x;
            sokol->global_mouse_pos_data.ly = sokol->global_mouse_pos_data.y;
            sokol->global_mouse_pos_data.x = event->mouse_x;
            sokol->global_mouse_pos_data.y = event->mouse_y;
            
        } break;
        case SAPP_EVENTTYPE_MOUSE_ENTER: {
            
        } break;
        case SAPP_EVENTTYPE_MOUSE_LEAVE: {
            
        } break;
        case SAPP_EVENTTYPE_TOUCHES_BEGAN: {
            
        } break;
        case SAPP_EVENTTYPE_TOUCHES_MOVED: {
            
        } break;
        case SAPP_EVENTTYPE_TOUCHES_ENDED: {
            
        } break;
        case SAPP_EVENTTYPE_TOUCHES_CANCELLED: {
            
        } break;
        case SAPP_EVENTTYPE_RESIZED: {
            
        } break;
        case SAPP_EVENTTYPE_ICONIFIED: {
            
        } break;
        case SAPP_EVENTTYPE_RESTORED: {
            
        } break;
        case SAPP_EVENTTYPE_FOCUSED: {
            
        } break;
        case SAPP_EVENTTYPE_UNFOCUSED: {
            
        } break;
        case SAPP_EVENTTYPE_SUSPENDED: {
            
        } break;
        case SAPP_EVENTTYPE_RESUMED: {
            
        } break;
        case SAPP_EVENTTYPE_QUIT_REQUESTED: {
            
        } break;
        case SAPP_EVENTTYPE_CLIPBOARD_PASTED: {
            
        } break;
        case SAPP_EVENTTYPE_FILES_DROPPED: {
        } break;
    }
}
