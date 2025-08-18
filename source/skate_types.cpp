
static skate_view_t make_view(vec3 pos, vec3 target, r32 fov) {
    skate_view_t result = {};
    result.fov = fov;
    glm_vec3_copy(pos, result.position);
    glm_vec3_copy(target, result.target);
    update_view(&result);
    return result;
}

static void update_view(skate_view_t *view) {
    glm_vec3_sub(view->target, view->position, view->direction);
    glm_vec3_normalize(view->direction);
    
    glm_vec3_cross(vec3{0.f, 1.f, 0.f}, view->direction, view->right);
    glm_vec3_cross(view->direction, view->right, view->up);
    
    skate_sokol_t *sokol = get_sokol();
    
    if(view->is_ortho) {
        glm_ortho(0.f, sokol->widthf(), 0.f, sokol->heightf(), -1.f, 1.f, view->ortho);
    } else {
        glm_perspective(view->fov, sokol->aspect(), 1.f, 100.f, view->perspective);
    }
    
    glm_lookat(view->position, view->target, view->up, view->view);
}

static void view_tick(skate_view_t *view) {
    skate_sokol_t *sokol = get_sokol();
    update_view(&sokol->default_view);
    
    // TODO(Kyle) all views, if needed...
}

static void add_to_view(skate_view_t *view, vec3 to_add) {
    vec3 new_pos = {};
    glm_vec3_add(view->position, to_add, new_pos);
    glm_vec3_copy(new_pos, view->position);
    view_tick(view);
}

static void look_at_view(skate_view_t *view, vec3 target) {
    glm_lookat(view->position, target, vec3{0.f, 1.f, 0.f}, view->view);
}

namespace sokol_skate {
    static u8 *mem_alloc(const s32 size) {
        u8 *ptr = nullptr;
#if defined(WINDOWS)
        ptr = (u8*)VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
        ptr = (u8*)malloc(size);
#endif
        return ptr;
    }
    
    static void mem_free(void *ptr) {
        if(!ptr) return;
        
#if defined(WINDOWS)
        VirtualFree(ptr, 0, MEM_RELEASE);
#else
        free(ptr);
#endif
    }
};

static skate_buffer_t alloc_buffer(const s16 type_size, const u32 count) {
    skate_buffer_t result = skate_buffer_t();
    result.size = type_size * count;
    result.type_size = type_size;
    result.ptr = sokol_skate::mem_alloc(result.size);
    result.pos = result.ptr;
    return result;
}

static void free_buffer(skate_buffer_t *buffer) {
    if(buffer->ptr) {
        sokol_skate::mem_free(buffer->ptr);
        buffer->ptr = nullptr;
        buffer->size = 0;
    }
}

static void push_buffer(skate_buffer_t *buffer, u8 *ptr, u32 size) {
    if(buffer->ptr && ptr && size) {
        memcpy(buffer->pos, ptr, size);
        buffer->pos += size;
    }
}

static skate_directory_t get_directory_for(const char *filename, const char *sub_dir) {
    skate_sokol_t *sokol = get_sokol();
    
    skate_directory_t result = {};
    char *ptr = (char*)&result.ptr[0];
    u32 len = sokol->root_dir.len;
    memcpy(ptr, sokol->root_dir.ptr, len);
    ptr += len;
    
    if(sub_dir) {
        len = strlen(sub_dir);
        memcpy(ptr, sub_dir, len);
        ptr += len;
    }
    
    len = strlen(filename);
    memcpy(ptr, filename, len);
    
    result.len = strlen((char*)result.ptr);
    return result;
}

skate_directory_t::skate_directory_t(const char *in_ptr, const char *subdir) {
    *this = get_directory_for(in_ptr, subdir);
}

static skate_file_t open_file(const skate_directory_t *dir) {
    skate_file_t result = {};
    if(dir->ptr) {
        FILE *f = fopen((const char*)dir->ptr, "rb");
        if(f) {
            fseek(f, 0, SEEK_END);
            result.len = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            result.ptr = sokol_skate::mem_alloc(result.len);
            fread(result.ptr, sizeof(char), result.len, f);
            fclose(f);
        }
    }
    return result;
}

static skate_image_file_t open_texture_file(const skate_directory_t *dir) {
    skate_image_file_t result = {};
    result.ptr = stbi_load(dir->get(), &result.w, &result.h, &result.c, 4);
    //stbi_image_free(data);
    return result;
}

skate_image_file_t::skate_image_file_t(const skate_directory_t *dir) {
    stbi_set_flip_vertically_on_load(1);
    ptr = stbi_load(dir->get(), &w, &h, &c, 4);
}