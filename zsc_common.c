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
 * @file        zsc_common.c
 * @date        2020-07-01
 * @author      Neil Abcouwer
 * @brief       Function definitions zlib safety-critical.
 */

#include "zsc_pub.h"
#include "zutil.h"

#include <stdio.h>

// allocate from the work buffer
voidpf z_static_alloc(voidpf opaque, uInt items, uInt size)
{
    voidpf new_ptr = Z_NULL;
    z_static_mem* mem = (z_static_mem*) opaque;
    // FIXME assert not null
    uLong bytes = items * size;
    // FIXME assert mult didn't overflow

    // check there's enough space
    if (mem->work_alloced + bytes > mem->work_len) {
        // FIXME warn?
        return Z_NULL;
    }

    new_ptr = (voidpf) (mem->work + mem->work_alloced);
    mem->work_alloced += bytes;
    return new_ptr;
}

void z_static_free(void* opaque, void* addr)
{
    // do nothing but make compiler happy
    (void) opaque;
    (void) addr;
}


