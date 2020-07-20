/* zutil.c -- target dependent utility functions for the compression library
 * Copyright (C) 1995-2017 Jean-loup Gailly
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#include "zutil.h"
// Abcouwer ZSC - removed gz files


const U8 * const z_errmsg[10] = {
    (const U8 *)"need dictionary",     /* Z_NEED_DICT       2  */
    (const U8 *)"stream end",          /* Z_STREAM_END      1  */
    (const U8 *)"",                    /* Z_OK              0  */
    (const U8 *)"file error",          /* Z_ERRNO         (-1) */
    (const U8 *)"stream error",        /* Z_STREAM_ERROR  (-2) */
    (const U8 *)"data error",          /* Z_DATA_ERROR    (-3) */
    (const U8 *)"insufficient memory", /* Z_MEM_ERROR     (-4) */
    (const U8 *)"buffer error",        /* Z_BUF_ERROR     (-5) */
    (const U8 *)"incompatible version",/* Z_VERSION_ERROR (-6) */
    (const U8 *)""
};


const U8 * ZEXPORT zlibVersion()
{
    return (U8*)ZLIB_VERSION;
}

/*
Abcouwer ZSC - As it is impossible to test the 2^N variations of zlib that
could exist with conditional compilation, most of these variations have been removed.
Unless noted elsewhere, the code has been changed to match the compilation
in the case where these flags are not defined.

If your favorite option has been removed, but the option is compatible with
the goals of ZSC, consider a pull request! But implement that option is such a way
that the source code compiles the same, the option is a run-time option.
That allows unit testing of all the options.

*/
U32 ZEXPORT zlibCompileFlags()
{
    U32 flags;

    flags = 0;
    ZSC_COMPILE_ASSERT((int)(sizeof(U32)) == 4, bad_u32_1);
    switch ((int)(sizeof(U32))) {
    case 2:     break;
    case 4:     flags += 1;     break;
    case 8:     flags += 2;     break;
    default:    flags += 3;
    }
    ZSC_COMPILE_ASSERT((int)(sizeof(U32)) == 4, bad_u32_2);
    switch ((int)(sizeof(U32))) {
    case 2:     break;
    case 4:     flags += 1 << 2;        break;
    case 8:     flags += 2 << 2;        break;
    default:    flags += 3 << 2;
    }
    switch ((int)(sizeof(void*))) {
    case 2:     break;
    case 4:     flags += 1 << 4;        break;
    case 8:     flags += 2 << 4;        break;
    default:    flags += 3 << 4;
    }
    switch ((int)(sizeof(z_off_t))) {
    case 2:     break;
    case 4:     flags += 1 << 6;        break;
    case 8:     flags += 2 << 6;        break;
    default:    flags += 3 << 6;
    }
#ifdef ZLIB_DEBUG
    flags += 1 << 8;
#endif

    /* Abcouwer ZSC - removed conditional compilation flags:
     * ASMV/ASMINF, ZLIB_WINAPI, BUILDFIXED, DYNAMIC_CRC_TABLE, NO_GZCOMPRESS,
     * PKZIP_BUG_WORKAROUND, NO_GZIP, FASTEST     */

    /* Abcouwer ZSC - sprintf variants not checked, as used in gzprintf,
     * and gz code not included */

    return flags;
}

#ifdef ZLIB_DEBUG
#include <stdlib.h>
#  ifndef verbose
#    define verbose 0
#  endif
int ZLIB_INTERNAL z_verbose = verbose;

void ZLIB_INTERNAL z_error (m)
    U8 *m;
{
    fprintf(stderr, "%s\n", m);
    exit(1);
}
#endif

/* exported to allow conversion of error code to string for compress() and
 * uncompress()
 */
const U8 * ZEXPORT zError(err)
    ZlibReturn err;
{
    return ERR_MSG(err);
}


// Abcouwer ZSC - removed solo memcpy and related functions

// Abcouwer ZSC - removed dynamic memory allocation functions
