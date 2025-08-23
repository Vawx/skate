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

static j_body_interface *get_jolt_body_interface() {
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

static void skate_jolt_init(skate_jolt *jolt) {
    
}

static void skate_jolt_frame(skate_jolt *jolt) {
    
}

static void skate_jolt_shutdown(skate_jolt *jolt) {
    
}

