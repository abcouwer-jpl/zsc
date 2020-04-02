/* uncompr.c -- decompress a memory buffer
 * Copyright (C) 1995-2003, 2010, 2014, 2016 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
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

#define GZIP_CODE (16)

typedef struct uncompress_safe_static_mem_s {
    Bytef *work; // work buffer
    uLong workLen; // work length
    uLong workAlloced; // work length allocated
} uncompress_safe_static_mem;

// allocate from the work buffer
voidpf uncompress_safe_static_alloc(voidpf opaque, uInt items, uInt size)
{
    voidpf new_ptr = Z_NULL;
    uncompress_safe_static_mem* mem = (uncompress_safe_static_mem*) opaque;
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
    printf("Uncompress allocated %lu bytes, total %lu.\n",
            bytes, mem->workAlloced);
    return new_ptr;
}

void uncompress_safe_static_free(void* opaque, void* addr)
{
    // do nothing but make compiler happy
    (void) opaque;
    (void) addr;
}

// get the bounded size of the work buffer
int ZEXPORT uncompressGetMinWorkBufSize2(windowBits, size_out)
    int windowBits;
    uLongf *size_out;
{
    return inflateGetMinWorkBufSize2(windowBits, size_out);
}

// get the bounded size of the work buffer
int ZEXPORT uncompressGetMinWorkBufSize(size_out)
    uLongf *size_out;
{
    return inflateGetMinWorkBufSize(size_out);
}

int ZEXPORT uncompressSafeGzip2(dest, destLen, source, sourceLen, work, workLen,
        windowBits, gz_head)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong *sourceLen;
    Bytef *work; // work buffer
    uLong workLen;
    int windowBits;
    gz_headerp gz_head;
{
    // check if workbuffer is large enough
    uLong min_work_buf_size = (uLong)(-1);

    int err = uncompressGetMinWorkBufSize2(windowBits, &min_work_buf_size);
    if (err != Z_OK) {
        return err;
    }
    if (workLen < min_work_buf_size) {
        // work buffer is smaller than bound
        // FIXME warn
        printf("work buffer is smaller than bound.\n");
        return Z_MEM_ERROR;
        // FIXME could adjust windowBits until it works?
    }

    uncompress_safe_static_mem mem;
    // FIXME zmemzero(&mem, sizeof(mem));
    mem.work = work;
    mem.workLen = workLen;
    mem.workAlloced = 0;

    z_stream stream;
    stream.next_in = (z_const Bytef *)source;
    stream.avail_in = *sourceLen;
    stream.next_out = dest;
    stream.avail_out = *destLen;
    stream.zalloc = uncompress_safe_static_alloc;
    stream.zfree = uncompress_safe_static_free;
    stream.opaque = (voidpf)&mem;

    err = inflateInit2(&stream, windowBits);
    if (err != Z_OK) return err;

    if(gz_head != Z_NULL) {
        err = inflateGetHeader(&stream, gz_head);
        if (err != Z_OK) return err;
    }

    // inflate in one swoop
    err = inflate(&stream, Z_FINISH);
    *destLen = stream.total_out;
    *sourceLen = stream.total_in;

    if(err != Z_STREAM_END) {
        // when uncompressing with inflate(Z_FINISH),
        // Z_STREAM_END is expected, not Z_OK
        // Z_OK may indicate there wasn't enough output space
        (void)inflateEnd(&stream); // clean up
        return (err == Z_OK) ? Z_MEM_ERROR : err;
    }

    err = inflateEnd(&stream);
    // FIXME warn if bad

    return err;
}

int ZEXPORT uncompressSafe2(dest, destLen, source, sourceLen, work, workLen,
        windowBits)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong *sourceLen;
    Bytef *work; // work buffer
    uLong workLen;
    int windowBits;
{
    return uncompressSafeGzip2(dest, destLen, source, sourceLen, work, workLen,
            windowBits, Z_NULL);
}

int ZEXPORT uncompressSafe (dest, destLen, source, sourceLen, work, workLen)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong *sourceLen;
    Bytef *work; // work buffer
    uLong workLen;
{
    return uncompressSafe2(dest, destLen, source, sourceLen, work, workLen,
            DEF_WBITS);
}

int ZEXPORT uncompressSafeGzip(dest, destLen, source, sourceLen, work, workLen,
        gz_head)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong *sourceLen;
    Bytef *work; // work buffer
    uLong workLen;
    gz_headerp gz_head;
{
    return uncompressSafeGzip2(dest, destLen, source, sourceLen, work, workLen,
            DEF_WBITS + GZIP_CODE, gz_head);
}


