/* compress_safe.c -- compress a memory buffer with static memory
 * 2020 Neil Abcouwer
 */

/* @(#) $Id$ */

#define ZLIB_INTERNAL
#include "zlib.h"

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
    return new_ptr;
}

void zs_static_free(void* opaque, void* addr)
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
    uLong bound = deflateBoundAlloc(windowBits, memLevel);
    if (workLen < bound) {
        // work buffer is smaller than bound
        // FIXME warn
        return Z_MEM_ERROR;
        // FIXME could adjust Bits and memLevel until it works?
    }

    compress_safe_static_mem mem;
    //FIXME zmemzero(&mem, sizeof(mem));
    mem.work = work;
    mem.workLen = workLen;
    mem.workAlloced = 0;

    const uInt max = (uInt)-1;

    uLong left = *destLen;
    *destLen = 0;

    z_stream stream;
    // FIXME zmemzero(&stream, sizeof(stream));
    stream.zalloc = compress_safe_static_alloc;
    stream.zfree = zs_static_free;
    stream.opaque = (voidpf)&mem;

    // see zlib.h for description of parameters
    int err = deflateInit2(&stream, level, Z_DEFLATED, windowBits, memLevel,
            strategy);
    if (err != Z_OK) return err;

    stream.next_out = dest;
    stream.avail_out = 0;
    stream.next_in = (z_const Bytef *)source;
    stream.avail_in = 0;

    // set gzip header, if not null
    if (gz_head != Z_NULL) {
        err = deflateSetHeader(&stream, gz_head);
        if (err != Z_OK) return err;
    }

    // FIXME bound loop
    do {
        if (stream.avail_out == 0) {
            stream.avail_out = left > (uLong)max ? max : (uInt)left;
            left -= stream.avail_out;
        }
        if (stream.avail_in == 0) {
            stream.avail_in = sourceLen > (uLong)max ? max : (uInt)sourceLen;
            sourceLen -= stream.avail_in;
        }
        err = deflate(&stream, sourceLen ? Z_NO_FLUSH : Z_FINISH);
    } while (err == Z_OK);

    *destLen = stream.total_out;
    deflateEnd(&stream);
    return err == Z_STREAM_END ? Z_OK : err;
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
            work, workLen, Z_DEFAULT_COMPRESSION, MAX_WBITS, MAX_MEM_LEVEL,
            Z_DEFAULT_STRATEGY, gz_head);
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
    return compressSafeGzip(dest, destLen, source, sourceLen,
            work, workLen, Z_NULL);
}



// get the bounded size of the work buffer
uLong ZEXPORT compressSafeBoundWork2(windowBits, memLevel)
    int windowBits;
    int memLevel;
{
    return deflateBoundAlloc(windowBits, memLevel);
}

uLong ZEXPORT compressSafeBoundWork(void)
{
    return compressSafeBoundWork2(MAX_WBITS, MAX_MEM_LEVEL);
}

uLong ZEXPORT compressSafeBoundDestGzip2(sourceLen,
        windowBits, memLevel, gz_head)
    uLong sourceLen;
    int windowBits;
    int memLevel;
    gz_headerp gz_head;
{
    return deflateBoundDestNoStream(sourceLen,
            windowBits, memLevel, gz_head);
}

uLong ZEXPORT compressSafeBoundDest2(sourceLen, windowBits, memLevel)
    uLong sourceLen;
    int windowBits;
    int memLevel;
{
    return compressSafeBoundDestGzip2(sourceLen,
            windowBits, memLevel, Z_NULL);
}

uLong ZEXPORT compressSafeBoundDestGzip(sourceLen, gz_head)
    uLong sourceLen;
    gz_headerp gz_head;
{
    return compressSafeBoundDestGzip2(sourceLen,
            MAX_WBITS, MAX_MEM_LEVEL, gz_head);
}

uLong ZEXPORT compressSafeBoundDest(sourceLen)
    uLong sourceLen;
{
    return compressSafeBoundDestGzip(sourceLen, Z_NULL);
}

