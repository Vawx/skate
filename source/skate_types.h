/* date = July 30th 2025 1:26 pm */

#ifndef SKATE_TYPES_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#define s_pi 3.14159265358979323846
#define s_2_pi (2.0 * s_pi)
#define s_half_pi (s_pi * 0.5f)
#define s_rads_per_deg (s_2_pi / 360.f)
#define s_degs_per_rad (360.f / s_2_pi)
#define s_sqrt12 (0.7071067811865475244008443621048490f)

#define s_deg_180 180.0
#define s_turn_half 0.5
#define s_rad_to_deg ((r32)(s_deg_180/s_pi))
#define s_rag_to_turn ((r32)(s_turn_half/s_pi))
#define s_deg_to_rad ((r32)(s_pi/s_deg_180))
#define s_deg_to_turn ((r32)(s_turn_half/s_deg_180))
#define s_turn_to_rad ((r32)(s_pi/s_turn_half))
#define s_turn_to_deg ((r32)(s_deg_180/s_turn_half))

#define s_rad(a) ((a)*s_deg_to_rad)
#define s_deg(a) ((a)*s_rad_to_deg)
#define s_turn(a) ((a)*s_turn_to_deg)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float r32;
typedef double r64;

#define stmnt(s) do{ s }while(0)

#define s_assert_break() (*(int*)0 = 0xABCD)
#define s_assert(c) stmnt( if (!(c)){ s_assert_break(); } )
#define s_assert_msg(c, msg, ...) stmnt( if(!(c)){ LOG_SHUTDOWN(msg, ##__VA_ARGS__); s_assert_break(); } )

#define array_count(a) (sizeof(a) / sizeof((a)[0]))

#define kilo(n)  (((u64)(n)) << 10)
#define mega(n)  (((u64)(n)) << 20)
#define giga(n)  (((u64)(n)) << 30)
#define tera(n)  (((u64)(n)) << 40)

#define thousand(n)   ((n)*1000)
#define million(n)    ((n)*1000000)
#define billion(n)    ((n)*1000000000)

#define R32_MIN 1.175494E-38
#define R32_MAX 3.40282347E+38

#include "cglm/cglm.h"

#define glm_vec3_muls(v, s, d) glm_vec3_scale_as((v), (s), (d))
#define glm_vec3_mulf(v, s, d) glm_vec3_muls((v), (s), (d))
#define glm_vec3_len(v) glm_vec3_norm((v))

#define skate_abs(v) (v) < 0.f ? -(v) : (v)

struct skate_buffer_t {
    skate_buffer_t() : ptr(nullptr), size(0), type_size(0) {}
    
    template<typename T>
        T *as() {
        return (T*)ptr;
    }
    
    template<typename T>
        T *as_idx(u32 idx) {
        u32 ss = idx * type_size;
        return (T*)&ptr[ss];
    }
    
    u32 count() {
        if(pos != ptr) {
            u32 l = pos - ptr;
            u32 c = l / type_size;
            return c;
        }
        return 0;
    }
    
    u8 *ptr;
    u8 *pos;
    u16 type_size;
    u32 size;
};

static skate_buffer_t alloc_buffer(const s16 type_size, const u32 count);
static void free_buffer(skate_buffer_t *buffer);
static void push_buffer(skate_buffer_t *buffer, u8 *ptr, u32 size);

namespace sokol_skate {
    static u8 *mem_alloc(const s32 size);
    static void mem_free(void *ptr);
};

#if _WIN32
#define WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define SKATE_DIRECTORY_NONE      ""
#define SKATE_DIRECTORY_CONTENT   "/content/"
#define SKATE_DIRECTORY_TEXTURE   SKATE_DIRECTORY_CONTENT##"texture/"
#define SKATE_DIRECTORY_AUDIO     SKATE_DIRECTORY_CONTENT##"audio/"
#define SKATE_DIRECTORY_MESH      SKATE_DIRECTORY_CONTENT##"mesh/"
#define SKATE_DIRECTORY_SHADER    SKATE_DIRECTORY_CONTENT##"shader/"
#define SKATE_DIRECTORY_LOGS      "/build/logs/"

#define SKATE_DIRECTORY_MAX_LEN   255

struct skate_directory_t {
    public:
    skate_directory_t() {memset(ptr, 0, SKATE_DIRECTORY_MAX_LEN); len = 0;}
    skate_directory_t(const char *in_ptr, const char *subdir);
    
    u8 len;
    u8 ptr[SKATE_DIRECTORY_MAX_LEN];
    
    const char *get() const {return (char*)ptr;}
};

static skate_directory_t get_directory_for(const char *filename, const char *sub_dir);

struct skate_file_t {
    public:
    skate_file_t() : ptr(nullptr), len(0) {}
    u8 *ptr;
    u32 len;
};

static skate_file_t open_file(const skate_directory_t *dir);

struct skate_image_file_t {
    public:
    skate_image_file_t() : ptr(nullptr), w(0), h(0), c(0) {}
    skate_image_file_t(const skate_directory_t *dir);
    u8 *ptr;
    s32 w;
    s32 h;
    s32 c;
};

static skate_image_file_t open_texture_file(const skate_directory_t *dir);


#define SKATE_TYPES_H
#endif //SKATE_TYPES_H
