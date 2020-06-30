/* compress_safe.c -- compress a memory buffer with static memory
 * 2020 Neil Abcouwer
 */

/* @(#) $Id$ */

#define ZLIB_INTERNAL
#include "zutil.h"

//FIXME delete
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
int ZEXPORT compressSafeGzip2(dest, destLen, source, sourceLen, maxBlockLen,
        work, workLen, level, windowBits, memLevel, strategy, gz_head)
    Bytef *dest;                // destination buffer
    uLongf *destLen;            // destination buffer length
    const Bytef *source;        // source buffer
    uLong sourceLen;            // source buffer length
    uLong maxBlockLen;          // max length of a deflate block
    Bytef *work;                // work buffer
    uLong workLen;              // work buffer length
    int level;                  // compression level
    int windowBits;             // window size
    int memLevel;               // memory level
    int strategy;               // compression strategy
    gz_headerp gz_head;         // gzip header
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
    if (err != Z_OK) {
        return err;
    }

//    stream.next_out = dest;
//    stream.avail_out = *destLen;
//    stream.next_in = (z_const Bytef *)source;
//    stream.avail_in = sourceLen;

    // set gzip header, if not null
    if (gz_head != Z_NULL) {
        err = deflateSetHeader(&stream, gz_head);
        if (err != Z_OK) {
            return err;
        }
    }

    // check output buffer size, warn if small (but still might succeed)
    uLong bound1 = deflateBound(&stream, sourceLen);
    uLong bound2 = (uLong)(-1);
    err = compressGetMaxOutputSizeGzip2(sourceLen, maxBlockLen,
            windowBits, memLevel, gz_head, &bound2);
    if(err != Z_OK) {
        return err;
    }
    int dest_small = (*destLen < bound1) || (*destLen < bound2);
    (void)dest_small; // FIXME make compiler happy until used



    stream.next_out = dest;
    stream.avail_out = 0;
    stream.next_in = (z_const Bytef *)source;
    stream.avail_in = sourceLen;

    uLong bytes_left_dest = *destLen;

    int cycles = 0;
    do {
        if (stream.avail_out == 0) {
            // provide more output
            stream.avail_out =
                    bytes_left_dest < (uLong) maxBlockLen ?
                            (uInt) bytes_left_dest : maxBlockLen;
            bytes_left_dest -= stream.avail_out;
        }
        printf("deflate cycle %d. before: avail_in=%u, out=%u",
                cycles, stream.avail_in, stream.avail_out);
        err = deflate(&stream, Z_FINISH); // Z_FULL_FLUSH);

        printf(" after: avail_in=%u, out=%u\n",
                stream.avail_in, stream.avail_out);
        cycles++;
    } while (err == Z_OK);
    *destLen = stream.total_out;
    printf("compressed to %lu bytes.\n", stream.total_out);

//    // compress in one fell swoop
//    err = deflate(&stream, Z_FINISH);
//    *destLen = stream.total_out;
//
//    if(err != Z_STREAM_END) {
//        // when compressing with deflate(Z_FINISH),
//        // Z_STREAM_END is expected, not Z_OK
//        // Z_OK may indicate there wasn't enough output space
//        // FIXME warn and report if small buffer
//        (void)deflateEnd(&stream); // clean up
//        return (err == Z_OK) ? Z_BUF_ERROR : err;
//    }

    if(err != Z_STREAM_END) {
        // FIXME warn
        (void)deflateEnd(&stream); // clean up
        return err;
    }

    err = deflateEnd(&stream);
    // FIXME warn if bad

    return err;
}

// compress using a work buffer instead of dynamic memory
int ZEXPORT compressSafe2(dest, destLen, source, sourceLen, maxBlockLen,
        work, workLen, level, windowBits, memLevel, strategy)
    Bytef *dest;                // destination buffer
    uLongf *destLen;            // destination buffer length
    const Bytef *source;        // source buffer
    uLong sourceLen;            // source buffer length
    uLong maxBlockLen;          // max length of a deflate block
    Bytef *work;                // work buffer
    uLong workLen;              // work buffer length
    int level;                  // compression level
    int windowBits;             // window size
    int memLevel;               // memory level
    int strategy;               // compression strategy
{
    return compressSafeGzip2(dest, destLen, source, sourceLen, maxBlockLen,
            work, workLen, level, windowBits, memLevel, strategy, Z_NULL);
}


