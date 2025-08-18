
static skate_render_mesh_t *init_render_mesh(const skate_model_import_t *import, const int RENDER_PASS) {
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
                vec4 pt;
                glm_mat4_mulv(inv, val, result[idx++]);
            }
        }
    }
    
    return result;
}

static void get_light_view_projection_matrix(mat4 out, const r32 np, const r32 fp) {
    skate_sokol_t *sokol = get_sokol();
    
    mat4 proj;
    glm_mat4_identity(proj);
    glm_perspective(glm_rad(sokol->default_view.fov), sapp_widthf() / sapp_heightf(), np, fp, proj);
    
    vec4 result[6] = {0};
    vec4 *result_ptr = result;
    result_ptr = get_frustrum_corners_world_space(proj, result);
    
    vec3 center = {0.f,0.f,0.f};
    for(int i = 0; i < 6; ++i) {
        vec3 rr = {result[i][0], result[i][1], result[i][2]};
        glm_vec3_add(center, rr, center);
    }
    glm_vec3_divs(center, 6.f, center);
    
    vec3 light_direction;
    vec3 offset;
    
    glm_vec3_sub(vec3{0, 0, 0}, sokol->render_pass[RENDER_PASS::TYPE_SHADOW].light_pos, light_direction);
    glm_vec3_normalize_to(light_direction, light_direction);
    glm_vec3_add(center, light_direction, offset);
    
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

static skate_sokol_t *get_sokol() {
    static skate_sokol_t sst;
    return &sst;
}

static void init_default_level() {
    skate_directory_t tfdir = skate_directory_t("grid_64_64.png", SKATE_DIRECTORY_TEXTURE);
    skate_image_file_t tf = skate_image_file_t(&tfdir);
    
    {
        skate_directory_t dir = get_directory_for("plane.model", SKATE_DIRECTORY_MESH);
        skate_model_import_t import = skate_model_import_t(&dir);
        skate_render_mesh_t *mesh = init_render_mesh(&import, RENDER_PASS::TYPE_STATIC);
        bind_image_to_mesh(mesh, &tf);
    }
    
    {
        skate_directory_t dir = get_directory_for("cube.model", SKATE_DIRECTORY_MESH);
        skate_model_import_t import = skate_model_import_t(&dir);
        skate_render_mesh_t *mesh = init_render_mesh(&import, RENDER_PASS::TYPE_STATIC);
        mesh->position[1] = 5.f;
        bind_image_to_mesh(mesh, &tf);
    }
}

static void sokol_init() {
    skate_sokol_t *sokol = get_sokol();
    
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);
    
    sokol->default_pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    sokol->default_pass_action.colors[0].clear_value = {0.4f, 0.4f, 0.4f, 1.f};
    
    sokol->default_view = make_view(vec3{0.f, 30.f, 20.f}, vec3{0.f, 0.f, 0.f}, 70);
    
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
        pipeline_desc.label = "static_pass";
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].pipeline = sg_make_pipeline(&pipeline_desc);
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].pass_action.colors[0].clear_value = {0.4f, 0.4f, 0.4f, 1.f};
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].mesh_buffer = alloc_buffer(sizeof(skate_render_mesh_t), MAX_MESHES_PER_RENDER_PASS);
        glm_vec3_copy(vec3{50, 50, -50}, sokol->render_pass[RENDER_PASS::TYPE_STATIC].light_pos);
        glm_vec3_sub(vec3{0,0,0}, sokol->render_pass[RENDER_PASS::TYPE_STATIC].light_pos, sokol->render_pass[RENDER_PASS::TYPE_STATIC].light_dir);
        sokol->render_pass[RENDER_PASS::TYPE_STATIC].active = 1;
    }
    
    // render pass shadow
    {
        sg_shader sshd = sg_make_shader(shadow_shader_desc(sg_query_backend()));
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
        shadow_pipeline_desc.colors[0].pixel_format = SG_PIXELFORMAT_NONE;
        shadow_pipeline_desc.label = "shadow_pass";
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pipeline = sg_make_pipeline(&shadow_pipeline_desc);
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action.depth.load_action = SG_LOADACTION_CLEAR;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action.depth.store_action = SG_STOREACTION_STORE;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass_action.depth.clear_value = 1.f;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].shared_buffer = &sokol->render_pass[RENDER_PASS::TYPE_STATIC].mesh_buffer;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].use_atts_no_swapchain = true;
        
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
        shadow_smp_desc.min_filter = SG_FILTER_LINEAR;
        shadow_smp_desc.mag_filter = SG_FILTER_LINEAR;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].smp = sg_make_sampler(&shadow_smp_desc);
        sg_attachments_desc att_desc = {};
        att_desc.depth_stencil.image = sokol->render_pass[RENDER_PASS::TYPE_SHADOW].map;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].atts = sg_make_attachments(&att_desc);
        glm_vec3_copy(vec3{50, 50, -50}, sokol->render_pass[RENDER_PASS::TYPE_SHADOW].light_pos);
        glm_vec3_sub(vec3{0,0,0}, sokol->render_pass[RENDER_PASS::TYPE_SHADOW].light_pos, sokol->render_pass[RENDER_PASS::TYPE_SHADOW].light_dir);
        
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].att_desc.depth_stencil.image = sokol->render_pass[RENDER_PASS::TYPE_SHADOW].map;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].att_desc.depth_stencil.slice = 0;
        sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pass.attachments = sg_make_attachments(&sokol->render_pass[RENDER_PASS::TYPE_SHADOW].att_desc);
        
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
            glm_mat4_mul(sokol->light_view_projections[slice_id], mesh->model, shadow_params.light_view_mvp);
            sg_apply_uniforms(UB_vs_shadow_params, &SG_RANGE(shadow_params));
            
            sg_apply_bindings(mesh->bind.bindings[binding_idx].sgb);
            sg_draw(0, mesh->bind.bindings[binding_idx].indice_count, 1);
        } break;
    }
}

