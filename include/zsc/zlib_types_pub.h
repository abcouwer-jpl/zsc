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
 * @file        zlib_types_pub.h
 * @date        2020-07-01
 * @author      Jean-loup Gailly, Mark Adler, Neil Abcouwer
 * @brief       Public zlib types.
 *
 * These types, or their analogs, used to reside in zlib.h. Splitting out
 * so users can use, assign, and interpret these types without necessarily
 * including the functions from zlib.h.
 *
 * zlib.h - Copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler
 *
 * Jean-loup Gailly        Mark Adler
 * jloup@gzip.org          madler@alumni.caltech.edu
 *
 *
 * The data format used by the zlib library is described by RFCs (Request for
 * Comments) 1950 to 1952 in the files http://tools.ietf.org/html/rfc1950
 * (zlib format), rfc1951 (deflate format) and rfc1952 (gzip format).
 */

#ifndef ZLIB_TYPES_PUB_H
#define ZLIB_TYPES_PUB_H

#include "zsc/zsc_conf_global_types.h"

// check that conf_global_types defined signed sized types correctly
ZSC_COMPILE_ASSERT(sizeof(U8)  == 1,  U8BadSize);
ZSC_COMPILE_ASSERT(sizeof(U16) == 2, U16BadSize);
ZSC_COMPILE_ASSERT(sizeof(U32) == 4, U32BadSize);
ZSC_COMPILE_ASSERT(sizeof(I32) == 4, I32BadSize);

#define Z_NULL  0  /// for initializing zalloc, zfree, opaque

// macros for users to define sizes of buffers at compile time

/**
 * @brief conservative bound on the size of a compressed output
 *
 * conservative bound on the largest size of a "compressed" output, given an input.
 * Note the return value is larger than the input:
 * input may be incompressible, and the output may be larger.
 * Assumes the largest wrapper, gzip, will be used, but without filling the
 * "name", "comment", or "extra" fields. If those will be filled,
 * the user should add more margin.
 *
 * @param source_len  Size of source buffer
 * @return maximum size of output
 */
#define Z_DEFLATE_OUTPUT_BOUND(source_len) \
    ((source_len) + \
           (((source_len)+7) >> 3) + (((source_len)+63) >> 6) + 5 + \
           18 + 2)

/**
 * @brief conservative bound on the size of a compressed output, given a block length
 *
 * conservative bound on the largest size of a "compressed" output, given an input
 * and the minimum value max_block_len could take.
 * With a smaller the block size, there will be more overhead.
 * Note the return value is larger than the input:
 * input may be incompressible, and the output may be larger.
 * assumes the largest wrapper, gzip, will be used, but without filling the
 * "name", "comment", or "extra" fields. If those will be filled,
 * the user should add more margin.
 *
 * @param source_len  Size of source buffer
 * @param min_max_block_len  Smallest size that the maximum block length could be.
 * @return maximum size of output
 */
#define Z_DEFLATE_OUTPUT_BOUND_BLOCKS(source_len, min_max_block_len) \
    (Z_DEFLATE_OUTPUT_BOUND((source_len)) + \
     (Z_DEFLATE_OUTPUT_BOUND((source_len)) / (min_max_block_len) + 1) * 4)


/// size of the private deflate state, with margin for pointer sizes changing
#define Z_DEFLATE_STATE_SIZE 6400

/**
 * @brief Size of work buffer needed for compression
 *
 * @param window_bits   The base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 * @param mem_level     How much memory to use for internal state.
 *                      Should be in the range 1 to 9.
 */
#define Z_COMPRESS_WORK_SIZE2(window_bits, mem_level) \
    (Z_DEFLATE_STATE_SIZE + \
            (1 << (window_bits)) * 2 * sizeof(U8) + \
            (1 << (window_bits)) * 2 * sizeof(Pos) + \
            (1 << ((mem_level)+7)) * sizeof(Pos) + \
            (1 << ((mem_level)+6)) * (sizeof(U16) +2) )

/// size of the private inflate state, with margin for pointer sizes changing
#define Z_INFLATE_STATE_SIZE 7600

