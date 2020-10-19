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
 * @file        zutil.h
 * @date        2020-08-05
 * @author      Jean-loup Gailly, Mark Adler, Neil Abcouwer
 * @brief       Internal macros and constants
 *
 * Stripped-down version of zutil.h for safety-critical applications.
 * Original file header follows.
 */

/* zutil.h -- internal interface and configuration of the compression library
 * Copyright (C) 1995-2016 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* @(#) $Id$ */

#ifndef ZUTIL_H
#define ZUTIL_H

// Abcouwer ZSC - remove ZLIB_INTERNAL

#include "zsc/zlib.h"

// Abcouwer ZSC - typedef ptrdiff_t moved to zsc_conf_global_types

// Abcouwer ZSC - removed definition of "local" as "static"
//                in favor of ZSC_PRIVATE in configuration header.

extern const U8 * const z_errmsg[10]; /* indexed by 2-zlib_error */
/* (size given to avoid silly warnings with Visual C++) */

#define ERR_MSG(err) z_errmsg[Z_NEED_DICT-(err)]

#define ERR_RETURN(strm,err) \
    (strm)->msg = ERR_MSG(err); \
    return (err)


/* To be used only when the state is known to be valid */

        /* common constants */

// Abcouwer ZSC - moved DEF_WBITS and DEF_MEM_LEVEL to zlib_types_pub.h

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define PRESET_DICT 0x20 /* preset dictionary flag in zlib header */

// define os code

#if defined(MSDOS) || (defined(WINDOWS) && !defined(WIN32))
#  define OS_CODE  0x00
#endif
#ifdef AMIGA
#  define OS_CODE  1
#endif
#if defined(VAXC) || defined(VMS)
#  define OS_CODE  2
#endif

#ifdef __370__
#  if __TARGET_LIB__ < 0x20000000
#    define OS_CODE 4
#  elif __TARGET_LIB__ < 0x40000000
#    define OS_CODE 11
#  else
#    define OS_CODE 8
#  endif
#endif

#if defined(ATARI) || defined(atarist)
#  define OS_CODE  5
#endif

#ifdef OS2
#  define OS_CODE  6
#endif

#if defined(MACOS) || defined(TARGET_OS_MAC)
#  define OS_CODE  7
#endif

#ifdef __acorn
#  define OS_CODE 13
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
#  define OS_CODE  10
#endif

#ifdef _BEOS_
#  define OS_CODE  16
#endif

#ifdef __TOS_OS400__
#  define OS_CODE 18
#endif

#ifdef __APPLE__
#  define OS_CODE 19
#endif

#ifndef OS_CODE
#  define OS_CODE  3     /* assume Unix */
#endif

// Abcouwer ZSC - Remove target dependencies related to fdopen and dynamic memory
// File handling is beyond scope of ZSC.

// Abcouwer ZSC - Remove crc combine functions
// Joining two compressed buffers is beyond scope of ZSC.

// Abcouwer ZSC - removed trace functions

// Abcouwer ZSC - removed dynamic memory functions

/* Reverse the bytes in a 32-bit value */
#define ZSWAP32(q) ((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
                    (((q) & 0xff00) << 8) + (((q) & 0xff) << 24))

// Abcouwer ZSC - provide min/max macros
#define ZMIN(a,b) ((a)<(b) ?  (a) : (b))
#define ZMAX(a,b) ((a)>(b) ?  (a) : (b))

#endif /* ZUTIL_H */
