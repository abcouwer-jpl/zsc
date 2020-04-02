/* compress_safe.c -- compress a memory buffer with static memory
 * 2020 Neil Abcouwer
 */

/* @(#) $Id$ */

#define ZLIB_INTERNAL
#include "zutil.h"

// FIXME REMOVE
#include <stdio.h>

#define GZIP_CODE (16)


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
    if (err != Z_OK) {
        return err;
    }
    if (workLen < min_work_buf_size) {
        // work buffer is smaller than bound
        // FIXME warn
        return Z_MEM_ERROR;
    }

    z_static_mem mem;
    zmemzero(&mem, sizeof(mem));
    mem.work = work;
    mem.workLen = workLen;
    mem.workAlloced = 0;

    z_stream stream;
    zmemzero(&stream, sizeof(stream));
    stream.zalloc = z_static_alloc;
    stream.zfree = z_static_free;
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
            work, workLen, Z_DEFAULT_COMPRESSION, DEF_WBITS + GZIP_CODE,
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
            DEF_WBITS + GZIP_CODE, DEF_MEM_LEVEL, gz_head, size_out);
}

int ZEXPORT compressGetMaxOutputSize(sourceLen, size_out)
    uLong sourceLen;
    uLongf *size_out;
{
    return compressGetMaxOutputSize2(sourceLen,
            DEF_WBITS, DEF_MEM_LEVEL, size_out);
}

