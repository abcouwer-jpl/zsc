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
#include "zutil.h"
#include "zsc_conf_private.h"


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
ZlibReturn ZEXPORT zsc_compress_gzip2(
        U8 *dest, U32 *dest_len, const U8 *source, U32 source_len,
        U32 max_block_len, U8 *work, U32 work_len, I32 level,
        I32 window_bits, I32 mem_level, ZlibStrategy strategy,
        gz_header * gz_header)
{
        printf("zsc_compress_gzip2 source_len = %u.\n",
                source_len);

    // check if work buffer is large enough
    U32 min_work_buf_size = U32_MAX;
    ZlibReturn err = zsc_compress_get_min_work_buf_size2(window_bits, mem_level,
            &min_work_buf_size);
    if (err != Z_OK) {
        ZSC_WARN1("Could not get min work buf size, error %d.", err);
        return err;
    }
    if (work_len < min_work_buf_size) {
        ZSC_WARN2("Working memory (%u bytes) was smaller than required (%u bytes).",
                work_len, min_work_buf_size);
        return Z_MEM_ERROR;
    }

    z_stream stream;
    zmemzero((U8*)&stream, sizeof(stream));
    stream.next_work = work;
    stream.avail_work = work_len;

    // see zlib.h for description of parameters
    err = deflateInit2(&stream, level, Z_DEFLATED, window_bits, mem_level,
            strategy);
    if (err != Z_OK) {
        ZSC_WARN1("Could not deflate init, error %d.", err);
        return err;
    }

    // set gzip header, if not null
    if (gz_header != Z_NULL) {
        err = deflateSetHeader(&stream, gz_header);
        if (err != Z_OK) {
            ZSC_WARN1("Could not set deflate header, error %d.", err);
            return err;
        }
    }

    // check output buffer size, warn if small (but still might succeed)
    U32 bound1 = deflateBound(&stream, source_len);
    U32 bound2 = U32_MAX;
    err = zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
            level, window_bits, mem_level, gz_header, &bound2);
    if(err != Z_OK) {
        ZSC_WARN1("Could not get deflate output bound, error %d.", err);
        return err;
    }
    if((*dest_len < bound1) || (*dest_len < bound2)) {
        ZSC_WARN3("Output buffer (%u bytes) was smaller than bounds (%u/%u bytes)."
            "Output may not fit in the buffer.",
            *dest_len, bound1, bound2);
    }

    stream.next_out = dest;
    stream.avail_out = 0;
    stream.next_in = (const U8 *)source;
    stream.avail_in = 0;

    U32 bytes_left_dest = *dest_len;

    I32 cycles = 0;
    // FIXME add loop limit
    do {
        if (stream.avail_out == 0) { // provide more output
            stream.avail_out =
                    bytes_left_dest < max_block_len ?
                            bytes_left_dest : max_block_len;
            bytes_left_dest -= stream.avail_out;
        }
        if (stream.avail_in == 0) { // provide more input
            stream.avail_in =
                    source_len <  max_block_len ?
                             source_len : max_block_len;
            source_len -= stream.avail_in;
        }
        ZlibFlush flush = (source_len > 0) ? Z_FULL_FLUSH : Z_FINISH;
        err = deflate(&stream, flush);
        cycles++;
    } while (err == Z_OK);
    *dest_len = stream.total_out;
    printf("Compressed to %u bytes, %d cycles.\n",
            stream.total_out, cycles);

    if(err != Z_STREAM_END) {
        ZSC_WARN1("Deflate loop ended with unexpected error code %d.", err);
        (void)deflateEnd(&stream); // clean up
        return (err == Z_OK) ? Z_STREAM_ERROR : err;
    }

    err = deflateEnd(&stream);
    if (err != Z_OK) {
        ZSC_WARN1("Deflate ended with error code %d.", err);
    }
    return err;
}

// compress using a work buffer instead of dynamic memory
ZlibReturn ZEXPORT zsc_compress2(
        U8 *dest, U32 *dest_len, const U8 *source, U32 source_len,
        U32 max_block_len, U8 *work, U32 work_len, I32 level,
        I32 window_bits, I32 mem_level, ZlibStrategy strategy)
{
    return zsc_compress_gzip2(dest, dest_len, source, source_len, max_block_len,
            work, work_len, level, window_bits, mem_level, strategy, Z_NULL);
}


ZlibReturn ZEXPORT zsc_compress_gzip(
        U8 *dest, U32 *dest_len, const U8 *source, U32 source_len,
        U32 max_block_len,  U8 *work, U32 work_len, I32 level,
        gz_header * gz_header)
{
    return zsc_compress_gzip2(dest, dest_len, source, source_len, max_block_len,
            work, work_len, level, DEF_WBITS + GZIP_CODE,
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, gz_header);
}

ZlibReturn ZEXPORT zsc_compress(
        U8* dest, U32 *dest_len, const U8 *source, U32 source_len,
        U32 max_block_len, U8 *work, U32 work_len, I32 level)
{
    return zsc_compress2(dest, dest_len, source, source_len, max_block_len,
            work, work_len, level, DEF_WBITS,
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
}

// given window_bits and mem_level,
// calculate the minimum size of the work buffer required for deflation
// return as output param
// return error if any
ZlibReturn zsc_compress_get_min_work_buf_size2(I32 window_bits,
        I32 mem_level, U32 *size_out)
{
    return deflateWorkSize2(window_bits, mem_level, size_out);
}

ZlibReturn zsc_compress_get_min_work_buf_size(U32* size_out)
{
    return deflateWorkSize(size_out);
}

ZlibReturn ZEXPORT zsc_compress_get_max_output_size_gzip2(
        U32 source_len, U32 max_block_len, I32 level, I32 window_bits,
        I32 mem_level, gz_header * gz_header, U32 *size_out)
{
    // determine output bound for the give source_len
    U32 intermediate_size = U32_MAX;
    ZlibReturn err = deflateBoundNoStream(source_len,
            level, window_bits, mem_level, gz_header, &intermediate_size);
    if (err != Z_OK) {
        ZSC_WARN1("Could not get deflate output bound, error %d.", err);
        return err;
    }

    // but we'll be splitting that output into independent blocks
    // FIXME assert not 0
    I32 num_blocks = (intermediate_size / max_block_len) + 1;

    // four extra bytes are stored per block
    U32 extra_bytes = num_blocks * 4;

    // recalculate output bound
    err = deflateBoundNoStream(source_len + extra_bytes,
            level, window_bits, mem_level, gz_header, size_out);
    if (err != Z_OK) {
        ZSC_WARN1("Could not get deflate output bound, error %d.", err);
    }
    return err;
}

ZlibReturn ZEXPORT zsc_compress_get_max_output_size2(
        U32 source_len, U32 max_block_len, I32 level,
        I32 window_bits, I32 mem_level, U32* size_out)
{
    return zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
            level, window_bits, mem_level, Z_NULL, size_out);
}

ZlibReturn ZEXPORT zsc_compress_get_max_output_size_gzip(
        U32 source_len, U32 max_block_len, I32 level,
                gz_header * gz_header, U32* size_out)
{
    return zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
            level, DEF_WBITS + GZIP_CODE, DEF_MEM_LEVEL, gz_header, size_out);
}

ZlibReturn ZEXPORT zsc_compress_get_max_output_size(
        U32 source_len, U32 max_block_len, I32 level, U32 *size_out)
{
    return zsc_compress_get_max_output_size2(source_len, max_block_len,
            level, DEF_WBITS, DEF_MEM_LEVEL, size_out);
}

