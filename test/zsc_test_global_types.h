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
#include <stddef.h>

// throw a compilation error if test is not true
#define ZSC_COMPILE_ASSERT(test, msg) \
  typedef U8 (msg)[ ((test) ? 1 : -1) ]

// signed, sized types
typedef  uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef  int32_t I32;

#define U32_MAX ((U32)0xFFFFFFFF)

// CRCs are 32 bits
typedef U32 z_crc_t;
// z_size_t must be unsigned, big enough to contain the size of the largest size
// that the system can handle
typedef size_t z_size_t;
// ptrdiff_t must be defined,
// can include stddef, or use signed version of z_size_ts

#endif /* ZSC_CONF_GLOBAL_TYPES_H */
