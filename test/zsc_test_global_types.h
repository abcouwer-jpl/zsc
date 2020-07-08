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
 * @file        zsc_test_global_types.h
 * @date        2020-07-01
 * @author      Neil Abcouwer
 * @brief       Definition of global types for testing demosaic
 *
 * zsc_conf_types_pub.h defines public types used by public demosaic functions.
 * and follows the common guideline, as expressed in MISRA
 * directive 4.6, "typedefs that indicate size and signedness should be used
 * in place of the basic numerical types".
 *
 * zsc_types_pub.h includes zsc_conf_global_types.h, which must define
 * these types.  For the purposes of platform-independent unit testing,
 * test/demosaic_test_global_types.h is copied to
 * include/demosaic/demosaic_conf_global_types.h.
 *
 * Users must define a configuration dependent header for their purposes.
 *
 * Sources of sized types:
 *      cstd: stdint.h
 *      NASA core fsw: osal -> common_types.h
 *      VXWorks: inttypes.h
 *
 * ZSC also needs a NULL.
 */

#ifndef ZSC_CONF_GLOBAL_TYPES_H
#define ZSC_CONF_GLOBAL_TYPES_H

#include <stdint.h>

// FIXME delete when removed
#define Z_SOLO
#ifdef __STDC_VERSION__
#  ifndef STDC
#    define STDC
#  endif
#  if __STDC_VERSION__ >= 199901L
#    ifndef STDC99
#      define STDC99
#    endif
#  endif
#endif
#if !defined(STDC) && (defined(__STDC__) || defined(__cplusplus))
#  define STDC
#endif
#define OF(args)  args
#define Z_ARG(args)  args
#define ZEXPORT
#define ZEXTERN extern
#define ZEXPORTVA
#define FAR
#define Z_U4 unsigned
#define z_const const

#define z_off_t long

#define z_off64_t I64



// throw a compilation error if test is not true
#define ZSC_COMPILE_ASSERT(test, msg) \
  typedef U8 (msg)[ ((test) ? 1 : -1) ]

#ifndef NULL
#define NULL  (0)
#endif


typedef int16_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;

ZSC_COMPILE_ASSERT(sizeof(I16) == 2, I16BadSize);
ZSC_COMPILE_ASSERT(sizeof(I32) == 4, I32BadSize);
ZSC_COMPILE_ASSERT(sizeof(U8)  == 1,  U8BadSize);
ZSC_COMPILE_ASSERT(sizeof(U16) == 2, U16BadSize);
ZSC_COMPILE_ASSERT(sizeof(U32) == 4, U32BadSize);

// FIXME delete

typedef U8  Byte;  /* 8 bits */
typedef U32   uInt;  /* 16 bits or more */
typedef U32  uLong; /* 32 bits or more */


typedef U32 z_crc_t;
typedef unsigned long z_size_t;
typedef long ptrdiff_t;  /* guess -- will be caught if guess is wrong */



#endif /* ZSC_CONF_GLOBAL_TYPES_H */
