#include "fbx_types.h"

#if _WIN32
void mem_free(void* ptr) {
	VirtualFree(ptr, 0, MEM_RELEASE);
}
#else
void mem_free(void* ptr) {
	__debugbreak();
}
#endif