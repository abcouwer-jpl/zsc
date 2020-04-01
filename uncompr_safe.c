/* uncompr.c -- decompress a memory buffer
 * Copyright (C) 1995-2003, 2010, 2014, 2016 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#define ZLIB_INTERNAL
#include "zlib.h"

// FIXME REMOVE
#include <stdio.h>

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
uLong ZEXPORT uncompressSafeBoundWork2(windowBits)
    int windowBits;
{
    return inflateBoundAlloc2(windowBits);
}

// get the bounded size of the work buffer
uLong ZEXPORT uncompressSafeBoundWork(void)
{
    return inflateBoundAlloc();
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
    // check if workbuffer is large enough
    uLong bound = uncompressSafeBoundWork2(windowBits);
    if (workLen < bound) {
        // work buffer is smaller than bound
        // FIXME warn
        return Z_MEM_ERROR;
        // FIXME could adjust windowBits until it works?
    }

    uncompress_safe_static_mem mem;
    // FIXME zmemzero(&mem, sizeof(mem));
    mem.work = work;
    mem.workLen = workLen;
    mem.workAlloced = 0;

    z_stream stream;
    // FIXME zmemzero(&stream, sizeof(stream));
    int err;
    const uInt max = (uInt)-1;
    uLong len, left;
    Byte buf[1];    /* for detection of incomplete stream when *destLen == 0 */

    len = *sourceLen;
    if (*destLen) {
        left = *destLen;
        *destLen = 0;
    }
    else {
        left = 1;
        dest = buf;
    }

    stream.next_in = (z_const Bytef *)source;
    stream.avail_in = 0;
    stream.zalloc = uncompress_safe_static_alloc;
    stream.zfree = uncompress_safe_static_free;
    stream.opaque = (voidpf)&mem;

    err = inflateInit2(&stream, windowBits);
    if (err != Z_OK) return err;

    stream.next_out = dest;
    stream.avail_out = 0;

    do {
        if (stream.avail_out == 0) {
            stream.avail_out = left > (uLong)max ? max : (uInt)left;
            left -= stream.avail_out;
        }
        if (stream.avail_in == 0) {
            stream.avail_in = len > (uLong)max ? max : (uInt)len;
            len -= stream.avail_in;
        }
        err = inflate(&stream, Z_NO_FLUSH);
    } while (err == Z_OK);

    *sourceLen -= len + stream.avail_in;
    if (dest != buf)
        *destLen = stream.total_out;
    else if (stream.total_out && err == Z_BUF_ERROR)
        left = 1;

    inflateEnd(&stream);
    return err == Z_STREAM_END ? Z_OK :
           err == Z_NEED_DICT ? Z_DATA_ERROR  :
           err == Z_BUF_ERROR && left + stream.avail_out ? Z_DATA_ERROR :
           err;
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
            MAX_WBITS);
}




