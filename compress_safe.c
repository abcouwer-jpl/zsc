/* compress_safe.c -- compress a memory buffer with static memory
 * 2020 Neil Abcouwer
 */

/* @(#) $Id$ */

#define ZLIB_INTERNAL
#include "zlib.h"

// FIXME REMOVE
#include <stdio.h>

#ifndef DEF_WBITS
#  define DEF_WBITS MAX_WBITS
#endif
/* default windowBits for decompression. MAX_WBITS is for compression only */

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
/* default memLevel */

typedef struct compress_safe_static_mem_s {
    Bytef* work; // work buffer
    uLong workLen; // work length
    uLong workAlloced; // work length allocated
} compress_safe_static_mem;

// allocate from the work buffer
voidpf compress_safe_static_alloc(voidpf opaque, uInt items, uInt size)
{
    voidpf new_ptr = Z_NULL;
    compress_safe_static_mem* mem = (compress_safe_static_mem*) opaque;
    // FIXME assert not null
    uLong bytes = items * size;
    // FIXME assert mult didn't overflow

    // check there's enough space
    if (mem->workAlloced + bytes > mem->workLen) {
        // FIXME warn?
        return Z_NULL;
    }

    new_ptr = (voidpf) (mem->work + mem->workAlloced);
    mem->workAlloced += bytes;
    // FIXME remove
    printf("Compress allocated %lu bytes, total %lu.\n",
            bytes, mem->workAlloced);
    return new_ptr;
}

void compress_safe_static_free(void* opaque, void* addr)
{
    // do nothing but make compiler happy
    (void) opaque;
    (void) addr;
}

/* ===========================================================================
     Compresses the source buffer into the destination buffer,
   using memory from the work buffer.  The level
   parameter has the same meaning as in deflateInit.  sourceLen is the byte
   length of the source buffer. Upon entry, destLen is the total size of the
   destination buffer, which must be at least 0.1% larger than sourceLen plus
   12 bytes. Upon exit, destLen is the actual size of the compressed buffer.
   workLen is the size of the work buffer.

     If gz_header is not null, will use that as the gzip header,
   if windowBits set appropriately for gzip header.

     compress_safe returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
   Z_STREAM_ERROR if the level parameter is invalid.
*/

// compress using a work buffer instead of dynamic memory
int ZEXPORT compressSafeGzip2(dest, destLen, source, sourceLen,
        work, workLen, level, windowBits, memLevel, strategy, gz_head)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong sourceLen;
    Bytef *work; // work buffer
    uLong workLen; // work length
    int level;
    int windowBits;
    int memLevel;
    int strategy;
    gz_headerp gz_head;
{
    // check if work buffer is large enough
    uLong min_work_buf_size = (uLong)(-1);
    int err = deflateGetMinWorkBufSize(windowBits, memLevel,
            &min_work_buf_size);
    if (err != Z_OK) { return err; }
    if ((uLong)workLen < min_work_buf_size) {
        // work buffer is smaller than bound
        // FIXME warn
        return Z_MEM_ERROR;
    }

    compress_safe_static_mem mem;
    mem.work = work;
    mem.workLen = workLen;
    mem.workAlloced = 0;

//    const uInt max = (uInt)(-1);

//    uLong left = *destLen;
//    *destLen = 0;

    z_stream stream;
    // FIXME zmemzero(&stream, sizeof(stream));
    stream.zalloc = compress_safe_static_alloc;
    stream.zfree = compress_safe_static_free;
    stream.opaque = (voidpf)&mem;

    // see zlib.h for description of parameters
    err = deflateInit2(&stream, level, Z_DEFLATED, windowBits, memLevel,
            strategy);
    if (err != Z_OK) { return err; }

    stream.next_out = dest;
    stream.avail_out = *destLen;
    stream.next_in = (z_const Bytef *)source;
    stream.avail_in = sourceLen;

    // set gzip header, if not null
    if (gz_head != Z_NULL) {
        err = deflateSetHeader(&stream, gz_head);
        if (err != Z_OK) { return err; }
    }

    // FIXME check output buffer size, warn if small (but still might succeed)

    // compress in one fell swoop
    err = deflate(&stream, Z_FINISH);
    *destLen = stream.total_out;

    if(err != Z_STREAM_END) {
        // when compressing with deflate(Z_FINISH),
        // Z_STREAM_END is expected, not Z_OK
        // Z_OK may indicate there wasn't enough output space
        (void)deflateEnd(&stream); // clean up
        return (err == Z_OK) ? Z_MEM_ERROR : err;
    }

    err = deflateEnd(&stream);
    // FIXME warn if bad

    return err;



//
//    // FIXME warning msg
//    return err == Z_STREAM_END ? Z_OK : err;
//
//
//
//    stream.next_out = dest;
//    stream.avail_out = 0;
//    stream.next_in = (z_const Bytef *)source;
//    stream.avail_in = 0;
//
//    // set gzip header, if not null
//    if (gz_head != Z_NULL) {
//        err = deflateSetHeader(&stream, gz_head);
//        if (err != Z_OK) return err;
//    }
//
//    // FIXME bound loop
//    do {
//        if (stream.avail_out == 0) {
//            stream.avail_out = left > (uLong)max ? max : (uInt)left;
//            left -= stream.avail_out;
//        }
//        if (stream.avail_in == 0) {
//            stream.avail_in = sourceLen > (uLong)max ? max : (uInt)sourceLen;
//            sourceLen -= stream.avail_in;
//        }
//        err = deflate(&stream, sourceLen ? Z_NO_FLUSH : Z_FINISH);
//    } while (err == Z_OK);
//
//    *destLen = stream.total_out;
//    deflateEnd(&stream);
//    return err == Z_STREAM_END ? Z_OK : err;
}

