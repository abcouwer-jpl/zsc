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
