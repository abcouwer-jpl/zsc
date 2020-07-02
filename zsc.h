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
 * @file        zsc.h
 * @date        2020-07-01
 * @author      Neil Abcouwer
 * @brief       Private function declarations for using zlib in a safety critical way.
 */

#ifndef ZSC_H
#define ZSC_H

#include "zconf.h"
#include "zlib.h"

#ifdef __cplusplus
extern "C" {
#endif

ZEXTERN voidpf ZEXPORT z_static_alloc OF((voidpf opaque, uInt items, uInt size));
ZEXTERN void ZEXPORT z_static_free OF((void* opaque, void* addr));

#ifdef __cplusplus
}
#endif

#endif /* ZSC_H */
