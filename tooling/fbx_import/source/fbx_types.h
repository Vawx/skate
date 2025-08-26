#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
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

#define k_assert_break() (*(int*)0 = 0xABCD)
#define k_assert(c) stmnt( if (!(c)){ k_assert_break(); } )
#define k_assert_msg(c, msg, ...) stmnt( if(!(c)){ LOG_SHUTDOWN(msg, ##__VA_ARGS__); k_assert_break(); } )

#define array_count(a) (sizeof(a) / sizeof((a)[0]))

#define kilo(n)  (((u64)(n)) << 10)
#define mega(n)  (((u64)(n)) << 20)
#define giga(n)  (((u64)(n)) << 30)
#define tera(n)  (((u64)(n)) << 40)

#define thousand(n)   ((n)*1000)
#define million(n)    ((n)*1000000)
#define billion(n)    ((n)*1000000000)

#define VERY_SMALL_NUMBER 0.e+006
#define VERY_LARGE_NUMBER UINT64_MAX

template<typename T>
T* mem_alloc(s64 count) {
	T* result = (T*)VirtualAlloc(0, count * sizeof(T), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return result;
}

void mem_free(void* ptr);