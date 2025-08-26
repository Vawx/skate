#include "fbx_zlib.h"

#include "windows.h"

namespace fbx_zlib {
    
    voidpf zcalloc(voidpf opaque, unsigned items, unsigned size) {
        (void)opaque;
        return (voidpf)VirtualAlloc(0, items * size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }
    
    void zcfree(voidpf opaque, voidpf ptr) {
        (void)opaque;
        VirtualFree(ptr, 0, MEM_RELEASE);
    }
    
    /* report a zlib or i/o error */
    void zerr(int ret, FILE *in, FILE *out) {
        fputs("zpipe: ", stderr);
        switch (ret) {
            case Z_ERRNO:
            if (ferror(in))
                printf("error reading in\n");
            if (ferror(out))
                printf("error writing out\n");
            break;
            case Z_STREAM_ERROR:
            printf("invalid compression level\n");
            break;
            case Z_DATA_ERROR:
            printf("invalid or incomplete deflate data\n");
            break;
            case Z_MEM_ERROR:
            printf("out of memory\n");
            break;
            case Z_VERSION_ERROR:
            printf("zlib version mismatch!\n");
        }
    }
    
    /* Compress from file source to file dest until EOF on source.
       def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
       allocated for processing, Z_STREAM_ERROR if an invalid compression
       level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
       version of the library linked do not match, or Z_ERRNO if there is
       an error reading or writing the files. */
    int def(FILE *source, FILE *dest, int level)
    {
        int ret, flush;
        unsigned have;
        z_stream strm;
        unsigned char in[CHUNK];
        unsigned char out[CHUNK];
        
        /* allocate deflate state */
        strm.zalloc = zcalloc;;
        strm.zfree = zcfree;
        strm.opaque = Z_NULL;
        ret = deflateInit(&strm, level);
        if (ret != Z_OK)
            return ret;
        
        /* compress until end of file */
        do {
            strm.avail_in = fread(in, 1, CHUNK, source);
            if (ferror(source)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
            flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
            strm.next_in = in;
            
            /* run deflate() on input until output buffer not full, finish
               compression if all of source has been read in */
            do {
                strm.avail_out = CHUNK;
                strm.next_out = out;
                ret = deflate(&strm, flush);    /* no bad return value */
                assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
                have = CHUNK - strm.avail_out;
                if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                    (void)deflateEnd(&strm);
                    return Z_ERRNO;
                }
            } while (strm.avail_out == 0);
            assert(strm.avail_in == 0);     /* all input will be used */
            
            /* done when last data in file processed */
        } while (flush != Z_FINISH);
        assert(ret == Z_STREAM_END);        /* stream will be complete */
        
        /* clean up and return */
        (void)deflateEnd(&strm);
        return Z_OK;
    }
    
    /* Decompress from file source to file dest until stream ends or EOF.
       inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
       allocated for processing, Z_DATA_ERROR if the deflate data is
       invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
       the version of the library linked do not match, or Z_ERRNO if there
       is an error reading or writing the files. */
    int inf(FILE *source, FILE *dest)
    {
        int ret;
        unsigned have;
        z_stream strm;
        unsigned char in[CHUNK];
        unsigned char out[CHUNK];
        
        /* allocate inflate state */
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;
        ret = inflateInit(&strm);
        if (ret != Z_OK)
            return ret;
        
        /* decompress until deflate stream ends or end of file */
        do {
            strm.avail_in = fread(in, 1, CHUNK, source);
            if (ferror(source)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
            if (strm.avail_in == 0)
                break;
            strm.next_in = in;
            
            /* run inflate() on input until output buffer not full */
            do {
                strm.avail_out = CHUNK;
                strm.next_out = out;
                ret = inflate(&strm, Z_NO_FLUSH);
                assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
                switch (ret) {
                    case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;     /* and fall through */
                    case Z_DATA_ERROR:
                    case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
                }
                have = CHUNK - strm.avail_out;
                if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                    (void)inflateEnd(&strm);
                    return Z_ERRNO;
                }
            } while (strm.avail_out == 0);
            
            /* done when inflate() says it's done */
        } while (ret != Z_STREAM_END);
        
        /* clean up and return */
        (void)inflateEnd(&strm);
        return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
    }
    
    int compress(u8 *dest, u64 *dest_len, const u8 *source, u64 source_len) {
        z_stream stream;
        int err;
        const u32 max = (u32)-1;
        uLong left;
        
        left = *dest_len;
        *dest_len = 0;
        
        stream.zalloc = zcalloc;
        stream.zfree = zcfree;
        stream.opaque = (voidpf)0;
        
        err = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
        if (err != Z_OK) return err;
        
        stream.next_out = dest;
        stream.avail_out = 0;
        stream.next_in = (z_const Bytef *)source;
        stream.avail_in = 0;
        
        do {
            if (stream.avail_out == 0) {
                stream.avail_out = left > (uLong)max ? max : (uInt)left;
                left -= stream.avail_out;
            }
            if (stream.avail_in == 0) {
                stream.avail_in = source_len > (uLong)max ? max : (uInt)source_len;
                source_len -= stream.avail_in;
            }
            err = deflate(&stream, source_len ? Z_NO_FLUSH : Z_FINISH);
        } while (err == Z_OK);
        
        *dest_len = stream.total_out;
        deflateEnd(&stream);
        return err == Z_STREAM_END ? Z_OK : err;
    }
}; // skate_zlib