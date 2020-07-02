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
 * @file        zlib_types_pub.h
 * @date        2020-07-01
 * @author      Neil Abcouwer (based on Gailly and Adler)
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

#include "zconf.h"

// FIXME Doxygen all types

typedef voidpf (*alloc_func) OF((voidpf opaque, uInt items, uInt size));
typedef void   (*free_func)  OF((voidpf opaque, voidpf address));

struct internal_state;

typedef struct z_stream_s {
    z_const Bytef *next_in;     /* next input byte */
    uInt     avail_in;  /* number of bytes available at next_in */
    uLong    total_in;  /* total number of input bytes read so far */

    Bytef    *next_out; /* next output byte will go here */
    uInt     avail_out; /* remaining free space at next_out */
    uLong    total_out; /* total number of bytes output so far */

    Bytef   *next_work; /* next free space in the work buffer */
    uLong   avail_work; /* number of bytes available at next_work */

    z_const char *msg;  /* last error message, NULL if no error */
    struct internal_state FAR *state; /* not visible by applications */

    int     data_type;  /* best guess about the data type: binary or text
                           for deflate, or the decoding state for inflate */
    uLong   adler;      /* Adler-32 or CRC-32 value of the uncompressed data */
    uLong   reserved;   /* reserved for future use */
} z_stream;

typedef z_stream FAR *z_streamp;

/*
     gzip header information passed to and from zlib routines.  See RFC 1952
  for more details on the meanings of these fields.
*/
typedef struct gz_header_s {
    int     text;       /* true if compressed data believed to be text */
    uLong   time;       /* modification time */
    int     xflags;     /* extra flags (not used when writing a gzip file) */
    int     os;         /* operating system */
    Bytef   *extra;     /* pointer to extra field or Z_NULL if none */
    uInt    extra_len;  /* extra field length (valid if extra != Z_NULL) */
    uInt    extra_max;  /* space at extra (only when reading header) */
    Bytef   *name;      /* pointer to zero-terminated file name or Z_NULL */
    uInt    name_max;   /* space at name (only when reading header) */
    Bytef   *comment;   /* pointer to zero-terminated comment or Z_NULL */
    uInt    comm_max;   /* space at comment (only when reading header) */
    int     hcrc;       /* true if there was or will be a header crc */
    int     done;       /* true when done reading gzip header (not used
                           when writing a gzip file) */
} gz_header;

typedef gz_header FAR *gz_headerp;

/*
     The application must update next_in and avail_in when avail_in has dropped
   to zero.  It must update next_out and avail_out when avail_out has dropped
   to zero.  The application must initialize zalloc, zfree and opaque before
   calling the init function.  All other fields are set by the compression
   library and must not be updated by the application.

     The opaque value provided by the application will be passed as the first
   parameter for calls of zalloc and zfree.  This can be useful for custom
   memory management.  The compression library attaches no meaning to the
   opaque value.

     zalloc must return Z_NULL if there is not enough memory for the object.
   If zlib is used in a multi-threaded application, zalloc and zfree must be
   thread safe.  In that case, zlib is thread-safe.  When zalloc and zfree are
   Z_NULL on entry to the initialization function, they are set to internal
   routines that use the standard library functions malloc() and free().

     On 16-bit systems, the functions zalloc and zfree must be able to allocate
   exactly 65536 bytes, but will not be required to allocate more than this if
   the symbol MAXSEG_64K is defined (see zconf.h).  WARNING: On MSDOS, pointers
   returned by zalloc for objects of exactly 65536 bytes *must* have their
   offset normalized to zero.  The default allocation function provided by this
   library ensures this (see zutil.c).  To reduce memory requirements and avoid
   any allocation of 64K objects, at the expense of compression ratio, compile
   the library with -DMAX_WBITS=14 (see zconf.h).

     The fields total_in and total_out can be used for statistics or progress
   reports.  After compression, total_in holds the total size of the
   uncompressed data and may be saved for use by the decompressor (particularly
   if the decompressor wants to decompress everything in a single step).
*/

                        /* constants */

/** Allowed flush values; see deflate() and inflate() below for details */
typedef enum {
    Z_NO_FLUSH = 0,
    Z_PARTIAL_FLUSH = 1,
    Z_SYNC_FLUSH = 2,
    Z_FULL_FLUSH = 3,
    Z_FINISH = 4,
    Z_BLOCK = 5,
    Z_TREES = 6,
} ZlibFlush;

/** Return codes for the compression/decompression functions. Negative values
 * are errors, positive values are used for special but normal events. */
typedef enum {
    Z_OK = 0,
    Z_STREAM_END = 1,
    Z_NEED_DICT = 2,
    Z_ERRNO = -1,
    Z_STREAM_ERROR= -2,
    Z_DATA_ERROR = -3,
    Z_MEM_ERROR = -4,
    Z_BUF_ERROR = -5,
    Z_VERSION_ERROR = -6,
} ZlibReturn;

/** Specific compression levels. Additionally, any integer between
 *  best speed and best compression is an allowed compression level */
enum {
    Z_NO_COMPRESSION = 0,
    Z_BEST_SPEED = 1,
    Z_BEST_COMPRESSION = 9,
    Z_DEFAULT_COMPRESSION = -1,
};

/** Compression strategies. see deflateInit2() below for details */
typedef enum {
    Z_FILTERED = 1,
    Z_HUFFMAN_ONLY = 2,
    Z_RLE = 3,
    Z_FIXED = 4,
    Z_DEFAULT_STRATEGY = 0,
} ZlibStrategy;

/** Possible values of the data_type field for deflate() */
typedef enum {
    Z_BINARY = 0,
    Z_TEXT = 1,
    Z_ASCII = Z_TEXT, /* for compatibility with 1.2.2 and earlier */
    Z_UNKNOWN = 2,
} ZlibDataType;

/** The deflate compression method (the only one supported in this version) */
typedef enum {
    Z_DEFLATED = 8,
} ZlibMethod;


#define Z_NULL  0  /* for initializing zalloc, zfree, opaque */

#endif
