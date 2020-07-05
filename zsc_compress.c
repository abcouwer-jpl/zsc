/***********************************************************************
 * Copyright 2020, by the California Institute of Technology.
 * ALL RIGHTS RESERVED. United States Government Sponsorship acknowledged.
 * Any commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 *
 * This software may be subject to U.S. export control laws.
 * By accepting this software, the user agrees to comply with
 * all applicable U.S. export laws and regulations. User has the
 * responsibility to obtain export licenses, or other export authority
 * as may be required before exporting such information to foreign
 * countries or providing access to foreign persons.
 *
 * @file        zsc_compress.c
 * @date        2020-07-01
 * @author      Neil Abcouwer
 * @brief       Function definitions for safety-critical compressing.
 */

#include "zsc_pub.h"
#include "deflate.h"
#include "zutil.h"


//FIXME delete
#include <stdio.h>

#define GZIP_CODE (16)


/* ===========================================================================
     Compresses the source buffer into the destination buffer,
   using memory from the work buffer.  The level
   parameter has the same meaning as in deflateInit.  source_len is the byte
   length of the source buffer. Upon entry, dest_len is the total size of the
   destination buffer, which must be at least 0.1% larger than source_len plus
   12 bytes. Upon exit, dest_len is the actual size of the compressed buffer.
   work_len is the size of the work buffer.

     If gz_header is not null, will use that as the gzip header,
   if window_bits set appropriately for gzip header.

     compress_safe returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
   Z_STREAM_ERROR if the level parameter is invalid.
*/

// compress using a work buffer instead of dynamic memory
int ZEXPORT zsc_compress_gzip2(dest, dest_len, source, source_len, max_block_len,
        work, work_len, level, window_bits, mem_level, strategy, gz_head)
    Bytef *dest;                // destination buffer
    uLongf *dest_len;            // destination buffer length
    const Bytef *source;        // source buffer
    uLong source_len;            // source buffer length
    uLong max_block_len;          // max length of a deflate block
    Bytef *work;                // work buffer
    uLong work_len;              // work buffer length
    int level;                  // compression level
    int window_bits;             // window size
    int mem_level;               // memory level
    int strategy;               // compression strategy
    gz_headerp gz_head;         // gzip header
{
        printf("zsc_compress_gzip2 source_len = %lu.\n",
                source_len);

    // check if work buffer is large enough
    uLong min_work_buf_size = (uLong)(-1);
    int err = zsc_compress_get_min_work_buf_size2(window_bits, mem_level,
            &min_work_buf_size);
    if (err != Z_OK) {
        return err;
    }
    if (work_len < min_work_buf_size) {
        // work buffer is smaller than bound
        // FIXME warn
        return Z_MEM_ERROR;
    }

    z_stream stream;
    zmemzero((Bytef*)&stream, sizeof(stream));
    stream.next_work = work;
    stream.avail_work = work_len;

    // see zlib.h for description of parameters
    err = deflateInit2(&stream, level, Z_DEFLATED, window_bits, mem_level,
            strategy);
    if (err != Z_OK) {
        return err;
    }

    // set gzip header, if not null
    if (gz_head != Z_NULL) {
        err = deflateSetHeader(&stream, gz_head);
        if (err != Z_OK) {
            return err;
        }
    }

    // check output buffer size, warn if small (but still might succeed)
    uLong bound1 = deflateBound(&stream, source_len);
    uLong bound2 = (uLong)(-1);
    err = zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
            level, window_bits, mem_level, gz_head, &bound2);
    if(err != Z_OK) {
        return err;
    }
    int dest_small = (*dest_len < bound1) || (*dest_len < bound2);
    (void)dest_small; // FIXME make compiler happy until used

    stream.next_out = dest;
    stream.avail_out = 0;
    stream.next_in = (const Bytef *)source;
    stream.avail_in = 0; //source_len;

    uLong bytes_left_dest = *dest_len;

    int cycles = 0;
    int output_blocks = 0;
    do {
        if (stream.avail_out == 0) {
            // provide more output
            stream.avail_out =
                    bytes_left_dest < (uLong) max_block_len ?
                            (uInt) bytes_left_dest : max_block_len;
            bytes_left_dest -= stream.avail_out;
            output_blocks++;
        }
        if (stream.avail_in == 0) {
            // provide more input
            stream.avail_in =
                    source_len < (uLong) max_block_len ?
                            (uInt) source_len : max_block_len;
            source_len -= stream.avail_in;
        }
        int flush = (source_len > 0) ? Z_FULL_FLUSH : Z_FINISH;
        printf("deflate cycle %d. before: bytes_left_dest = %lu source_len = %lu, "
                "avail_in=%u, out=%u, flush = %d, ",
                cycles, bytes_left_dest, source_len,
                stream.avail_in, stream.avail_out, flush);
        err = deflate(&stream, flush);

        printf(" after: avail_in=%u, out=%u, err = %d\n",
                stream.avail_in, stream.avail_out, err);
        cycles++;
    } while (err == Z_OK);
    *dest_len = stream.total_out;
    printf("Compressed to %lu bytes, %d cycles, %d blocks.\n",
            stream.total_out, cycles, output_blocks);

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
int ZEXPORT zsc_compress2(dest, dest_len, source, source_len, max_block_len,
        work, work_len, level, window_bits, mem_level, strategy)
    Bytef *dest;                // destination buffer
    uLongf *dest_len;            // destination buffer length
    const Bytef *source;        // source buffer
    uLong source_len;            // source buffer length
    uLong max_block_len;          // max length of a deflate block
    Bytef *work;                // work buffer
    uLong work_len;              // work buffer length
    int level;                  // compression level
    int window_bits;             // window size
    int mem_level;               // memory level
    int strategy;               // compression strategy
{
    return zsc_compress_gzip2(dest, dest_len, source, source_len, max_block_len,
            work, work_len, level, window_bits, mem_level, strategy, Z_NULL);
}


