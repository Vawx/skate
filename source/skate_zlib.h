/* date = July 31st 2025 2:42 pm */

#ifndef SKATE_ZLIB_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

namespace skate_zlib {
    /* report a zlib or i/o error */
    void zerr(int ret, FILE *in, FILE *out);
    
    /* Compress from file source to file dest until EOF on source.
           def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
           allocated for processing, Z_STREAM_ERROR if an invalid compression
           level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
           version of the library linked do not match, or Z_ERRNO if there is
           an error reading or writing the files. */
    int def(FILE *source, FILE *dest, int level);
    
    /* Decompress from file source to file dest until stream ends or EOF.
       inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
       allocated for processing, Z_DATA_ERROR if the deflate data is
       invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
       the version of the library linked do not match, or Z_ERRNO if there
       is an error reading or writing the files. */
    int inf(FILE *source, FILE *dest);
};

#define SKATE_ZLIB_H
#endif //SKATE_ZLIB_H