int ZEXPORT compressSafeGzip(dest, destLen, source, sourceLen, maxBlockLen,
        work, workLen, level, gz_head)
    Bytef *dest;                // destination buffer
    uLongf *destLen;            // destination buffer length
    const Bytef *source;        // source buffer
    uLong sourceLen;            // source buffer length
    uLong maxBlockLen;          // max length of a deflate block
    Bytef *work;                // work buffer
    uLong workLen;              // work buffer length
    int level;                  // compression level
    gz_headerp gz_head;         // gzip header
{
    return compressSafeGzip2(dest, destLen, source, sourceLen, maxBlockLen,
            work, workLen, level, DEF_WBITS + GZIP_CODE,
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, gz_head);
}

int ZEXPORT compressSafe(dest, destLen, source, sourceLen, maxBlockLen,
        work, workLen, level)
    Bytef *dest;                // destination buffer
    uLongf *destLen;            // destination buffer length
    const Bytef *source;        // source buffer
    uLong sourceLen;            // source buffer length
    uLong maxBlockLen;          // max length of a deflate block
    Bytef *work;                // work buffer
    uLong workLen;              // work buffer length
    int level;                  // compression level
{
    return compressSafe2(dest, destLen, source, sourceLen, maxBlockLen,
            work, workLen, level, DEF_WBITS,
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
}



// get the bounded size of the work buffer
int ZEXPORT compressGetMinWorkBufSize2(windowBits, memLevel, size_out)
    int windowBits;     // window size
    int memLevel;       // memory level
    uLongf *size_out;   // bounded size of work buffer
{
    return deflateGetMinWorkBufSize(windowBits, memLevel, size_out);
}

int ZEXPORT compressGetMinWorkBufSize(size_out)
    uLongf *size_out;   // bounded size of work buffer
{
    return compressGetMinWorkBufSize2(DEF_WBITS, DEF_MEM_LEVEL, size_out);
}

int ZEXPORT compressGetMaxOutputSizeGzip2(sourceLen, maxBlockLen,
        windowBits, memLevel, gz_head, size_out)
    uLong sourceLen;    // source buffer length
    uLong maxBlockLen;  // max length of a deflate block
    int windowBits;     // window size
    int memLevel;       // memory level
    gz_headerp gz_head; // gzip header
    uLongf *size_out;   // bounded size of work buffer
{
    int ret = deflateBoundNoStream(sourceLen,
            windowBits, memLevel, gz_head, size_out);
    if (ret != Z_OK) {
        return ret;
    }
    // if maxBlockLen < size_out, we may compress to more than one block
    // of size <= maxBlockLen for data protection
    // add addition bound for the each restart
    // FIXME assert not 0
    int num_extra_blocks = (*size_out / maxBlockLen);
    // four extra bytes are stored per block
    *size_out += num_extra_blocks * 4;
    return ret;
}

int ZEXPORT compressGetMaxOutputSize2(sourceLen, maxBlockLen,
        windowBits, memLevel, size_out)
    uLong sourceLen;    // source buffer length
    uLong maxBlockLen;  // max length of a deflate block
    int windowBits;     // window size
    int memLevel;       // memory level
    uLongf *size_out;   // bounded size of work buffer
{
    return compressGetMaxOutputSizeGzip2(sourceLen, maxBlockLen,
            windowBits, memLevel, Z_NULL, size_out);
}

int ZEXPORT compressGetMaxOutputSizeGzip(
        sourceLen, maxBlockLen, gz_head, size_out)
    uLong sourceLen;    // source buffer length
    uLong maxBlockLen;  // max length of a deflate block
    gz_headerp gz_head; // gzip header
    uLongf *size_out;   // bounded size of work buffer
{
    return compressGetMaxOutputSizeGzip2(sourceLen, maxBlockLen,
            DEF_WBITS + GZIP_CODE, DEF_MEM_LEVEL, gz_head, size_out);
}

int ZEXPORT compressGetMaxOutputSize(sourceLen, maxBlockLen, size_out)
    uLong sourceLen;    // source buffer length
    uLong maxBlockLen;  // max length of a deflate block
    uLongf *size_out;   // bounded size of work buffer
{
    return compressGetMaxOutputSize2(sourceLen, maxBlockLen,
            DEF_WBITS, DEF_MEM_LEVEL, size_out);
}

