#define SOKOL_IMPL

#define STB_SPRINTF_IMPLEMENTATION 
#include "stb/stb_sprintf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "skate_types.h"
#include "skate_log.h"
#include "skate_sokol.h"
#include "skate_zlib.h"
#include "skate_model_import.h"

sapp_desc sokol_main(int argc, char* argv[]) {
    logger_init();
    
    skate_sokol_t *sokol = get_sokol();
    
    // store root dir (where the exe for this program is located in the file directory)
    char *arg = argv[0];
    char *ptr = strstr(arg, "\\build");
    if(ptr) {
        memcpy(&sokol->root_dir.ptr, arg, ptr - arg);
        for(s32 i =0; i < ptr - arg; ++i) {
            if(sokol->root_dir.ptr[i] == '\\') {
                sokol->root_dir.ptr[i] = '/';
            }
        }
        sokol->root_dir.len = strlen((char*)sokol->root_dir.ptr);
    }
    
    // sokol app
    sapp_desc desc = {};
    desc.width = sokol->initial_window_size[0];
    desc.height = sokol->initial_window_size[1];
    desc.init_cb = sokol_init;
    desc.frame_cb = sokol_frame;
    desc.cleanup_cb = sokol_cleanup;
    desc.event_cb = sokol_event;
    desc.logger.func = slog_func;
    desc.window_title = "Skate";
    desc.win32_console_create = true;
    desc.win32_console_attach = true;
    desc.sample_count = 2;
    return desc;
}

#include "skate_sokol.cpp"
#include "skate_types.cpp"
#include "skate_model_import.cpp"
#include "skate_zlib.cpp"
#include "skate_log.cpp"