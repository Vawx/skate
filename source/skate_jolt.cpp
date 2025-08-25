static skate_jolt *get_jolt() {
    static skate_jolt jolt;
    if(!jolt.init) {
        JPC_RegisterDefaultAllocator();
        JPC_FactoryInit();
        JPC_RegisterTypes();
        
        jolt.temp_allocator = JPC_TempAllocatorImpl_new(10 * 1024 * 1024);
        jolt.job_system = JPC_JobSystemThreadPool_new2(JPC_MAX_PHYSICS_JOBS, JPC_MAX_PHYSICS_BARRIERS);
        jolt.broad_phase_layer_interface = JPC_BroadPhaseLayerInterface_new(nullptr, jolt_get_broad_phase_layer_interface());
        
        jolt.object_vs_broad_phase_layer_filter = JPC_ObjectVsBroadPhaseLayerFilter_new(nullptr, get_obj_vs_broadphase_layer_filter());
        jolt.object_vs_object_layer_filter = JPC_ObjectLayerPairFilter_new(nullptr, jolt_get_object_layer_pair_filter());
        
        jolt.physics_system = JPC_PhysicsSystem_new();
        JPC_PhysicsSystem_Init(jolt.physics_system,
                               JOLT_MAX_BODIES,
                               JOLT_NUM_BODY_MUTEX,
                               JOLT_MAX_BODY_PAIRS,
                               JOLT_MAX_CONTACT_CONSTRAINTS,
                               jolt.broad_phase_layer_interface,
                               jolt.object_vs_broad_phase_layer_filter,
                               jolt.object_vs_object_layer_filter);
        jolt.init = true;
    }
    return &jolt;
}

static j_body_interface *jolt_get_body_interface() {
    skate_jolt *jolt = get_jolt();
    return JPC_PhysicsSystem_GetBodyInterface(jolt->physics_system);
}

static u32 jolt_get_num_of_broadphase_layers(const void *self) {
	return SkateJoltBroadPhaseLayers::Type::COUNT;
}

static j_broadphase_layer jolt_get_broadphase_layer(const void *self, JPC_ObjectLayer in_layer) {
    if(in_layer == SkateJoltObjectLayers::Type::NON_MOVING) {return SkateJoltBroadPhaseLayers::Type::NON_MOVING;}
    if(in_layer == SkateJoltObjectLayers::Type::MOVING) {return SkateJoltBroadPhaseLayers::Type::MOVING;}
    
    unreachable();
}

static j_broad_phase_layer_interface_func jolt_get_broad_phase_layer_interface() {
    static j_broad_phase_layer_interface_func layer_interface;
    layer_interface.GetNumBroadPhaseLayers = jolt_get_num_of_broadphase_layers;
	layer_interface.GetBroadPhaseLayer = jolt_get_broadphase_layer;
    return layer_interface;
};

static bool jolt_object_broadphase_should_collide(const void *self, JPC_ObjectLayer ol, JPC_BroadPhaseLayer bpl) {
	if(ol == SkateJoltObjectLayers::Type::NON_MOVING) {return bpl == SkateJoltBroadPhaseLayers::Type::MOVING;}
    if(ol == SkateJoltObjectLayers::Type::MOVING) {return true;}
    
    unreachable();
}

static j_obj_vs_broadphase_layer_filter_func get_obj_vs_broadphase_layer_filter() {
    static j_obj_vs_broadphase_layer_filter_func filter;
	filter.ShouldCollide = jolt_object_broadphase_should_collide;
    return filter;
};

static bool jolt_obj_vs_obj_layer_should_collide(const void *self, JPC_ObjectLayer ol1, JPC_ObjectLayer ol2) {
	if(ol1 == SkateJoltObjectLayers::Type::NON_MOVING) {return ol2 == SkateJoltObjectLayers::Type::MOVING;}
    if(ol1 == SkateJoltObjectLayers::Type::MOVING) {return true;}
    
    unreachable();
}

static j_object_layer_Pair_filter_func jolt_get_object_layer_pair_filter() {
	static j_object_layer_Pair_filter_func filter;
    filter.ShouldCollide = jolt_obj_vs_obj_layer_should_collide;
    return filter;
};

static void jolt_draw_debug_line(const void *self, JPC_RVec3 inFrom, JPC_RVec3 inTo, JPC_Color inColor) {
	// printf("Draw line from (%f, %f, %f) to (%f, %f, %f) with color (%d, %d, %d)\n",
	// 	inFrom.x, inFrom.y, inFrom.z, inTo.x, inTo.y, inTo.z, inColor.r, inColor.g, inColor.b);
    
    // TODO(Kyle): debug lines -- sokol does this
}

static j_debug_render_func *jolt_get_debug_render(){
	static j_debug_render_func render;
    render.DrawLine = jolt_draw_debug_line;
    return &render;
};

static bool jolt_push_shape_sphere(vec3 pos, const r32 rad, u32 activation, u8 motion_type, u8 object_layer, j_body_id *out) {
    j_body_interface *ji = jolt_get_body_interface();
    
    JPC_SphereShapeSettings sphere_shape_settings;
	JPC_SphereShapeSettings_default(&sphere_shape_settings);
	sphere_shape_settings.Radius = rad;
    
    JPC_Shape *sphere_shape = nullptr;
    JPC_String *err = nullptr;
	if(!JPC_SphereShapeSettings_Create(&sphere_shape_settings, &sphere_shape, nullptr)) {
        LOG_PANIC("failed to create sphere physics shape %s", JPC_String_c_str(err));
        return false;
    }
    
    JPC_BodyCreationSettings sphere_settings;
	JPC_BodyCreationSettings_default(&sphere_settings);
	sphere_settings.Position = JPC_RVec3{pos[0], pos[1], pos[2]};
	sphere_settings.MotionType = (JPC_MotionType)motion_type;
	sphere_settings.ObjectLayer = object_layer;
	sphere_settings.Shape = sphere_shape;
    
    j_body *body = JPC_BodyInterface_CreateBody(ji, &sphere_settings);
    *out = JPC_Body_GetID(body);
	JPC_BodyInterface_AddBody(ji, *out, (JPC_Activation)activation);
    return true;
}