// compress using a work buffer instead of dynamic memory
int ZEXPORT compressSafe2(dest, destLen, source, sourceLen,
        work, workLen, level, windowBits, memLevel, strategy)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong sourceLen;
    Bytef *work; // work buffer
    uLong workLen; // work length
    int level;
    int windowBits;
    int memLevel;
    int strategy;
{
    return compressSafeGzip2(dest, destLen, source, sourceLen,
            work, workLen, level, windowBits, memLevel, strategy, Z_NULL);
}


int ZEXPORT compressSafeGzip(dest, destLen, source, sourceLen,
        work, workLen, gz_head)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong sourceLen;
    Bytef *work; // work buffer
    uLong workLen; // work length
    gz_headerp gz_head;
{
    return compressSafeGzip2(dest, destLen, source, sourceLen,
            work, workLen, Z_DEFAULT_COMPRESSION, DEF_WBITS + 16, // magic num for gzip
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, gz_head);
}

int ZEXPORT compressSafe(dest, destLen, source, sourceLen,
        work, workLen)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong sourceLen;
    Bytef *work; // work buffer
    uLong workLen; // work length
{
    return compressSafe2(dest, destLen, source, sourceLen,
            work, workLen, Z_DEFAULT_COMPRESSION, DEF_WBITS,
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
}



// get the bounded size of the work buffer
int ZEXPORT compressGetMinWorkBufSize2(windowBits, memLevel, size_out)
    int windowBits;
    int memLevel;
    uLongf *size_out;
{
    return deflateGetMinWorkBufSize(windowBits, memLevel, size_out);
}

int ZEXPORT compressGetMinWorkBufSize(size_out)
    uLongf *size_out;
{
    return compressGetMinWorkBufSize2(DEF_WBITS, DEF_MEM_LEVEL, size_out);
}

int ZEXPORT compressGetMaxOutputSizeGzip2(sourceLen,
        windowBits, memLevel, gz_head, size_out)
    uLong sourceLen;
    int windowBits;
    int memLevel;
    gz_headerp gz_head;
    uLongf *size_out;
{
    return deflateBoundNoStream(sourceLen, windowBits, memLevel, gz_head, size_out);
}

int ZEXPORT compressGetMaxOutputSize2(sourceLen, windowBits, memLevel, size_out)
    uLong sourceLen;
    int windowBits;
    int memLevel;
    uLongf *size_out;
{
    return compressGetMaxOutputSizeGzip2(sourceLen,
            windowBits, memLevel, Z_NULL, size_out);
}

int ZEXPORT compressGetMaxOutputSizeGzip(sourceLen, gz_head, size_out)
    uLong sourceLen;
    gz_headerp gz_head;
    uLongf *size_out;
{
    return compressGetMaxOutputSizeGzip2(sourceLen,
            DEF_WBITS + 16, DEF_MEM_LEVEL, gz_head, size_out);
}

int ZEXPORT compressGetMaxOutputSize(sourceLen, size_out)
    uLong sourceLen;
    uLongf *size_out;
{
    return compressGetMaxOutputSizeGzip(sourceLen, Z_NULL, size_out);
}

