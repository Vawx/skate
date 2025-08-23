
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