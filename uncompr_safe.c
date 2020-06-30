/* uncompr.c -- decompress a memory buffer
 * Copyright (C) 1995-2003, 2010, 2014, 2016 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#define ZLIB_INTERNAL
#include "zutil.h"

// FIXME REMOVE
#include <stdio.h>

#define GZIP_CODE (16)

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

    z_static_mem mem;
    zmemzero(&mem, sizeof(mem));
    mem.work = work;
    mem.workLen = workLen;
    mem.workAlloced = 0;

    z_stream stream;
    zmemzero(&stream, sizeof(stream));
    stream.next_in = (z_const Bytef *)source;
    stream.avail_in = *sourceLen;
    stream.next_out = dest;
    stream.avail_out = *destLen;
    stream.zalloc = z_static_alloc;
    stream.zfree = z_static_free;
    stream.opaque = (voidpf)&mem;

    err = inflateInit2(&stream, windowBits);
    if (err != Z_OK) {
        // might be unreachable, as windowbits is checked above
        return err;
    }

    if(gz_head != Z_NULL) {
        err = inflateGetHeader(&stream, gz_head);
        if (err != Z_OK) {
            printf("inflateGetHeader failed\n");
            return err;
        }
    }


    int got_data_err = 0;
    int cycles = 0;
    do {
        printf("inflate cycle %d. before: avail_in=%u, out=%u\n",
                cycles, stream.avail_in, stream.avail_out);
        err = inflate(&stream, Z_FINISH);
        printf(" after: avail_in=%u, out=%u, err = %d\n",
                stream.avail_in, stream.avail_out, err);
        if (err == Z_DATA_ERROR) {
            // there was probably some corruption in the buffer
            got_data_err = 1;
            // FIXME warn
            printf("data error.\n");
            if (stream.msg != Z_NULL) {
                printf("msg: %s\n", stream.msg);
            }
            // try to find a new flush point to recover some partial data
            err = inflateSync(&stream);
            printf(" after sync: avail_in=%u, out=%u\n",
                    stream.avail_in, stream.avail_out);

            if (err == Z_OK) {
                printf("found new flush point\n");
            } else {

                printf("inflateSync() returned %d, "
                        "could not find a new flush point.\n", err);
            }
        }
        cycles++;
    } while (err == Z_OK);

//    // inflate in one swoop
//    err = inflate(&stream, Z_FINISH);

    *destLen = stream.total_out;
    *sourceLen = stream.total_in;

    if(err != Z_STREAM_END) {
        printf("inflate failed\n");

        // when uncompressing with inflate(Z_FINISH),
        // Z_STREAM_END is expected, not Z_OK
        // Z_OK may indicate there wasn't enough output space
        (void)inflateEnd(&stream); // clean up
        return (err == Z_OK) ? Z_MEM_ERROR : err;
    }

    err = inflateEnd(&stream);
    // FIXME warn if bad

    if (err == Z_OK && got_data_err) {
        err = Z_DATA_ERROR;
    }

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