static bool jolt_push_shape_plane(vec3 pos, const r32 dist, u32 activation, u8 motion_type, u8 object_layer, j_body_id *out) {
    LOG_YELL("plane physics shape not implemented");
    out = 0;
    return false;
    
}

static bool jolt_push_shape_box(vec3 pos, vec3 whd, u32 activation, u8 motion_type, u8 object_layer, j_body_id *out) {
    j_body_interface *ji = jolt_get_body_interface();
    JPC_BoxShapeSettings floor_shape_settings;
	JPC_BoxShapeSettings_default(&floor_shape_settings);
	floor_shape_settings.HalfExtent = JPC_Vec3{whd[0], whd[1], whd[2]};
	floor_shape_settings.Density = 1;
    
	JPC_Shape *floor_shape;
    JPC_String *err;
	if (!JPC_BoxShapeSettings_Create(&floor_shape_settings, &floor_shape, &err)) {
		LOG_PANIC("failed to create box physics shape %s", JPC_String_c_str(err));
        return false;
	}
    
	JPC_BodyCreationSettings floor_settings;
	JPC_BodyCreationSettings_default(&floor_settings);
	floor_settings.Position = JPC_RVec3{pos[0], pos[1], pos[2]};
	floor_settings.MotionType = (JPC_MotionType)motion_type;
	floor_settings.ObjectLayer = object_layer;
	floor_settings.Shape = floor_shape;
    
	JPC_Body* floor = JPC_BodyInterface_CreateBody(ji, &floor_settings);
    *out = JPC_Body_GetID(floor);
	JPC_BodyInterface_AddBody(ji, *out, (JPC_Activation)activation);
    return true;
}

static void jolt_get_model_transform(j_body_id *id, mat4 out) {
    j_body_interface *ji = jolt_get_body_interface();
    JPC_RMat44 mt = JPC_BodyInterface_GetWorldTransform(ji, *id);
    
    out[0][0] = mt.col[0].x;
    out[0][1] = mt.col[0].y;
    out[0][2] = mt.col[0].z;
    out[0][3] = mt.col[0].w;
    
    out[1][0] = mt.col[1].x;
    out[1][1] = mt.col[1].y;
    out[1][2] = mt.col[1].z;
    out[1][3] = mt.col[1].w;
    
    out[2][0] = mt.col[2].x;
    out[2][1] = mt.col[2].y;
    out[2][2] = mt.col[2].z;
    out[2][3] = mt.col[2].w;
    
    out[3][0] = mt.col3.x;
    out[3][1] = mt.col3.y;
    out[3][2] = mt.col3.z;
    out[3][3] = mt.col3._w;
}

#if 0 // example shapes
bool success = false;
j_body_id id;
if(jolt_push_shape_box(vec3{0,-1,0}, vec3{100, 1, 100}, JOLT_DONT_ACTIVATE, JOLT_STATIC, SkateJoltObjectLayers::Type::NON_MOVING, &id)) {
    if(jolt_push_shape_sphere(vec3{0.f, 300.f, 0.f}, 0.5f, JOLT_ACTIVATE, JOLT_DYNAMIC, SkateJoltObjectLayers::Type::MOVING, &sid)) {
        JPC_BodyInterface_SetLinearVelocity(jolt_get_body_interface(), sid, JPC_Vec3{0.0, -15.0, 0.0});
        success = true;
    }
}

LOG_PANIC_COND(success, "FAILED TO PUSH OBJECTS TO JOLT");
#endif

static void skate_jolt_init(skate_jolt *jolt) {
    j_body_id id;
    if(jolt_push_shape_box(vec3{0,-30,0}, vec3{100, 1, 100}, JOLT_DONT_ACTIVATE, JOLT_STATIC, SkateJoltObjectLayers::Type::NON_MOVING, &id)) {
        mat4 t;
        jolt_get_model_transform(&id, t);
        
        printf("");
    }
}

static void skate_jolt_frame(skate_jolt *jolt) {
    
    JPC_PhysicsSystem_Update(jolt->physics_system, JOLT_FPS_MS, JOLT_COLLISION_STEPS, jolt->temp_allocator, (JPC_JobSystem*)jolt->job_system);
    
}

static void skate_jolt_shutdown(skate_jolt *jolt) {
    JPC_PhysicsSystem_delete(jolt->physics_system);
	JPC_BroadPhaseLayerInterface_delete(jolt->broad_phase_layer_interface);
	JPC_ObjectVsBroadPhaseLayerFilter_delete(jolt->object_vs_broad_phase_layer_filter);
	JPC_ObjectLayerPairFilter_delete(jolt->object_vs_object_layer_filter);
    
	JPC_JobSystemThreadPool_delete(jolt->job_system);
	JPC_TempAllocatorImpl_delete(jolt->temp_allocator);
    
	JPC_UnregisterTypes();
	JPC_FactoryDelete();
}