int ZEXPORT zsc_compress_gzip(dest, dest_len, source, source_len, max_block_len,
        work, work_len, level, gz_head)
    Bytef *dest;                // destination buffer
    uLongf *dest_len;            // destination buffer length
    const Bytef *source;        // source buffer
    uLong source_len;            // source buffer length
    uLong max_block_len;          // max length of a deflate block
    Bytef *work;                // work buffer
    uLong work_len;              // work buffer length
    int level;                  // compression level
    gz_headerp gz_head;         // gzip header
{
    return zsc_compress_gzip2(dest, dest_len, source, source_len, max_block_len,
            work, work_len, level, DEF_WBITS + GZIP_CODE,
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, gz_head);
}

int ZEXPORT zsc_compress(dest, dest_len, source, source_len, max_block_len,
        work, work_len, level)
    Bytef *dest;                // destination buffer
    uLongf *dest_len;            // destination buffer length
    const Bytef *source;        // source buffer
    uLong source_len;            // source buffer length
    uLong max_block_len;          // max length of a deflate block
    Bytef *work;                // work buffer
    uLong work_len;              // work buffer length
    int level;                  // compression level
{
    return zsc_compress2(dest, dest_len, source, source_len, max_block_len,
            work, work_len, level, DEF_WBITS,
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
}

// given window_bits and mem_level,
// calculate the minimum size of the work buffer required for deflation
// return as output param
// return error if any
int zsc_compress_get_min_work_buf_size2(window_bits, mem_level, size_out)
    int window_bits;
    int mem_level;
    uLongf *size_out;
{
    // FIXME assert size_out not NULL

    // give known value
    *size_out = (uLongf)(-1);

    // properize window_bits
    if (window_bits < 0) { /* suppress zlib wrapper */
        window_bits = -window_bits;
    }
#ifdef GZIP
    else if (window_bits > 15) {
        window_bits -= 16;
    }
#endif

    if (window_bits == 8) {
        window_bits = 9; /* until 256-byte window bug fixed */
    }

    if (mem_level < 1 || mem_level > MAX_MEM_LEVEL ||
        window_bits < 8 || window_bits > 15) {
        // FIXME warn
        return Z_STREAM_ERROR;
    }

    // see deflateInit2_() for these allocations
    uInt window_size = 1 << window_bits;
    uInt hash_bits = (uInt) mem_level + 7;
    uInt hash_size = 1 << hash_bits;
    uInt lit_bufsize = 1 << (mem_level + 6); /* 16K elements by default */

    uLong size = 0;
    size += sizeof(deflate_state);           // strm->state
    size += window_size * 2 * sizeof(Byte);  // strm->state->window
    size += window_size * 2 * sizeof(Pos);   // strm->state->prev
    size += hash_size * sizeof(Pos);         // strm->state->head
    size += lit_bufsize * (sizeof(ush) + 2); // strm->state->pending_buf

    *size_out = size;

    return Z_OK;
}

int zsc_compress_get_min_work_buf_size(size_out)
    uLongf *size_out;   // bounded size of work buffer
{
    return zsc_compress_get_min_work_buf_size2(DEF_WBITS, DEF_MEM_LEVEL, size_out);
}

int ZEXPORT zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
        level, window_bits, mem_level, gz_head, size_out)
    uLong source_len;    // source buffer length
    uLong max_block_len;  // max length of a deflate block
    int level;          // compression level
    int window_bits;     // window size
    int mem_level;       // memory level
    gz_headerp gz_head; // gzip header
    uLongf *size_out;   // bounded size of work buffer
{
    int ret = deflateBoundNoStream(source_len,
            level, window_bits, mem_level, gz_head, size_out);
    if (ret != Z_OK) {
        return ret;
    }
    // if max_block_len < size_out, we may compress to more than one block
    // of size <= max_block_len for data protection
    // add addition bound for the each restart
    // FIXME assert not 0
    int num_extra_blocks = (*size_out / max_block_len) + 1;
    // four extra bytes are stored per block
    *size_out += num_extra_blocks * 4;
    return ret;
}

int ZEXPORT zsc_compress_get_max_output_size2(source_len, max_block_len,
        level, window_bits, mem_level, size_out)
    uLong source_len;    // source buffer length
    uLong max_block_len;  // max length of a deflate block
    int level;          // compression level
    int window_bits;     // window size
    int mem_level;       // memory level
    uLongf *size_out;   // bounded size of work buffer
{
    return zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
            level, window_bits, mem_level, Z_NULL, size_out);
}

int ZEXPORT zsc_compress_get_max_output_size_gzip(
        source_len, max_block_len, level, gz_head, size_out)
    uLong source_len;    // source buffer length
    uLong max_block_len;  // max length of a deflate block
    int level;          // compression level
    gz_headerp gz_head; // gzip header
    uLongf *size_out;   // bounded size of work buffer
{
    return zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
            level, DEF_WBITS + GZIP_CODE, DEF_MEM_LEVEL, gz_head, size_out);
}

int ZEXPORT zsc_compress_get_max_output_size(source_len, max_block_len, level, size_out)
    uLong source_len;    // source buffer length
    uLong max_block_len;  // max length of a deflate block
    int level;          // compression level
    uLongf *size_out;   // bounded size of work buffer
{
    return zsc_compress_get_max_output_size2(source_len, max_block_len,
            level, DEF_WBITS, DEF_MEM_LEVEL, size_out);
}

