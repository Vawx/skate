#include "fbx_importer.h"

int main(int argc, char **argv) {
    fbx_dir source_dir = fbx_dir(argv[1]);
    fbx_dir output_dir = fbx_dir(argv[2]);
	fbx_importer importer(&source_dir, &output_dir, true);
    return 0;
}
