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
 * @brief       Function definitions for safety-critical decompressing.
 */

#include "zsc_pub.h"
#include "zutil.h"


// FIXME REMOVE
#include <stdio.h>

#define GZIP_CODE (16)

ZlibReturn ZEXPORT zsc_uncompress_get_min_work_buf_size2(
        I32 window_bits, U32 *size_out)
{
    return inflateWorkSize2(window_bits, size_out);
}

// get the bounded size of the work buffer
ZlibReturn ZEXPORT zsc_uncompress_get_min_work_buf_size(U32 *size_out)
{
    return inflateWorkSize(size_out);
}

ZlibReturn ZEXPORT zsc_uncompress_safe_gzip2(
        U8 *dest, U32 *dest_len, const U8 *source, U32 *source_len,
        U8 *work, U32 work_len, I32 window_bits, gz_header * gz_head)
{
    // check if workbuffer is large enough
    U32 min_work_buf_size = U32_MAX;

    ZlibReturn err =
            zsc_uncompress_get_min_work_buf_size2(window_bits, &min_work_buf_size);
    if (err != Z_OK) {
        return err;
    }
    if (work_len < min_work_buf_size) {
        // work buffer is smaller than bound
        // FIXME warn
        printf("work buffer is smaller than bound.\n");
        return Z_MEM_ERROR;
        // FIXME could adjust windowBits until it works?
    }

    z_stream stream;
    zmemzero((Byte*)&stream, sizeof(stream));
    stream.next_work = work;
    stream.avail_work = work_len;

    err = inflateInit2(&stream, window_bits);
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

    stream.next_in = (const Byte *)source;
    stream.avail_in = *source_len;
    stream.next_out = dest;
    stream.avail_out = *dest_len;

    I32 got_data_err = 0;
    I32 cycles = 0;
    // FIXME loop limit
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
            printf(" after sync: avail_in=%u, out=%u, err = %d\n",
                    stream.avail_in, stream.avail_out, err);

            if (err == Z_OK) {
                printf("found new flush point\n");
            } else {

                printf("inflateSync() returned %d, "
                        "could not find a new flush point.\n", err);
            }
        }
        cycles++;
    } while (err == Z_OK);

    *dest_len = stream.total_out;
    *source_len = stream.total_in;

    if(err != Z_STREAM_END) {
        printf("inflate failed\n");

        // when uncompressing with inflate(Z_FINISH),
        // Z_STREAM_END is expected, not Z_OK
        // Z_OK may indicate there wasn't enough output space
        (void)inflateEnd(&stream); // clean up
        return (err == Z_OK) ? Z_STREAM_ERROR : err; // FIXME stream error?
    }

    err = inflateEnd(&stream);
    // FIXME warn if bad

    if (err == Z_OK && got_data_err) {
        err = Z_DATA_ERROR;
    }

    return err;
}

ZlibReturn ZEXPORT zsc_uncompress_safe2(
        U8 *dest, U32 *dest_len, const U8 *source, U32 *source_len,
        U8 *work, U32 work_len, I32 window_bits)
{
    return zsc_uncompress_safe_gzip2(dest, dest_len, source, source_len,
            work, work_len, window_bits, Z_NULL);
}

ZlibReturn ZEXPORT zsc_uncompress(
        U8 *dest, U32 *dest_len, const U8 *source,
        U32 *source_len, U8 *work, U32 work_len)
{
    return zsc_uncompress_safe2(dest, dest_len, source, source_len,
            work, work_len, DEF_WBITS);
}

ZlibReturn ZEXPORT zsc_uncompress_safe_gzip(
        U8 *dest, U32 *dest_len, const U8 *source, U32 *source_len,
        U8 *work, U32 work_len, gz_header * gz_head)
{
    return zsc_uncompress_safe_gzip2(dest, dest_len, source, source_len,
            work, work_len, DEF_WBITS + GZIP_CODE, gz_head);
}


