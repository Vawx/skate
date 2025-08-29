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

static bool in_ascii_number_range(char in) {return in <= 57 && in >= 48;}
static bool could_be_floating_point(char *in, int len) {
    for(int i = 0; i < len; ++i) {
        if(in[i] != '.' && in[i] != '-') {
            if(!in_ascii_number_range(in[i])) {
                return false;
            }
        } 
    }
    return true;
    
}

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

#define PRECISION_MAX
const r64 precision[PRECISION_MAX] = {
    0.1f,
    0.01f,
    0.001f,
    0.0001f,
    0.00001f,
    0.000001f,
    0.0000001f,
    0.00000001f,
    0.000000001f,
    0.0000000001f,
    0.00000000001f,
    0.000000000001f,
};

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

struct skate_string_core_t {
    skate_buffer_t string_buffer;
    void reset();
};

static skate_string_core_t *string_core();

struct skate_string_t {
    public:
    skate_string_t() : ptr(nullptr), len(0), f(0.f), dec_init(false), is_floating(false), user_flag(false) {}
    skate_string_t(const char *in_ptr) : dec_init(false), is_floating(false), user_flag(false) {
        if(in_ptr == nullptr) __debugbreak();
        len = (u32)strlen(in_ptr);
        ptr = string_core()->string_buffer.pos;
        string_core()->string_buffer.pos += len + 1;
        memcpy(ptr, in_ptr, len);
        ptr[len] = '\0';
        
        init_dec();
    }
    
    skate_string_t(const char *in_ptr, const u32 in_len) : dec_init(false), is_floating(false), user_flag(false) {
        if(in_ptr == nullptr || in_len <= 0) __debugbreak();
        len = in_len;
        ptr = string_core()->string_buffer.pos;
        string_core()->string_buffer.pos += len + 1;
        memcpy(ptr, in_ptr, len);
        ptr[len] = '\0';
        
        init_dec();
    }
    
    bool compare(skate_string_t *in) {
        if(in->len != len) return false;
        u32 c = 0;
        while(c != len) {
            if(in->ptr[c] != ptr[c]) {
                return false;
            }
            ++c;
        }
        return true;
    }
    
    char *find(const char *in, const u32 in_len) {
        bool success = true;
        char *temp = (char*)in;
        for(int i = 0; i < len; i++) {
            if(*temp == ptr[i]) {
                ++temp;
                if(temp - (char*)ptr >= in_len) {
                    return (char*)&ptr[i];
                }
            } else {
                temp = (char*)in;
            }
        }
        
    }
    
    char *find(const skate_string_t *str) {
        return find((char*)str->ptr, str->len);
    }
    
    s64 to_int() {
        if(is_floating) {
            return (s64)f;
        } else {
            return i;
        }
    }
    
    r64 to_float() {
        if(is_floating) {
            return f;
        } else {
            return (r64)i;
        }
    }
    
    
    skate_string_t &operator=(const skate_string_t &in) {
        ptr = in.ptr;
        len = in.len;
        user_flag = in.user_flag;
        dec_init = false;
        is_floating = false;
        init_dec();
        return *this;
    }
    
    union {
        u8 *ptr;
        char *c_ptr;
    };
    u32 len;
    
    bool user_flag;
    
    private:
    
    s32 convert_int() {
        s32 result = 1;
        u64 j = 1;
        
        for(int j = 0, i = 0; i < len; ++i) {
            if(ptr[i] == '-') {s_assert(i == 0); result *= -1; continue;}
            s_assert(ptr[i] >= 48 && ptr[i] <= 57); // must be integer
            
            s32 x = (ptr[i] - '0');
            if(j) {
                x *= j;
            }
            result += x;
            j *= 10;
        }
        return result - 1; // start at 1 so we dont do 0 * -1
    }
    
    r64 convert_float() {
        r64 result = 0;
        
        char *decimal = &c_ptr[len - 1];
        while(*decimal != '.') {--decimal;}
        
        int decimal_count = &c_ptr[len - 1] - decimal;
        ++decimal;
        for(int i = 0; i < decimal_count; ++i) {
            s_assert(decimal[i] >= 48 && decimal[i] <= 57); // must be integer
            if(decimal[i] == '0') continue;
            
            s32 x = decimal[i] - '0';
            result += (r64)x * (r32)precision[i];
        }
        
        char *whole = decimal - 2;
        int j = 1;
        while(whole >= c_ptr) {
            if(*whole == '-') {--whole; continue;}
            
            r32 x = *whole - '0';
            result += x * j;
            --whole;
            j *= 10;
        }
        
        if(c_ptr[0] == '-') {result = -result;}
        return result;
    }
    
    void init_dec() {
        if(dec_init) return;
        
        // check that it is actually a number
        bool success = true;
        for(int i = 0; i < len; ++i) {
            if(!could_be_floating_point(c_ptr, len)) {
                success = false;
                break;
            }
        }
        
        if(success) {
            // clarify if this is a floating point by checking for decible
            if(strstr(c_ptr, ".")) {
                is_floating = true;
                f = convert_float();
            } else {
                i = convert_int();
            }
        }
        
        dec_init = true;
    }
    
    union {
        r64 f;
        s64 i;
    };
    bool dec_init;
    bool is_floating;
};

#define SKATE_TYPES_H
#endif //SKATE_TYPES_H