/**
 * @brief Size of work buffer needed for decompression
 *
 * @param window_bits   The base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 */
#define Z_UNCOMPRESS_WORK_SIZE2(window_bits) \
    (Z_INFLATE_STATE_SIZE + (1 << (window_bits)) * sizeof(U8))




/* constants */


/* The memory requirements for deflate are (in bytes):
            (1 << (windowBits+2)) +  (1 << (memLevel+9))
 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
 plus a few kilobytes for small objects. For example, if you want to reduce
 the default memory requirements from 256K to 128K, compile with
     make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
 Of course this will generally degrade compression (there's no free lunch).

   The memory requirements for inflate are (in bytes) 1 << windowBits
 that is, 32K for windowBits=15 (default value) plus about 7 kilobytes
 for small objects.
*/
enum {
    MAX_MEM_LEVEL = 9,  /// Maximum value for memLevel in deflateInit2
    DEF_MEM_LEVEL = 8,  /// Default memory level
    /* Maximum value for windowBits in deflateInit2 and inflateInit2.
     * WARNING: reducing MAX_WBITS makes minigzip unable to extract .gz files
     * created by gzip. (Files created by minigzip can still be extracted by
     * gzip.) */
    MAX_WBITS = 15,             /// Maximum bits: 32K LZ77 window
    DEF_WBITS = MAX_WBITS,      /// Default window bits
};

ZSC_COMPILE_ASSERT(DEF_MEM_LEVEL <= MAX_MEM_LEVEL, bad_def_mem_level);
ZSC_COMPILE_ASSERT(DEF_WBITS <= DEF_WBITS, bad_def_wbits);


enum {
    GZIP_CODE = 0x10  /// adding GZIP_CODE to window bits signifies gzip
};

/** Allowed flush values - see deflate() and inflate() for details */
typedef enum {
    Z_NO_FLUSH = 0,      /** Deflate/inflate: function decides how much data
                               to accumulate before output */
    Z_PARTIAL_FLUSH = 1, /** Deflate: all pending output flushed,
                               not aligned to byte boundary.
                             Inflate: flushes as much output as possible. */
    Z_SYNC_FLUSH = 2,    /** Deflate: all pending output flushed,
                               aligned to byte boundary.
                             Inflate: flushes as much output as possible. */
    Z_FULL_FLUSH = 3,    /** Deflate: all pending output flushed,
                               aligned to byte boundary, deflate state reset
                               so decompression can restart from this point */
    Z_FINISH = 4,        /** Deflate/Inflate: process all pending input,
                               process all pending output flushed,
                               returns with Z_STREAM_END if there was enough
                               output space, otherwise must be given more. */
    Z_BLOCK = 5,         /** Deflate: all pending output flushed,
                               not aligned to byte boundary. Up to seven bits
                               are held to be written to next block.
                             Inflate: requests inflate stop when it gets to
                               next deflate block boundary. */
    Z_TREES = 6,         /** Deflate: Not permitted.
                             Inflate: Behaves as Z_BLOCK does, but also returns when
                             the end of each deflate block header is reached. */
} ZlibFlush;

/** Return codes for the compression/decompression functions. Negative values
 * are errors, positive values are used for special but normal events. */
typedef enum {
    Z_OK = 0,                   /// Some progress has been made.
    Z_STREAM_END = 1,           /** All input consumed and all output produced,
                                      when flush set to Z_FINISH. */
    Z_NEED_DICT = 2,            /// A preset dictionary is needed.
    Z_ERRNO = -1,
    Z_STREAM_ERROR = -2,        /// Stream data or parameter is inconsistent
    Z_DATA_ERROR = -3,          /// Data in compressed buffer can't be handled.
    Z_MEM_ERROR = -4,           /// Not enough memory
    Z_BUF_ERROR = -5,           /// More input or output space needed.
    Z_VERSION_ERROR = -6,       /** Zlib version in incompatible with
                                      the version expected by the caller */
} ZlibReturn;

