/***********************************************************************
 * Copyright 2020, by the California Institute of Technology.
 * ALL RIGHTS RESERVED. United States Government Sponsorship acknowledged.
 * Any commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file        zsc_compress.c
 * @date        2020-07-01
 * @author      Neil Abcouwer
 * @brief       Function definitions for all-in-one compression.
 *
 * Functions do deflateInit(), loops of deflate(), using full flush
 * to create independent blocks, and deflateEnd()
 */

#include "zsc/zsc_pub.h"
#include "zsc/zutil.h"
#include "zsc/zsc_conf_private.h"

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
ZlibReturn zsc_compress_gzip2(
        U8 *dest, U32 *dest_len, const U8 *source, U32 source_len,
        U32 max_block_len, U8 *work, U32 work_len, I32 level,
        I32 window_bits, I32 mem_level, ZlibStrategy strategy,
        gz_header * gz_header)
{
    ZSC_ASSERT(source != Z_NULL);
    ZSC_ASSERT(dest != Z_NULL);
    ZSC_ASSERT(dest_len != Z_NULL);
    ZSC_ASSERT(work != Z_NULL);
    // gz_header can be null

    U32 dest_len_in = *dest_len;
    *dest_len = 0; // nothing yet written to output

    z_stream stream;
    zmemzero((U8*)&stream, sizeof(stream));
    stream.next_work = work;
    stream.avail_work = work_len;
    stream.next_out = dest;
    stream.avail_out = 0;
    stream.next_in = (const U8 *)source;
    stream.avail_in = 0;

    // check if work buffer is large enough
    U32 min_work_buf_size = U32_MAX;
    ZlibReturn err = zsc_compress_get_min_work_buf_size2(window_bits, mem_level,
            &min_work_buf_size);
    if (err != Z_OK) {
        ZSC_WARN1("In zsc_compress_gzip2(), could not get min work buf size, "
                 "error %d.", err);
        return err;
    }
    if (work_len < min_work_buf_size) {
        ZSC_WARN2("In zsc_compress_gzip2(), working memory (%u B) "
                "was smaller than required (%u B).",
                work_len, min_work_buf_size);
        return Z_MEM_ERROR;
    }

    // init the stream
    err = deflateInit2(&stream, level, Z_DEFLATED, window_bits, mem_level,
            strategy);
    if (err != Z_OK) {
        ZSC_WARN1("In zsc_compress_gzip2, could not deflateInit, error %d.", err);
        return err;
    }

    // set gzip header, if provided
    if (gz_header != Z_NULL) {
        err = deflateSetHeader(&stream, gz_header);
        if (err != Z_OK) {
            ZSC_WARN1("In zsc_compress_gzip2(), could not set deflate header, "
                    "error %d.", err);
            return err;
        }
    }

    // check output buffer size, warn if small (but still might succeed)
    U32 bound1 = deflateBound(&stream, source_len);
    U32 bound2 = U32_MAX;
    err = zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
            level, window_bits, mem_level, gz_header, &bound2);
    if(err != Z_OK) {
        ZSC_WARN1("In, zsc_compress_gzip2(), could not get deflate output bound, "
                "error %d.", err);
        return err;
    }
    U32 small_output = (dest_len_in < bound1) || (dest_len_in < bound2);
    // don't warn yet. If there is a failure, and the output was small, then inform

    U32 bytes_left_dest = dest_len_in;

    U32 loops = 0;
    ZSC_ASSERT(max_block_len != 0);
    U32 loop_limit = dest_len_in / max_block_len + source_len / max_block_len + 10;
    while (err == Z_OK && loops < loop_limit) {
        loops++;
        if (stream.avail_out == 0) { // provide more output
            stream.avail_out = ZMIN(bytes_left_dest, max_block_len);
            bytes_left_dest -= stream.avail_out;
        }
        if (stream.avail_in == 0) { // provide more input
            stream.avail_in = ZMIN(source_len, max_block_len);
            source_len -= stream.avail_in;
        }
        ZlibFlush flush = (source_len > 0) ? Z_FULL_FLUSH : Z_FINISH;
        err = deflate(&stream, flush);
    }
    ZSC_ASSERT2(loops < loop_limit, loops, loop_limit);
    *dest_len = stream.total_out;

    if(err != Z_STREAM_END) {
        ZSC_WARN1("In zsc_compress_gzip2(), deflate loop ended "
                "with error code %d.", err);
        if (small_output) {
            ZSC_WARN3("In zsc_compress_gzip2(), output buffer (%u bytes) "
                    "was smaller than bounds (%u/%u bytes). "
                    "Output may not have fit in the buffer.",
                    dest_len_in, bound1, bound2);
        }
        (void)deflateEnd(&stream); // clean up
        return (err == Z_OK) ? Z_STREAM_ERROR : err;
    }

    err = deflateEnd(&stream);
    if (err != Z_OK) {
        ZSC_WARN1("In zsc_compress_gzip2(), deflate ended with error code %d.", err);
    }
    return err;
}