static bool apply_uniforms_for(const int PASS, int idx, skate_render_mesh_t *mesh) {
    skate_sokol_t *sokol = get_sokol();
    
    bool result = false;
    
    vec4 light_pos = {0};
    mat4 light_proj;
    mat4 light_view_proj;
    { // light
        glm_lookat(sokol->render_pass[PASS].light_pos, vec3{0,0,0}, vec3{0,1,0}, sokol->render_pass[PASS].light_mat);
        
        glm_ortho(-5.f, 5.f, -5.f, 5.f, 0, 128.f, light_proj);
        glm_mat4_mul(light_proj, sokol->render_pass[PASS].light_mat, light_view_proj);
    }
    
    glm_mat4_identity(mesh->model);
    {
        mat4 xm;
        mat4 ym;
        mat4 zm;
        mat4 store;
        mat4 store2;
        glm_mat4_identity(xm);
        glm_mat4_identity(ym);
        glm_mat4_identity(zm);
        glm_mat4_identity(store);
        glm_mat4_identity(store2);
        
        glm_rotated_x(xm, mesh->rotation[0], xm);
        glm_rotated_y(ym, mesh->rotation[1], ym);
        glm_rotated_y(zm, mesh->rotation[2], zm);
        glm_mat4_mul(xm, ym, store);
        glm_mat4_mul(store, zm, store2);
        
        mat4 tm;
        glm_mat4_identity(tm);
        glm_translate_to(tm, mesh->position, tm);
        
        mat4 sm;
        glm_mat4_identity(sm);
        glm_scale_to(sm, mesh->scale, sm);
        
        glm_mat4_mul(sm, store2, store);
        glm_mat4_mul(store, tm, mesh->model);
    }
    
    mat4 view_projection;
    glm_mat4_identity(view_projection);
    {
        glm_mat4_mul(sokol->default_view.perspective, sokol->default_view.view, view_projection);
    }
    
    sg_pass pass = {};
    switch(PASS) {
        case RENDER_PASS::TYPE_STATIC: {
            texture_static_mesh_vs_params_t params = {};
            glm_mat4_mul(view_projection, mesh->model, params.mvp);
            
            glm_mat4_copy(mesh->model, params.model);
            sg_apply_uniforms(UB_texture_static_mesh_vs_params, &SG_RANGE(params));
            
            // fs
            texture_static_mesh_fs_params_t fs_params = {};
            glm_vec3_copy(sokol->render_pass[PASS].light_pos, fs_params.eye_pos);
            glm_vec3_copy(sokol->render_pass[PASS].light_dir, fs_params.light_dir);
            glm_vec3_copy(vec3{1.f,1.f,1.f}, fs_params.light_color);
            glm_vec3_copy(vec3{0.25f, 0.25f, 0.25f}, fs_params.light_ambient);
            
            for(int i = 0; i < CASCADE_LEVEL_COUNT; ++i) {
                glm_mat4_copy(sokol->light_view_projections[i], fs_params.light_view_proj[i]);
                
                vec4 cssms = {0};
                cssms[0] = map_sizes[i];
                cssms[1] = map_sizes[i];
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

static void render_loop() {
    skate_sokol_t *sokol = get_sokol();
    
    // shadow cascades
    for(int i = 0; i < CASCADE_LEVEL_COUNT + 1; ++i) {
        glm_mat4_identity(sokol->light_view_projections[i]);
        if(i == 0) {
            get_light_view_projection_matrix(sokol->light_view_projections[i], CAMERA_NEAR_PLANE, shadow_cascade_levels[i]);
        } else if(i < CASCADE_LEVEL_COUNT) {
            get_light_view_projection_matrix(sokol->light_view_projections[i], shadow_cascade_levels[i - 1], shadow_cascade_levels[i]);
        } else {
            get_light_view_projection_matrix(sokol->light_view_projections[i], shadow_cascade_levels[i - 1], CAMERA_FAR_PLANE);
        }
    }
    
    for(int pass_idx = 0; pass_idx < RENDER_PASS::TYPE_COUNT; ++pass_idx) {
        if(!sokol->render_pass[pass_idx].active) {continue;} // not active? skip
        
        // shadow
        if(pass_idx == RENDER_PASS::TYPE_SHADOW) {
            sokol->render_pass[pass_idx].pass.action = sokol->render_pass[pass_idx].pass_action;
            sokol->render_pass[pass_idx].pass.label = "shadow_pass";
            
            for(int i = 0; i < CASCADE_LEVEL_COUNT; ++i) {
                sokol->render_pass[pass_idx].att_desc.depth_stencil.slice = i;
                sg_init_attachments(sokol->render_pass[pass_idx].pass.attachments, &sokol->render_pass[pass_idx].att_desc);
                
                sg_begin_pass(&sokol->render_pass[pass_idx].pass);
                sg_apply_pipeline(sokol->render_pass[RENDER_PASS::TYPE_SHADOW].pipeline);
                
                skate_buffer_t *mesh_buffer = get_buffer_for_pass(pass_idx);
                for(int m = 0; m < mesh_buffer->count(); ++m) {
                    skate_render_mesh_t *mesh = mesh_buffer->as_idx<skate_render_mesh_t>(m);
                    for(int p = 0; p < mesh->bind.bindings_count; ++p) {
                        if(apply_uniforms_for(pass_idx, p, mesh)) {
                            render_mesh_for(RENDER_PASS::TYPE_SHADOW, p, i, mesh); 
                        }
                    }
                }
                sg_end_pass();
            }
        }
        if(pass_idx == RENDER_PASS::TYPE_STATIC) {
            sokol->render_pass[pass_idx].pass.action = sokol->render_pass[pass_idx].pass_action;
            sokol->render_pass[pass_idx].pass.swapchain = sglue_swapchain();;
            
            sg_begin_pass(sokol->render_pass[pass_idx].pass);
            sg_apply_pipeline(sokol->render_pass[pass_idx].pipeline);
            
            skate_buffer_t *mesh_buffer = get_buffer_for_pass(pass_idx);
            for(int m = 0; m < mesh_buffer->count(); ++m) {
                skate_render_mesh_t *mesh = mesh_buffer->as_idx<skate_render_mesh_t>(m);
                for(int p = 0; p < mesh->bind.bindings_count; ++p) {
                    if(apply_uniforms_for(pass_idx, p, mesh)) {}
                }
            }
            
            sg_end_pass();
            
        }
    }
    sg_commit();
}

static void sokol_frame() {
    skate_sokol_t *sokol = get_sokol();
    view_tick(&sokol->default_view);
    
    render_loop();
}

static void sokol_cleanup() {
    skate_sokol_t *sokol = get_sokol();
    free_buffer(&sokol->render_pass[RENDER_PASS::TYPE_STATIC].mesh_buffer);
    free_buffer(&sokol->render_pass[RENDER_PASS::TYPE_SHADOW].mesh_buffer);
    free_buffer(&sokol->render_pass[RENDER_PASS::TYPE_DYNAMIC].mesh_buffer);
    
    logger_shutdown();
    sg_shutdown();
}

static void sokol_event(const sapp_event *event) {
    
}
