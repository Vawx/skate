#define stmnt(s) do{ s }while(0)

#define c_assert_break() (*(int*)0 = 0xABCD)
#define c_assert(c) stmnt( if (!(c)){ c_assert_break(); } )
#define c_assert_msg(c, msg, ...) stmnt( if(!(c)){ LOG_SHUTDOWN(msg, ##__VA_ARGS__); c_assert_break(); } )

static logger_t logs;

static u32 log_get_system_time_str(char *buffer, char *dest, u8 file_path_output) {
    u32 result_len = 0;
#if _WIN32 // TODO(Kyle) other os's
    SYSTEMTIME st, lt;
    GetLocalTime(&lt);
    
    u8 hour = lt.wHour;
    if(hour > 12) hour -= 12;
    if(file_path_output) {
        stbsp_sprintf(buffer, "%02d_%02d_%02d__%02d_%02d_%02d", lt.wMonth, lt.wDay, lt.wYear, hour, lt.wMinute, lt.wSecond);
    } else {
        stbsp_sprintf(buffer, "[%02d/%02d/%02d - %02d:%02d:%02d] : ", lt.wMonth, lt.wDay, lt.wYear, hour, lt.wMinute, lt.wSecond);
    }
    result_len = strlen(buffer);
    
    memcpy(dest, buffer, result_len);
#endif
    return result_len;
}

void log_store_ouput(log_severity severity, const char *file, const u32 line, const char *str, ...) {
    //TODO(Kyle) check log_idx and ptr_idx for used. if used too much, flush to file and reset.
    // or, just always flush log to file as it comes in.
    
    char *ttt = (char*)strstr(str, "\n");
    if(ttt) c_assert_break();
    
    if(logs.log_idx + 1 > LOGGER_LOGS_COUNT || LOGGER_MEMORY_RESERVE - logs.ptr_idx < 255) {
        logger_shutdown();
        c_assert_break();
        // out of space -- if you hit this, fix it.
    }
    
    log_t *log = &logs.logs[logs.log_idx++]; // increment logs buffer
    log->severity = severity;
    log->ptr = &logs.ptr[logs.ptr_idx];
    char *ptr = (char*)log->ptr;
    
    char time_buffer[255] = {0};
    ptr += log_get_system_time_str(time_buffer, ptr, false);
    
    u32 len = strlen(log_severity_str[severity]);
    memcpy(ptr, log_severity_str[severity], len);
    ptr += len;
    
    char file_buffer[255] = {0};
    memcpy(file_buffer, file, strlen(file));
    char *file_buffer_ext = strstr(file_buffer, ".");
    
    while(*file_buffer_ext != '\\' && *file_buffer_ext != '/') {
        *file_buffer_ext--;
    }
    ++file_buffer_ext;
    
    ptr += stbsp_sprintf(ptr, "[%s][%d] : ", file_buffer_ext, line);
    
    va_list args;
    va_start (args, str);
    vsprintf (ptr, str, args);
    va_end (args);
    
    log->len = strlen((const char*)log->ptr);
    logs.ptr_idx += log->len + 1; // increment store buffer
    
#if _WIN32
    const u8 colors[LOG_COUNT] = {7, 6, 12, 4}; // order of severity, starting with tell.
    
    HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h_console, colors[severity]);
    printf("%s\n", log->ptr);
    
    SetConsoleTextAttribute(h_console, 15); // reset console font colors
#else
    printf("%s\n", log->ptr);
#endif
    
    switch(severity) {
        case LOG_PANIC: {
            __debugbreak();
        } break;
        case LOG_SHUTDOWN: {
            c_assert_break();
        } break;
    }
}

void logger_shutdown() {
    char logger_file_name[255] = {0};
    char time_str[255] = {0};
    u32 len = log_get_system_time_str(time_str, logger_file_name, true);
    char *ptr = &logger_file_name[len];
    
    const char *file_name_ending = ".txt";
    memcpy(ptr, file_name_ending, strlen(file_name_ending));
    
    skate_directory_t dir = get_directory_for(logger_file_name, SKATE_DIRECTORY_LOGS);
    FILE *f = fopen((char*)&dir.ptr[0], "w");
    if(f) {
        for(s32 i = 0; i < logs.log_idx; ++i) {
            log_t *log = &logs.logs[i];
            fwrite(log->ptr, sizeof(u8), log->len, f);
        }
        fclose(f);
    }
}

void logger_init() {
    logs.ptr = sokol_skate::mem_alloc(LOGGER_MEMORY_RESERVE);
    logs.ptr_idx = 0;
    logs.logs = (log_t*)sokol_skate::mem_alloc(sizeof(log_t) * LOGGER_LOGS_COUNT);
    LOG_TELL("logger enabled");
}