// compress using a work buffer instead of dynamic memory
ZlibReturn zsc_compress2(
        U8 *dest, U32 *dest_len, const U8 *source, U32 source_len,
        U32 max_block_len, U8 *work, U32 work_len, I32 level,
        I32 window_bits, I32 mem_level, ZlibStrategy strategy)
{
    return zsc_compress_gzip2(dest, dest_len, source, source_len, max_block_len,
            work, work_len, level, window_bits, mem_level, strategy, Z_NULL);
}


ZlibReturn zsc_compress_gzip(
        U8 *dest, U32 *dest_len, const U8 *source, U32 source_len,
        U32 max_block_len,  U8 *work, U32 work_len, I32 level,
        gz_header * gz_header)
{
    return zsc_compress_gzip2(dest, dest_len, source, source_len, max_block_len,
            work, work_len, level, DEF_WBITS + GZIP_CODE,
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, gz_header);
}

ZlibReturn zsc_compress(
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

ZlibReturn zsc_compress_get_max_output_size_gzip2(
        U32 source_len, U32 max_block_len, I32 level, I32 window_bits,
        I32 mem_level, gz_header * gz_header, U32 *size_out)
{
    // determine output bound for the give source_len
    U32 intermediate_size = U32_MAX;
    ZlibReturn err = deflateBoundNoStream(source_len,
            level, window_bits, mem_level, gz_header, &intermediate_size);
    if (err != Z_OK) {
        ZSC_WARN1("In zsc_compress_get_max_output_size_gzip2(), "
                "could not get deflate output bound, error %d.", err);
        return err;
    }

    // but we'll be splitting that output into independent blocks
    ZSC_ASSERT(max_block_len != 0);
    I32 num_blocks = (intermediate_size / max_block_len) + 1;

    // four extra bytes are stored per block
    U32 extra_bytes = num_blocks * 4;

    // recalculate output bound
    err = deflateBoundNoStream(source_len + extra_bytes,
            level, window_bits, mem_level, gz_header, size_out);
    if (err != Z_OK) {
        ZSC_WARN1("In zsc_compress_get_max_output_size_gzip2(), "
                "could not recalculate deflate output bound, error %d.", err);
    }
    return err;
}

ZlibReturn zsc_compress_get_max_output_size2(
        U32 source_len, U32 max_block_len, I32 level,
        I32 window_bits, I32 mem_level, U32* size_out)
{
    return zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
            level, window_bits, mem_level, Z_NULL, size_out);
}

ZlibReturn zsc_compress_get_max_output_size_gzip(
        U32 source_len, U32 max_block_len, I32 level,
                gz_header * gz_header, U32* size_out)
{
    return zsc_compress_get_max_output_size_gzip2(source_len, max_block_len,
            level, DEF_WBITS + GZIP_CODE, DEF_MEM_LEVEL, gz_header, size_out);
}

ZlibReturn zsc_compress_get_max_output_size(
        U32 source_len, U32 max_block_len, I32 level, U32 *size_out)
{
    return zsc_compress_get_max_output_size2(source_len, max_block_len,
            level, DEF_WBITS, DEF_MEM_LEVEL, size_out);
}

