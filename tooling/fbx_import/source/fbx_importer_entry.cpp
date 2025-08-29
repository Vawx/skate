
static char version_file_dir[255] = {0};
static int version_file_dir_len = 0;

#include "fbx_importer.h"

int main(int argc, char **argv) {
    
    char file_dir[255] = {0};
    int file_dir_len = 0;
    
    int len = strlen(argv[0]);
    memcpy(&file_dir[file_dir_len], argv[0], len);
    file_dir_len += len;
    for(int i = 0; i < len; ++i) {
        if(file_dir[i] == '\\') {
            file_dir[i] = '/';
        }
    }
    
    char *end = &file_dir[file_dir_len];
    while(*end != '/') {--end;}
    
    char *tt = file_dir;
    while(tt != end) {
        version_file_dir[version_file_dir_len++] = *tt++;
    }
    version_file_dir[version_file_dir_len++] = '/';
    const char *file_ext = ".version\0";
    const int file_ext_len = 9;
    for(int i = 0; i < file_ext_len; ++i) {
        version_file_dir[version_file_dir_len++] = file_ext[i];
    }
    
    // check to see if version file is there
    FILE *t = fopen(version_file_dir, "r");
    if(!t) {
        return FBX_IMPORTER_CODE::FAILED_MISSING_VERSION_FILE;
    }
    fclose(t);
    
    fbx_dir version_dir = fbx_dir(version_file_dir);
    fbx_dir source_dir = fbx_dir(argv[1]);
    fbx_dir output_dir = fbx_dir(argv[2]);
	fbx_importer importer(&version_dir, &source_dir, &output_dir, true);
    return importer.importer_success_code;
}