/** Specific compression levels. Additionally, any integer between
 *  best speed and best compression is an allowed compression level */
enum {
    Z_NO_COMPRESSION = 0,       /// Copy data into ZLIB format
    Z_BEST_SPEED = 1,           /// Compress data as quickly as possible
    Z_BEST_COMPRESSION = 9,     /// Compress data as compactly as possible
    Z_DEFAULT_COMPRESSION = -1, /// Compress data in default speed/compactness
};

/** Compression strategies. see deflateInit2() below for details */
typedef enum {
    Z_FILTERED = 1,         /// Data produced by a filter or predictor
    Z_HUFFMAN_ONLY = 2,     /// Force Huffman encoding only
    Z_RLE = 3,              /// Limit match distances to one (run-length encoding)
    Z_FIXED = 4,            /// Prevent dynamic Huffman codes
    Z_DEFAULT_STRATEGY = 0, /// Normal data
} ZlibStrategy;

/** Possible values of the data_type field for deflate() */
typedef enum {
    Z_BINARY = 0, /// Not a text file
    Z_TEXT = 1,   /// Text file
    Z_ASCII = Z_TEXT, /// for compatibility with 1.2.2 and earlier */
    Z_UNKNOWN = 2, /// Type unknown
} ZlibDataType;

/** The deflate compression method (the only one supported in this version) */
typedef enum {
    Z_DEFLATED = 8, /// Deflate is the only compression method.
} ZlibMethod;

/*

     Abcouwer ZSC - The application must pass a buffer to next_work (and its size to
   avail_work) before an init function is called.

     The application must update next_in and avail_in when avail_in has dropped
   to zero.  It must update next_out and avail_out when avail_out has dropped
   to zero.

     Abcouwer ZSC - The z_stream originally had function pointers for allocation
   functions. These have been replaced with pointer to and size of a work buffer.

     The fields total_in and total_out can be used for statistics or progress
   reports.  After compression, total_in holds the total size of the
   uncompressed data and may be saved for use by the decompressor (particularly
   if the decompressor wants to decompress everything in a single step).
*/

struct internal_state;

typedef struct z_stream_s {
    const U8* next_in;     /* next input byte */
    U32     avail_in;  /* number of bytes available at next_in */
    U32    total_in;  /* total number of input bytes read so far */

    U8*    next_out; /* next output byte will go here */
    U32     avail_out; /* remaining free space at next_out */
    U32    total_out; /* total number of bytes output so far */

    // Abcouwer ZSC - removed allocation functions in favor of work buffer
    // must be initialized before call to Init()
    U8*   next_work; /* next free space in the work buffer */
    U32   avail_work; /* number of bytes available at next_work */

    const U8* msg;  /* last error message, NULL if no error */
    struct internal_state *state; /* not visible by applications */

    ZlibDataType     data_type;  /* best guess about the data type: binary or text
                           for deflate, or the decoding state for inflate */
    U32   adler;      /* Adler-32 or CRC-32 value of the uncompressed data */
    U32   reserved;   /* reserved for future use */
} z_stream;

/*
     gzip header information passed to and from zlib routines.  See RFC 1952
  for more details on the meanings of these fields.
*/
typedef struct gz_header_s {
    I32 text;       /* true if compressed data believed to be text */
    U32 time;       /* modification time */
    I32 xflags;     /* extra flags (not used when writing a gzip file) */
    I32 os;         /* operating system */
    U8* extra;      /* pointer to extra field or Z_NULL if none */
    U32 extra_len;  /* extra field length (valid if extra != Z_NULL) */
    U32 extra_max;  /* space at extra (only when reading header) */
    U8* name;       /* pointer to zero-terminated file name or Z_NULL */
    U32 name_max;   /* space at name (only when reading header) */
    U8* comment;    /* pointer to zero-terminated comment or Z_NULL */
    U32 comm_max;   /* space at comment (only when reading header) */
    I32 hcrc;       /* true if there was or will be a header crc */
    I32 done;       /* true when done reading gzip header (not used
                           when writing a gzip file) */
} gz_header;

#endif
