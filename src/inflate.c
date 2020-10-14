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
 * @file        inflate.c
 * @date        2020-08-05
 * @author      Mark Adler, Neil Abcouwer
 * @brief       Zlib decompression
 *
 * Modified version of inflate.c for safety-critical applications.
 * Modifications:
 *   * Provides functions to size an appropriate work buffer.
 *   * Allocates memory for inflate buffers from work buffer.
 *   * Other modifications based on MISRA, P10 safety guidelines.
 * Original file header follows.
 */

/* inflate.c -- zlib decompression
 * Copyright (C) 1995-2016 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/*
 * Change history:
 *
 * 1.2.beta0    24 Nov 2002
 * - First version -- complete rewrite of inflate to simplify code, avoid
 *   creation of window when not needed, minimize use of window when it is
 *   needed, make inffast.c even faster, implement gzip decoding, and to
 *   improve code readability and style over the previous zlib inflate code
 *
 * 1.2.beta1    25 Nov 2002
 * - Use pointers for available input and output checking in inffast.c
 * - Remove input and output counters in inffast.c
 * - Change inffast.c entry and loop from avail_in >= 7 to >= 6
 * - Remove unnecessary second byte pull from length extra in inffast.c
 * - Unroll direct copy to three copies per loop in inffast.c
 *
 * 1.2.beta2    4 Dec 2002
 * - Change external routine names to reduce potential conflicts
 * - Correct filename to inffixed.h for fixed tables in inflate.c
 * - Make hbuf[] unsigned char to match parameter type in inflate.c
 * - Change strm->next_out[-state->offset] to *(strm->next_out - state->offset)
 *   to avoid negation problem on Alphas (64 bit) in inflate.c
 *
 * 1.2.beta3    22 Dec 2002
 * - Add comments on state->bits assertion in inffast.c
 * - Add comments on op field in inftrees.h
 * - Fix bug in reuse of allocated window after inflateReset()
 * - Remove bit fields--back to byte structure for speed
 * - Remove distance extra == 0 check in inflate_fast()--only helps for lengths
 * - Change post-increments to pre-increments in inflate_fast(), PPC biased?
 * - Add compile time option, POSTINC, to use post-increments instead (Intel?)
 * - Make MATCH copy in inflate() much faster for when inflate_fast() not used
 * - Use local copies of stream next and avail values, as well as local bit
 *   buffer and bit count in inflate()--for speed when inflate_fast() not used
 *
 * 1.2.beta4    1 Jan 2003
 * - Split ptr - 257 statements in inflate_table() to avoid compiler warnings
 * - Move a comment on output buffer sizes from inffast.c to inflate.c
 * - Add comments in inffast.c to introduce the inflate_fast() routine
 * - Rearrange window copies in inflate_fast() for speed and simplification
 * - Unroll last copy for window match in inflate_fast()
 * - Use local copies of window variables in inflate_fast() for speed
 * - Pull out common wnext == 0 case for speed in inflate_fast()
 * - Make op and len in inflate_fast() unsigned for consistency
 * - Add FAR to lcode and dcode declarations in inflate_fast()
 * - Simplified bad distance check in inflate_fast()
 * - Added inflateBackInit(), inflateBack(), and inflateBackEnd() in new
 *   source file infback.c to provide a call-back interface to inflate for
 *   programs like gzip and unzip -- uses window as output buffer to avoid
 *   window copying
 *
 * 1.2.beta5    1 Jan 2003
 * - Improved inflateBack() interface to allow the caller to provide initial
 *   input in strm.
 * - Fixed stored blocks bug in inflateBack()
 *
 * 1.2.beta6    4 Jan 2003
 * - Added comments in inffast.c on effectiveness of POSTINC
 * - Typecasting all around to reduce compiler warnings
 * - Changed loops from while (1) or do {} while (1) to for (;;), again to
 *   make compilers happy
 * - Changed type of window in inflateBackInit() to unsigned char *
 *
 * 1.2.beta7    27 Jan 2003
 * - Changed many types to unsigned or unsigned short to avoid warnings
 * - Added inflateCopy() function
 *
 * 1.2.0        9 Mar 2003
 * - Changed inflateBack() interface to provide separate opaque descriptors
 *   for the in() and out() functions
 * - Changed inflateBack() argument and in_func typedef to swap the length
 *   and buffer address return values for the input function
 * - Check next_in and next_out for Z_NULL on entry to inflate()
 *
 * The history for versions after 1.2.0 are in ChangeLog in zlib distribution.
 */

#include "zsc/zutil.h"
#include "zsc/zsc_conf_private.h"
#include "zsc/inftrees.h"
#include "zsc/inflate.h"
#include "zsc/inffast.h"

// ABcouwer ZSC - declare the iffixed tables in c file

    static const code lenfix[512] = {
        {96,7,0},{0,8,80},{0,8,16},{20,8,115},{18,7,31},{0,8,112},{0,8,48},
        {0,9,192},{16,7,10},{0,8,96},{0,8,32},{0,9,160},{0,8,0},{0,8,128},
        {0,8,64},{0,9,224},{16,7,6},{0,8,88},{0,8,24},{0,9,144},{19,7,59},
        {0,8,120},{0,8,56},{0,9,208},{17,7,17},{0,8,104},{0,8,40},{0,9,176},
        {0,8,8},{0,8,136},{0,8,72},{0,9,240},{16,7,4},{0,8,84},{0,8,20},
        {21,8,227},{19,7,43},{0,8,116},{0,8,52},{0,9,200},{17,7,13},{0,8,100},
        {0,8,36},{0,9,168},{0,8,4},{0,8,132},{0,8,68},{0,9,232},{16,7,8},
        {0,8,92},{0,8,28},{0,9,152},{20,7,83},{0,8,124},{0,8,60},{0,9,216},
        {18,7,23},{0,8,108},{0,8,44},{0,9,184},{0,8,12},{0,8,140},{0,8,76},
        {0,9,248},{16,7,3},{0,8,82},{0,8,18},{21,8,163},{19,7,35},{0,8,114},
        {0,8,50},{0,9,196},{17,7,11},{0,8,98},{0,8,34},{0,9,164},{0,8,2},
        {0,8,130},{0,8,66},{0,9,228},{16,7,7},{0,8,90},{0,8,26},{0,9,148},
        {20,7,67},{0,8,122},{0,8,58},{0,9,212},{18,7,19},{0,8,106},{0,8,42},
        {0,9,180},{0,8,10},{0,8,138},{0,8,74},{0,9,244},{16,7,5},{0,8,86},
        {0,8,22},{64,8,0},{19,7,51},{0,8,118},{0,8,54},{0,9,204},{17,7,15},
        {0,8,102},{0,8,38},{0,9,172},{0,8,6},{0,8,134},{0,8,70},{0,9,236},
        {16,7,9},{0,8,94},{0,8,30},{0,9,156},{20,7,99},{0,8,126},{0,8,62},
        {0,9,220},{18,7,27},{0,8,110},{0,8,46},{0,9,188},{0,8,14},{0,8,142},
        {0,8,78},{0,9,252},{96,7,0},{0,8,81},{0,8,17},{21,8,131},{18,7,31},
        {0,8,113},{0,8,49},{0,9,194},{16,7,10},{0,8,97},{0,8,33},{0,9,162},
        {0,8,1},{0,8,129},{0,8,65},{0,9,226},{16,7,6},{0,8,89},{0,8,25},
        {0,9,146},{19,7,59},{0,8,121},{0,8,57},{0,9,210},{17,7,17},{0,8,105},
        {0,8,41},{0,9,178},{0,8,9},{0,8,137},{0,8,73},{0,9,242},{16,7,4},
        {0,8,85},{0,8,21},{16,8,258},{19,7,43},{0,8,117},{0,8,53},{0,9,202},
        {17,7,13},{0,8,101},{0,8,37},{0,9,170},{0,8,5},{0,8,133},{0,8,69},
        {0,9,234},{16,7,8},{0,8,93},{0,8,29},{0,9,154},{20,7,83},{0,8,125},
        {0,8,61},{0,9,218},{18,7,23},{0,8,109},{0,8,45},{0,9,186},{0,8,13},
        {0,8,141},{0,8,77},{0,9,250},{16,7,3},{0,8,83},{0,8,19},{21,8,195},
        {19,7,35},{0,8,115},{0,8,51},{0,9,198},{17,7,11},{0,8,99},{0,8,35},
        {0,9,166},{0,8,3},{0,8,131},{0,8,67},{0,9,230},{16,7,7},{0,8,91},
        {0,8,27},{0,9,150},{20,7,67},{0,8,123},{0,8,59},{0,9,214},{18,7,19},
        {0,8,107},{0,8,43},{0,9,182},{0,8,11},{0,8,139},{0,8,75},{0,9,246},
        {16,7,5},{0,8,87},{0,8,23},{64,8,0},{19,7,51},{0,8,119},{0,8,55},
        {0,9,206},{17,7,15},{0,8,103},{0,8,39},{0,9,174},{0,8,7},{0,8,135},
        {0,8,71},{0,9,238},{16,7,9},{0,8,95},{0,8,31},{0,9,158},{20,7,99},
        {0,8,127},{0,8,63},{0,9,222},{18,7,27},{0,8,111},{0,8,47},{0,9,190},
        {0,8,15},{0,8,143},{0,8,79},{0,9,254},{96,7,0},{0,8,80},{0,8,16},
        {20,8,115},{18,7,31},{0,8,112},{0,8,48},{0,9,193},{16,7,10},{0,8,96},
        {0,8,32},{0,9,161},{0,8,0},{0,8,128},{0,8,64},{0,9,225},{16,7,6},
        {0,8,88},{0,8,24},{0,9,145},{19,7,59},{0,8,120},{0,8,56},{0,9,209},
        {17,7,17},{0,8,104},{0,8,40},{0,9,177},{0,8,8},{0,8,136},{0,8,72},
        {0,9,241},{16,7,4},{0,8,84},{0,8,20},{21,8,227},{19,7,43},{0,8,116},
        {0,8,52},{0,9,201},{17,7,13},{0,8,100},{0,8,36},{0,9,169},{0,8,4},
        {0,8,132},{0,8,68},{0,9,233},{16,7,8},{0,8,92},{0,8,28},{0,9,153},
        {20,7,83},{0,8,124},{0,8,60},{0,9,217},{18,7,23},{0,8,108},{0,8,44},
        {0,9,185},{0,8,12},{0,8,140},{0,8,76},{0,9,249},{16,7,3},{0,8,82},
        {0,8,18},{21,8,163},{19,7,35},{0,8,114},{0,8,50},{0,9,197},{17,7,11},
        {0,8,98},{0,8,34},{0,9,165},{0,8,2},{0,8,130},{0,8,66},{0,9,229},
        {16,7,7},{0,8,90},{0,8,26},{0,9,149},{20,7,67},{0,8,122},{0,8,58},
        {0,9,213},{18,7,19},{0,8,106},{0,8,42},{0,9,181},{0,8,10},{0,8,138},
        {0,8,74},{0,9,245},{16,7,5},{0,8,86},{0,8,22},{64,8,0},{19,7,51},
        {0,8,118},{0,8,54},{0,9,205},{17,7,15},{0,8,102},{0,8,38},{0,9,173},
        {0,8,6},{0,8,134},{0,8,70},{0,9,237},{16,7,9},{0,8,94},{0,8,30},
        {0,9,157},{20,7,99},{0,8,126},{0,8,62},{0,9,221},{18,7,27},{0,8,110},
        {0,8,46},{0,9,189},{0,8,14},{0,8,142},{0,8,78},{0,9,253},{96,7,0},
        {0,8,81},{0,8,17},{21,8,131},{18,7,31},{0,8,113},{0,8,49},{0,9,195},
        {16,7,10},{0,8,97},{0,8,33},{0,9,163},{0,8,1},{0,8,129},{0,8,65},
        {0,9,227},{16,7,6},{0,8,89},{0,8,25},{0,9,147},{19,7,59},{0,8,121},
        {0,8,57},{0,9,211},{17,7,17},{0,8,105},{0,8,41},{0,9,179},{0,8,9},
        {0,8,137},{0,8,73},{0,9,243},{16,7,4},{0,8,85},{0,8,21},{16,8,258},
        {19,7,43},{0,8,117},{0,8,53},{0,9,203},{17,7,13},{0,8,101},{0,8,37},
        {0,9,171},{0,8,5},{0,8,133},{0,8,69},{0,9,235},{16,7,8},{0,8,93},
        {0,8,29},{0,9,155},{20,7,83},{0,8,125},{0,8,61},{0,9,219},{18,7,23},
        {0,8,109},{0,8,45},{0,9,187},{0,8,13},{0,8,141},{0,8,77},{0,9,251},
        {16,7,3},{0,8,83},{0,8,19},{21,8,195},{19,7,35},{0,8,115},{0,8,51},
        {0,9,199},{17,7,11},{0,8,99},{0,8,35},{0,9,167},{0,8,3},{0,8,131},
        {0,8,67},{0,9,231},{16,7,7},{0,8,91},{0,8,27},{0,9,151},{20,7,67},
        {0,8,123},{0,8,59},{0,9,215},{18,7,19},{0,8,107},{0,8,43},{0,9,183},
        {0,8,11},{0,8,139},{0,8,75},{0,9,247},{16,7,5},{0,8,87},{0,8,23},
        {64,8,0},{19,7,51},{0,8,119},{0,8,55},{0,9,207},{17,7,15},{0,8,103},
        {0,8,39},{0,9,175},{0,8,7},{0,8,135},{0,8,71},{0,9,239},{16,7,9},
        {0,8,95},{0,8,31},{0,9,159},{20,7,99},{0,8,127},{0,8,63},{0,9,223},
        {18,7,27},{0,8,111},{0,8,47},{0,9,191},{0,8,15},{0,8,143},{0,8,79},
        {0,9,255}
    };

    static const code distfix[32] = {
        {16,5,1},{23,5,257},{19,5,17},{27,5,4097},{17,5,5},{25,5,1025},
        {21,5,65},{29,5,16385},{16,5,3},{24,5,513},{20,5,33},{28,5,8193},
        {18,5,9},{26,5,2049},{22,5,129},{64,5,0},{16,5,2},{23,5,385},
        {19,5,25},{27,5,6145},{17,5,7},{25,5,1537},{21,5,97},{29,5,24577},
        {16,5,4},{24,5,769},{20,5,49},{28,5,12289},{18,5,13},{26,5,3073},
        {22,5,193},{64,5,0}
    };


/* function prototypes */
ZSC_PRIVATE I32 inflateStateCheck (z_stream * strm);
ZSC_PRIVATE void fixedtables (inflate_state *state);
ZSC_PRIVATE I32 updatewindow (z_stream * strm, const U8 *end,U32 copy);
ZSC_PRIVATE U32 syncsearch (U32 *have, const U8 *buf, U32 len);

ZSC_PRIVATE I32 inflateStateCheck(z_stream * strm)
{
    inflate_state *state;
    if (strm == Z_NULL)
        return 1;
    state = (inflate_state *)strm->state;
    if (state == Z_NULL || state->strm != strm ||
        state->mode < HEAD || state->mode > SYNC)
        return 1;
    return 0;
}

ZSC_PRIVATE void * inflate_get_work_mem(z_stream * strm, U32 items, U32 size)
{
    void * new_ptr = Z_NULL;
    U32 bytes = items * size;
    // assert no overflow
    ZSC_ASSERT3(items != 0 && bytes / items == size, bytes, items, size);

    ZSC_ASSERT(strm != Z_NULL);
    if (strm->avail_work >= bytes) {
        new_ptr = strm->next_work;
        strm->next_work += bytes;
        strm->avail_work -= bytes;
    }
    return new_ptr;
}

ZlibReturn inflateWorkSize2(I32 windowBits, U32 *size_out)
{
    ZSC_ASSERT(size_out != Z_NULL);

    /* check for wrapper bits within windowBits */
    if (windowBits < 0) {
        windowBits = -windowBits;
    } else {
        if (windowBits < 48) {
            windowBits &= 15;
        } else {
            // no special bits to properize
        }
    }

    if (windowBits && (windowBits < 8 || windowBits > 15)) {
        ZSC_WARN1("Cannot determine working size for windowBits = %d",
                windowBits);
        return Z_STREAM_ERROR;
    }

    U32 size = 0;
    size += sizeof(inflate_state);
    size += (1U << windowBits) * sizeof(U8);
    *size_out = size;

    return Z_OK;
}

ZlibReturn inflateWorkSize(U32 *size_out)
{
    return inflateWorkSize2(DEF_WBITS, size_out);
}

ZlibReturn inflateResetKeep(z_stream * strm)
{
    inflate_state *state;

    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateResetKeep(), bad inflate state.");
        return Z_STREAM_ERROR;
    }
    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    strm->total_in = strm->total_out = state->total = 0;
    strm->msg = Z_NULL;
    if (state->wrap)        /* to support ill-conceived Java test suite */
        strm->adler = state->wrap & 1;
    state->mode = HEAD;
    state->last = 0;
    state->havedict = 0;
    state->dmax = 32768U;
    state->head = Z_NULL;
    state->hold = 0;
    state->bits = 0;
    state->lencode = state->distcode = state->next = state->codes;
    state->sane = 1;
    state->back = -1;
    return Z_OK;
}

ZlibReturn inflateReset(z_stream * strm)
{
    inflate_state *state;

    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateReset(), bad inflate state.");
        return Z_STREAM_ERROR;
    }

    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    state->wsize = 0;
    state->whave = 0;
    state->wnext = 0;
    return inflateResetKeep(strm);
}

ZlibReturn inflateReset2(z_stream * strm, I32 windowBits)
{
    I32 wrap;
    inflate_state *state;

    /* get the state */
    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateReset2(), bad inflate state.");
        return Z_STREAM_ERROR;
    }
    state = (inflate_state *)strm->state;

    /* extract wrap request from windowBits parameter */
    if (windowBits < 0) {
        wrap = 0;
        windowBits = -windowBits;
    }
    else {
        wrap = (windowBits >> 4) + 5;
        if (windowBits < 48) {
            windowBits &= 15;
        }
    }

    /* set number of window bits, free window if different */
    if (windowBits && (windowBits < 8 || windowBits > 15)) {
        return Z_STREAM_ERROR;
    }

    ZSC_ASSERT(state != Z_NULL);
    if (state->window != Z_NULL && state->wbits != (U32)windowBits) {
        // Abcouwer ZSC - this case, required dynamic memory. Removed.
        return Z_STREAM_ERROR;
    }

    /* update state and reset the rest of it */
    state->wrap = wrap;
    state->wbits = (U32)windowBits;
    return inflateReset(strm);
}

I32 inflateInit2_(z_stream * strm,
        I32 windowBits, const U8 *version, I32 stream_size)
{
    I32 ret;
    inflate_state *state;

    if (version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
        stream_size != (I32)(sizeof(z_stream))){
        ZSC_WARN("In inflateInit2_(), bad version.");
        return Z_VERSION_ERROR;
    }
    if (strm == Z_NULL) {
        ZSC_WARN("In inflateInit2_(), null stream.");
        return Z_STREAM_ERROR;
    }
    U32 work_size = U32_MAX;
    if(strm->next_work == Z_NULL
            || inflateWorkSize2(windowBits, &work_size) != Z_OK
            || strm->avail_work < work_size) {
        ZSC_WARN3("In inflateInit2_(), bad stream: %d %d %d.",
                strm->next_work == Z_NULL,
                inflateWorkSize2(windowBits, &work_size) != Z_OK,
                strm->avail_work < work_size);
        return Z_STREAM_ERROR;
    }
    strm->msg = Z_NULL;                 /* in case we return an error */

    // Abcouwer ZSC -  dynamic allocation disallowed, removed functions

    state = (inflate_state *)
        inflate_get_work_mem(strm, 1, sizeof(inflate_state));
    if (state == Z_NULL) {
        ZSC_WARN("In inflateInit2_(), could not get memory for state.");
        return Z_MEM_ERROR;
    }
    strm->state = (struct internal_state *)state;
    state->strm = strm;
    state->window = Z_NULL;
    state->mode = HEAD;     /* to pass state test in inflateReset2() */
    ret = inflateReset2(strm, windowBits);
    if (ret != Z_OK) {
        strm->state = Z_NULL;
    }
    return ret;
}

ZlibReturn inflateInit_(z_stream * strm, const U8 *version, I32 stream_size)
{
    return inflateInit2_(strm, DEF_WBITS, version, stream_size);
}

ZlibReturn inflatePrime(z_stream * strm, I32 bits, I32 value)
{
    inflate_state *state;

    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflatePrime(), bad stream.");
        return Z_STREAM_ERROR;
    }
    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    if (bits < 0) {
        state->hold = 0;
        state->bits = 0;
        return Z_OK;
    }
    if (bits > 16 || state->bits + (U32)bits > 32) {
        ZSC_WARN("In inflatePrime(), bits too large.");
        return Z_STREAM_ERROR;
    }
    value &= (1L << bits) - 1;
    state->hold += (U32)value << state->bits;
    state->bits += (U32)bits;
    return Z_OK;
}

/*
   Return state with length and distance decoding tables and index sizes set to
   fixed code decoding.  Normally this returns fixed tables from inffixed.h.
   If BUILDFIXED is defined, then instead this routine builds the tables the
   first time it's called, and returns those tables the first time and
   thereafter.  This reduces the size of the code by about 2K bytes, in
   exchange for a little execution time.  However, BUILDFIXED should not be
   used for threaded applications, since the rewriting of the tables and virgin
   may not be thread-safe.
 */
ZSC_PRIVATE void fixedtables(inflate_state *state)
{
    ZSC_ASSERT(state != Z_NULL);

    state->lencode = lenfix;
    state->lenbits = 9;
    state->distcode = distfix;
    state->distbits = 5;
}

/*
   Update the window with the last wsize (normally 32K) bytes written before
   returning.  If window does not exist yet, create it.  This is only called
   when a window is already in use, or when output has been written during this
   inflate call, but the end of the deflate stream has not been reached yet.
   It is also called to create a window for dictionary data when a dictionary
   is loaded.

   Providing output buffers larger than 32K to inflate() should provide a speed
   advantage, since only the last 32K of output is copied to the sliding window
   upon return from inflate(), and since all distances after the first 32K of
   output will fall in the output data, making match copies simpler and faster.
   The advantage may be dependent on the size of the processor's data caches.
 */
ZSC_PRIVATE I32 updatewindow(z_stream * strm, const U8 *end, U32 copy)
{
    ZSC_ASSERT(strm != Z_NULL);
    ZSC_ASSERT(end != Z_NULL);

    inflate_state *state;
    U32 dist;

    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);

    /* if it hasn't been done already, allocate space for the window */
    if (state->window == Z_NULL) {
        state->window = (U8 *)
        inflate_get_work_mem(strm, 1U << state->wbits,
                               sizeof(U8));
        if (state->window == Z_NULL) return 1;
    }

    /* if window not in use yet, initialize */
    if (state->wsize == 0) {
        state->wsize = 1U << state->wbits;
        state->wnext = 0;
        state->whave = 0;
    }

    /* copy state->wsize or less output bytes into the circular window */
    if (copy >= state->wsize) {
        zmemcpy(state->window, end - state->wsize, state->wsize);
        state->wnext = 0;
        state->whave = state->wsize;
    }
    else {
        dist = state->wsize - state->wnext;
        if (dist > copy) dist = copy;
        zmemcpy(state->window + state->wnext, end - copy, dist);
        copy -= dist;
        if (copy) {
            zmemcpy(state->window, end - copy, copy);
            state->wnext = copy;
            state->whave = state->wsize;
        }
        else {
            state->wnext += dist;
            if (state->wnext == state->wsize) state->wnext = 0;
            if (state->whave < state->wsize) state->whave += dist;
        }
    }
    return 0;
}

/* Macros for inflate(): */

/* check function to use adler32() for zlib or crc32() for gzip */
#  define UPDATE(check, buf, len) \
    (state->flags ? crc32((check), (buf), (len)) : adler32((check), (buf), (len)))

/* check macros for header crc */
#  define CRC2(check, word) \
    do { \
        hbuf[0] = (U8)(word); \
        hbuf[1] = (U8)((word) >> 8); \
        (check) = crc32((check), hbuf, 2); \
    } while (0)

#  define CRC4(check, word) \
    do { \
        hbuf[0] = (U8)(word); \
        hbuf[1] = (U8)((word) >> 8); \
        hbuf[2] = (U8)((word) >> 16); \
        hbuf[3] = (U8)((word) >> 24); \
        (check) = crc32((check), hbuf, 4); \
    } while (0)

/* Load registers with state in inflate() for speed */
#define LOAD() \
    do { \
        put = strm->next_out; \
        left = strm->avail_out; \
        next = strm->next_in; \
        have = strm->avail_in; \
        hold = state->hold; \
        bits = state->bits; \
    } while (0)

/* Restore state from registers in inflate() */
#define RESTORE() \
    do { \
        strm->next_out = put; \
        strm->avail_out = left; \
        strm->next_in = next; \
        strm->avail_in = have; \
        state->hold = hold; \
        state->bits = bits; \
    } while (0)

/* Clear the input bit accumulator */
#define INITBITS() \
    do { \
        hold = 0; \
        bits = 0; \
    } while (0)

/* Get a byte of input into the bit accumulator, or return from inflate()
   if there is no input available. */
#define PULLBYTE() \
    do { \
        if (have == 0) goto inf_leave; \
        have--; \
        hold += (U32)(*next++) << bits; \
        bits += 8; \
    } while (0)

/* Assure that there are at least n bits in the bit accumulator.  If there is
   not enough available input to do that, then return from inflate(). */
#define NEEDBITS(n) \
    do { \
        while (bits < (U32)(n)) { \
            PULLBYTE(); \
        } \
    } while (0)

/* Return the low n bits of the bit accumulator (n < 16) */
#define BITS(n) \
    ((U32)hold & ((1U << (n)) - 1))

/* Remove n bits from the bit accumulator */
#define DROPBITS(n) \
    do { \
        hold >>= (n); \
        bits -= (U32)(n); \
    } while (0)

/* Remove zero to seven bits as needed to go to a byte boundary */
#define BYTEBITS() \
    do { \
        hold >>= bits & 7; \
        bits -= bits & 7; \
    } while (0)

/*
   inflate() uses a state machine to process as much input data and generate as
   much output data as possible before returning.  The state machine is
   structured roughly as follows:

    for (;;) switch (state) {
    ...
    case STATEn:
        if (not enough input data or output space to make progress)
            return;
        ... make progress ...
        state = STATEm;
        break;
    ...
    }

   so when inflate() is called again, the same case is attempted again, and
   if the appropriate resources are provided, the machine proceeds to the
   next state.  The NEEDBITS() macro is usually the way the state evaluates
   whether it can proceed or should return.  NEEDBITS() does the return if
   the requested bits are not available.  The typical use of the BITS macros
   is:

        NEEDBITS(n);
        ... do something with BITS(n) ...
        DROPBITS(n);

   where NEEDBITS(n) either returns from inflate() if there isn't enough
   input left to load n bits into the accumulator, or it continues.  BITS(n)
   gives the low n bits in the accumulator.  When done, DROPBITS(n) drops
   the low n bits off the accumulator.  INITBITS() clears the accumulator
   and sets the number of available bits to zero.  BYTEBITS() discards just
   enough bits to put the accumulator on a byte boundary.  After BYTEBITS()
   and a NEEDBITS(8), then BITS(8) would return the next byte in the stream.

   NEEDBITS(n) uses PULLBYTE() to get an available byte of input, or to return
   if there is no input available.  The decoding of variable length codes uses
   PULLBYTE() directly in order to pull just enough bytes to decode the next
   code, and no more.

   Some states loop until they get enough input, making sure that enough
   state information is maintained to continue the loop where it left off
   if NEEDBITS() returns in the loop.  For example, want, need, and keep
   would all have to actually be part of the saved state in case NEEDBITS()
   returns:

    case STATEw:
        while (want < need) {
            NEEDBITS(n);
            keep[want++] = BITS(n);
            DROPBITS(n);
        }
        state = STATEx;
    case STATEx:

   As shown above, if the next state is also the next case, then the break
   is omitted.

   A state may also return if there is not enough output space available to
   complete that state.  Those states are copying stored data, writing a
   literal byte, and copying a matching string.

   When returning, a "goto inf_leave" is used to update the total counters,
   update the check value, and determine whether any progress has been made
   during that inflate() call in order to return the proper return code.
   Progress is defined as a change in either strm->avail_in or strm->avail_out.
   When there is a window, goto inf_leave will update the window with the last
   output written.  If a goto inf_leave occurs in the middle of decompression
   and there is no window currently, goto inf_leave will create one and copy
   output to the window for the next call of inflate().

   In this implementation, the flush parameter of inflate() only affects the
   return code (per zlib.h).  inflate() always writes as much as possible to
   strm->next_out, given the space available and the provided input--the effect
   documented in zlib.h of Z_SYNC_FLUSH.  Furthermore, inflate() always defers
   the allocation of and copying into a sliding window until necessary, which
   provides the effect documented in zlib.h for Z_FINISH when the entire input
   stream available.  So the only thing the flush parameter actually does is:
   when flush is set to Z_FINISH, inflate() cannot return Z_OK.  Instead it
   will return Z_BUF_ERROR if it has not reached the end of the stream.
 */

ZlibReturn inflate(z_stream * strm, ZlibFlush flush)
{
    inflate_state *state;
    const U8 *next;    /* next input */
    U8 *put;     /* next output */
    U32 have, left;        /* available input and output */
    U32 hold;         /* bit buffer */
    U32 bits;              /* bits in bit buffer */
    U32 in, out;           /* save starting available input and output */
    U32 copy;              /* number of stored or match bytes to copy */
    U8 *from;    /* where to copy match bytes from */
    code here;                  /* current decoding table entry */
    code last;                  /* parent table entry */
    U32 len;               /* length to copy for repeats, bits to drop */
    I32 ret;                    /* return code */
    U8 hbuf[4];      /* buffer for gzip header crc calculation */
    static const U16 order[19] = /* permutation of code lengths */
        {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    if (inflateStateCheck(strm) || strm->next_out == Z_NULL
            || (strm->next_in == Z_NULL && strm->avail_in != 0)) {
        ZSC_WARN("In inflate(), bad stream or buffers.");
        return Z_STREAM_ERROR;
    }
    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    if (state->mode == TYPE) {
        state->mode = TYPEDO;      /* skip check */
    }
    LOAD();
    in = have;
    out = left;
    ret = Z_OK;
    for (;;) {
        switch (state->mode) {
        case HEAD:
            if (state->wrap == 0) {
                state->mode = TYPEDO;
                break;
            }
            NEEDBITS(16);
            if ((state->wrap & 2) && hold == 0x8b1f) {  /* gzip header */
                if (state->wbits == 0) {
                    state->wbits = 15;
                }
                state->check = crc32(0L, Z_NULL, 0);
                CRC2(state->check, hold);
                INITBITS();
                state->mode = FLAGS;
                break;
            }
            state->flags = 0;           /* expect zlib header */
            if (state->head != Z_NULL) {
                state->head->done = -1;
            }
            if (!(state->wrap & 1) ||   /* check if zlib header allowed */
                ((BITS(8) << 8) + (hold >> 8)) % 31) {
                strm->msg = (U8*)"incorrect header check";
                state->mode = BAD;
                break;
            }
            if (BITS(4) != Z_DEFLATED) {
                strm->msg = (U8*)"unknown compression method";
                state->mode = BAD;
                break;
            }
            DROPBITS(4);
            len = BITS(4) + 8;
            if (state->wbits == 0) {
                state->wbits = len;
            }
            if (len > 15 || len > state->wbits) {
                strm->msg = (U8*)"invalid window size";
                state->mode = BAD;
                break;
            }
            state->dmax = 1U << len;
            strm->adler = state->check = adler32(0L, Z_NULL, 0);
            state->mode = hold & 0x200 ? DICTID : TYPE;
            INITBITS();
            break;
        case FLAGS:
            NEEDBITS(16);
            state->flags = (I32)(hold);
            if ((state->flags & 0xff) != Z_DEFLATED) {
                strm->msg = (U8*)"unknown compression method";
                state->mode = BAD;
                break;
            }
            if (state->flags & 0xe000) {
                strm->msg = (U8*)"unknown header flags set";
                state->mode = BAD;
                break;
            }
            if (state->head != Z_NULL) {
                state->head->text = (I32)((hold >> 8) & 1);
            }
            if ((state->flags & 0x0200) && (state->wrap & 4)) {
                CRC2(state->check, hold);
            }
            INITBITS();
            state->mode = TIME;
            break;
        case TIME:
            NEEDBITS(32);
            if (state->head != Z_NULL) {
                state->head->time = hold;
            }
            if ((state->flags & 0x0200) && (state->wrap & 4)) {
                CRC4(state->check, hold);
            }
            INITBITS();
            state->mode = OS;
            break;
        case OS:
            NEEDBITS(16);
            if (state->head != Z_NULL) {
                state->head->xflags = (I32)(hold & 0xff);
                state->head->os = (I32)(hold >> 8);
            }
            if ((state->flags & 0x0200) && (state->wrap & 4)) {
                CRC2(state->check, hold);
            }
            INITBITS();
            state->mode = EXLEN;
            break;
        case EXLEN:
            if (state->flags & 0x0400) {
                NEEDBITS(16);
                state->length = (U32)(hold);
                if (state->head != Z_NULL) {
                    state->head->extra_len = (U32)hold;
                }
                if ((state->flags & 0x0200) && (state->wrap & 4)) {
                    CRC2(state->check, hold);
                }
                INITBITS();
            } else if (state->head != Z_NULL) {
                state->head->extra = Z_NULL;
            } else {
                // no header, do nothing
            }
            state->mode = EXTRA;
            break;
        case EXTRA:
            if (state->flags & 0x0400) {
                copy = state->length;
                if (copy > have) {
                    copy = have;
                }
                if (copy) {
                    if (state->head != Z_NULL &&
                        state->head->extra != Z_NULL) {
                        len = state->head->extra_len - state->length;
                        zmemcpy(state->head->extra + len, next,
                                len + copy > state->head->extra_max ?
                                state->head->extra_max - len : copy);
                    }
                    if ((state->flags & 0x0200) && (state->wrap & 4)) {
                        state->check = crc32(state->check, next, copy);
                    }
                    have -= copy;
                    next += copy;
                    state->length -= copy;
                }
                if (state->length) {
                    goto inf_leave;
                }
            }
            state->length = 0;
            state->mode = NAME;
            break;
        case NAME:
            if (state->flags & 0x0800) {
                if (have == 0) {
                    goto inf_leave;
                }
                copy = 0;
                do {
                    len = (U32)(next[copy++]);
                    if (state->head != Z_NULL &&
                            state->head->name != Z_NULL &&
                            state->length < state->head->name_max) {
                        state->head->name[state->length++] = (U8)len;
                    }
                } while (len && copy < have);
                if ((state->flags & 0x0200) && (state->wrap & 4)) {
                    state->check = crc32(state->check, next, copy);
                }
                have -= copy;
                next += copy;
                if (len) {
                    goto inf_leave;
                }
            } else if (state->head != Z_NULL) {
                state->head->name = Z_NULL;
            } else {
                // no header, do nothing
            }
            state->length = 0;
            state->mode = COMMENT;
            break;
        case COMMENT:
            if (state->flags & 0x1000) {
                if (have == 0) {
                    goto inf_leave;
                }
                copy = 0;
                do {
                    len = (U32)(next[copy++]);
                    if (state->head != Z_NULL &&
                            state->head->comment != Z_NULL &&
                            state->length < state->head->comm_max) {
                        state->head->comment[state->length++] = (U8)len;
                    }
                } while (len && copy < have);
                if ((state->flags & 0x0200) && (state->wrap & 4)) {
                    state->check = crc32(state->check, next, copy);
                }
                have -= copy;
                next += copy;
                if (len) {
                    goto inf_leave;
                }
            } else if (state->head != Z_NULL) {
                state->head->comment = Z_NULL;
            } else {
                // no header, do nothing
            }
            state->mode = HCRC;
            break;
        case HCRC:
            if (state->flags & 0x0200) {
                NEEDBITS(16);
                if ((state->wrap & 4) && hold != (state->check & 0xffff)) {
                    strm->msg = (U8*)"header crc mismatch";
                    state->mode = BAD;
                    break;
                }
                INITBITS();
            }
            if (state->head != Z_NULL) {
                state->head->hcrc = (I32)((state->flags >> 9) & 1);
                state->head->done = 1;
            }
            strm->adler = state->check = crc32(0L, Z_NULL, 0);
            state->mode = TYPE;
            break;
        case DICTID:
            NEEDBITS(32);
            strm->adler = state->check = ZSWAP32(hold);
            INITBITS();
            state->mode = DICT;
            break;
        case DICT:
            if (state->havedict == 0) {
                RESTORE();
                return Z_NEED_DICT;
            }
            strm->adler = state->check = adler32(0L, Z_NULL, 0);
            state->mode = TYPE;
            break;
        case TYPE:
            if (flush == Z_BLOCK || flush == Z_TREES) {
                goto inf_leave;
            }
            state->mode = TYPEDO;
            break;
        case TYPEDO:
            if (state->last) {
                BYTEBITS();
                state->mode = CHECK;
                break;
            }
            NEEDBITS(3);
            state->last = BITS(1);
            DROPBITS(1);
            switch (BITS(2)) {
            case 0:                             /* stored block */
                state->mode = STORED;
                break;
            case 1:                             /* fixed block */
                fixedtables(state);
                state->mode = LEN_;             /* decode codes */
                if (flush == Z_TREES) {
                    DROPBITS(2);
                    goto inf_leave;
                }
                break;
            case 2:                             /* dynamic block */
                state->mode = TABLE;
                break;
            case 3:
                strm->msg = (U8*)"invalid block type";
                state->mode = BAD;
                break;
            default: // Abcouwer ZSC - misra needs defalt cases
                strm->msg = (U8*)"very invalid block type";
                state->mode = BAD;
                break;
            }
            DROPBITS(2);
            break;
        case STORED:
            BYTEBITS();                         /* go to byte boundary */
            NEEDBITS(32);
            if ((hold & 0xffff) != ((hold >> 16) ^ 0xffff)) {
                strm->msg = (U8*)"invalid stored block lengths";
                state->mode = BAD;
                break;
            }
            state->length = (U32)hold & 0xffff;
            INITBITS();
            state->mode = COPY_;
            if (flush == Z_TREES) {
                goto inf_leave;
            }
            break;
        case COPY_:
            state->mode = COPY;
            break;
        case COPY:
            copy = state->length;
            if (copy) {
                if (copy > have) {
                    copy = have;
                }
                if (copy > left) {
                    copy = left;
                }
                if (copy == 0) {
                    goto inf_leave;
                }
                zmemcpy(put, next, copy);
                have -= copy;
                next += copy;
                left -= copy;
                put += copy;
                state->length -= copy;
                break;
            }
            state->mode = TYPE;
            break;
        case TABLE:
            NEEDBITS(14);
            state->nlen = BITS(5) + 257;
            DROPBITS(5);
            state->ndist = BITS(5) + 1;
            DROPBITS(5);
            state->ncode = BITS(4) + 4;
            DROPBITS(4);
            if (state->nlen > 286 || state->ndist > 30) {
                strm->msg = (U8*)"too many length or distance symbols";
                state->mode = BAD;
                break;
            }
            state->have = 0;
            state->mode = LENLENS;
            break;
        case LENLENS:
            while (state->have < state->ncode) {
                NEEDBITS(3);
                state->lens[order[state->have++]] = (U16)BITS(3);
                DROPBITS(3);
            }
            while (state->have < 19) {
                state->lens[order[state->have++]] = 0;
            }
            state->next = state->codes;
            state->lencode = (const code *)(state->next);
            state->lenbits = 7;
            ret = inflate_table(CODES, state->lens, 19, &(state->next),
                                &(state->lenbits), state->work);
            if (ret) {
                strm->msg = (U8*)"invalid code lengths set";
                state->mode = BAD;
                break;
            }
            state->have = 0;
            state->mode = CODELENS;
            break;
        case CODELENS:
            while (state->have < state->nlen + state->ndist) {
                for (;;) {
                    here = state->lencode[BITS(state->lenbits)];
                    if ((U32)(here.bits) <= bits) {
                        break;
                    }
                    PULLBYTE();
                }
                if (here.val < 16) {
                    DROPBITS(here.bits);
                    state->lens[state->have++] = here.val;
                }
                else {
                    if (here.val == 16) {
                        NEEDBITS(here.bits + 2);
                        DROPBITS(here.bits);
                        if (state->have == 0) {
                            strm->msg = (U8*)"invalid bit length repeat";
                            state->mode = BAD;
                            break;
                        }
                        len = state->lens[state->have - 1];
                        copy = 3 + BITS(2);
                        DROPBITS(2);
                    }
                    else if (here.val == 17) {
                        NEEDBITS(here.bits + 3);
                        DROPBITS(here.bits);
                        len = 0;
                        copy = 3 + BITS(3);
                        DROPBITS(3);
                    }
                    else {
                        NEEDBITS(here.bits + 7);
                        DROPBITS(here.bits);
                        len = 0;
                        copy = 11 + BITS(7);
                        DROPBITS(7);
                    }
                    if (state->have + copy > state->nlen + state->ndist) {
                        strm->msg = (U8*)"invalid bit length repeat";
                        state->mode = BAD;
                        break;
                    }
                    while (copy) {
                        copy--;
                        state->lens[state->have++] = (U16)len;
                    }
                }
            }

            /* handle error breaks in while */
            if (state->mode == BAD) {
                break;
            }

            /* check for end-of-block code (better have one) */
            if (state->lens[256] == 0) {
                strm->msg = (U8*)"invalid code -- missing end-of-block";
                state->mode = BAD;
                break;
            }

            /* build code tables -- note: do not change the lenbits or distbits
               values here (9 and 6) without reading the comments in inftrees.h
               concerning the ENOUGH constants, which depend on those values */
            state->next = state->codes;
            state->lencode = (const code *)(state->next);
            state->lenbits = 9;
            ret = inflate_table(LENS, state->lens, state->nlen, &(state->next),
                                &(state->lenbits), state->work);
            if (ret) {
                strm->msg = (U8*)"invalid literal/lengths set";
                state->mode = BAD;
                break;
            }
            state->distcode = (const code *)(state->next);
            state->distbits = 6;
            ret = inflate_table(DISTS, state->lens + state->nlen, state->ndist,
                            &(state->next), &(state->distbits), state->work);
            if (ret) {
                strm->msg = (U8*)"invalid distances set";
                state->mode = BAD;
                break;
            }
            state->mode = LEN_;
            if (flush == Z_TREES) {
                goto inf_leave;
            }
            break;
        case LEN_:
            state->mode = LEN;
            break;
        case LEN:
            if (have >= 6 && left >= 258) {
                RESTORE();
                inflate_fast(strm, out);
                LOAD();
                if (state->mode == TYPE) {
                    state->back = -1;
                }
                break;
            }
            state->back = 0;
            for (;;) {
                here = state->lencode[BITS(state->lenbits)];
                if ((U32)(here.bits) <= bits) {
                    break;
                }
                PULLBYTE();
            }
            if (here.op && (here.op & 0xf0) == 0) {
                last = here;
                for (;;) {
                    here = state->lencode[last.val +
                            (BITS(last.bits + last.op) >> last.bits)];
                    if ((U32)(last.bits + here.bits) <= bits) break;
                    PULLBYTE();
                }
                DROPBITS(last.bits);
                state->back += last.bits;
            }
            DROPBITS(here.bits);
            state->back += here.bits;
            state->length = (U32)here.val;
            if ((I32)(here.op) == 0) {
                state->mode = LIT;
                break;
            }
            if (here.op & 32) {
                state->back = -1;
                state->mode = TYPE;
                break;
            }
            if (here.op & 64) {
                strm->msg = (U8*)"invalid literal/length code";
                state->mode = BAD;
                break;
            }
            state->extra = (U32)(here.op) & 15;
            state->mode = LENEXT;
            break;
        case LENEXT:
            if (state->extra) {
                NEEDBITS(state->extra);
                state->length += BITS(state->extra);
                DROPBITS(state->extra);
                state->back += state->extra;
            }
            state->was = state->length;
            state->mode = DIST;
            break;
        case DIST:
            for (;;) {
                here = state->distcode[BITS(state->distbits)];
                if ((U32)(here.bits) <= bits) break;
                PULLBYTE();
            }
            if ((here.op & 0xf0) == 0) {
                last = here;
                for (;;) {
                    here = state->distcode[last.val +
                            (BITS(last.bits + last.op) >> last.bits)];
                    if ((U32)(last.bits + here.bits) <= bits) break;
                    PULLBYTE();
                }
                DROPBITS(last.bits);
                state->back += last.bits;
            }
            DROPBITS(here.bits);
            state->back += here.bits;
            if (here.op & 64) {
                strm->msg = (U8*)"invalid distance code";
                state->mode = BAD;
                break;
            }
            state->offset = (U32)here.val;
            state->extra = (U32)(here.op) & 15;
            state->mode = DISTEXT;
            break;
        case DISTEXT:
            if (state->extra) {
                NEEDBITS(state->extra);
                state->offset += BITS(state->extra);
                DROPBITS(state->extra);
                state->back += state->extra;
            }
            if (state->offset > state->dmax) {
                strm->msg = (U8*)"invalid distance too far back";
                state->mode = BAD;
                break;
            }
            state->mode = MATCH;
            break;
        case MATCH:
            if (left == 0) goto inf_leave;
            copy = out - left;
            if (state->offset > copy) {         /* copy from window */
                copy = state->offset - copy;
                if (copy > state->whave) {
                    if (state->sane) {
                        strm->msg = (U8*)"invalid distance too far back";
                        state->mode = BAD;
                        break;
                    }
                }
                if (copy > state->wnext) {
                    copy -= state->wnext;
                    from = state->window + (state->wsize - copy);
                }
                else
                    from = state->window + (state->wnext - copy);
                if (copy > state->length) copy = state->length;
            }
            else {                              /* copy from output */
                from = put - state->offset;
                copy = state->length;
            }
            if (copy > left) copy = left;
            left -= copy;
            state->length -= copy;
            do {
                *put++ = *from++;
                copy--;
            } while (copy);
            if (state->length == 0) state->mode = LEN;
            break;
        case LIT:
            if (left == 0) goto inf_leave;
            *put++ = (U8)(state->length);
            left--;
            state->mode = LEN;
            break;
        case CHECK:
            if (state->wrap) {
                NEEDBITS(32);
                out -= left;
                strm->total_out += out;
                state->total += out;
                if ((state->wrap & 4) && out) {
                    strm->adler = state->check =
                        UPDATE(state->check, put - out, out);
                }
                out = left;
                if ((state->wrap & 4)
                        && (state->flags ? hold : ZSWAP32(hold)) != state->check) {
                    strm->msg = (U8*) "incorrect data check";
                    state->mode = BAD;
                    break;
                }
                INITBITS();
            }
            state->mode = LENGTH;
            break;
        case LENGTH:
            if (state->wrap && state->flags) {
                NEEDBITS(32);
                if (hold != (state->total & 0xffffffffUL)) {
                    strm->msg = (U8*)"incorrect length check";
                    state->mode = BAD;
                    break;
                }
                INITBITS();
            }
            state->mode = DONE;
            break;
        case DONE:
            ret = Z_STREAM_END;
            goto inf_leave;
            break;
        case BAD:
            ret = Z_DATA_ERROR;
            goto inf_leave;
            break;
        case MEM:
            return Z_MEM_ERROR;
            break;
        case SYNC:
        default:
            return Z_STREAM_ERROR;
            break;
        }
    }
    /*
       Return from inflate(), updating the total counts and the check value.
       If there was no progress during the inflate() call, return a buffer
       error.  Call updatewindow() to create and/or update the window state.
       Note: a memory error from inflate() is non-recoverable.
     */
  inf_leave:
    RESTORE();
    if (state->wsize || (out != strm->avail_out && state->mode < BAD &&
            (state->mode < CHECK || flush != Z_FINISH))) {
        if (updatewindow(strm, strm->next_out, out - strm->avail_out)) {
            state->mode = MEM;
            ZSC_WARN("In inflate(), updatewindow() failed.");
            return Z_MEM_ERROR;
        }
    }
    in -= strm->avail_in;
    out -= strm->avail_out;
    strm->total_in += in;
    strm->total_out += out;
    state->total += out;
    if ((state->wrap & 4) && out) {
        strm->adler = state->check =
            UPDATE(state->check, strm->next_out - out, out);
    }
    strm->data_type = (I32)state->bits + (state->last ? 64 : 0) +
                      (state->mode == TYPE ? 128 : 0) +
                      (state->mode == LEN_ || state->mode == COPY_ ? 256 : 0);
    if (((in == 0 && out == 0) || flush == Z_FINISH) && ret == Z_OK) {
        ret = Z_BUF_ERROR;
    }
    return ret;
}

ZlibReturn inflateEnd(z_stream * strm)
{
    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateEnd(), bad state.");
        return Z_STREAM_ERROR;
    }

    // Abcouwer ZSC - no dynamic allocation, removed frees

    ZSC_ASSERT(strm != Z_NULL);
    strm->state = Z_NULL;
    return Z_OK;
}

ZlibReturn inflateGetDictionary(z_stream * strm,
        U8 *dictionary, U32 *dictLength)
{
    inflate_state *state;

    /* check state */
    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateGetDictionary(), bad state.");
        return Z_STREAM_ERROR;
    }
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);

    /* copy dictionary */
    if (state->whave && dictionary != Z_NULL) {
        zmemcpy(dictionary, state->window + state->wnext,
                state->whave - state->wnext);
        zmemcpy(dictionary + state->whave - state->wnext,
                state->window, state->wnext);
    }
    if (dictLength != Z_NULL) {
        *dictLength = state->whave;
    }
    return Z_OK;
}

ZlibReturn inflateSetDictionary(z_stream * strm,
        const U8 *dictionary, U32 dictLength)
{
    inflate_state *state;
    U32 dictid;
    I32 ret;

    /* check state */
    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateSetDictionary(), bad state.");
        return Z_STREAM_ERROR;
    }
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    if (state->wrap != 0 && state->mode != DICT) {
        ZSC_WARN2("In inflateSetDictionary(), bad wrapper (%d) or mode (%d).",
                state->wrap, state->mode);
        return Z_STREAM_ERROR;
    }

    /* check for correct dictionary identifier */
    if (state->mode == DICT) {
        dictid = adler32(0L, Z_NULL, 0);
        dictid = adler32(dictid, dictionary, dictLength);
        if (dictid != state->check) {
            ZSC_WARN("In inflateSetDictionary(), bad dictionary id.");
            return Z_DATA_ERROR;
        }
    }

    /* copy dictionary to window using updatewindow(), which will amend the
       existing dictionary if appropriate */
    ret = updatewindow(strm, dictionary + dictLength, dictLength);
    if (ret) {
        state->mode = MEM;
        ZSC_WARN("In inflateSetDictionary(), updatewindow() failed.");
        return Z_MEM_ERROR;
    }
    state->havedict = 1;
    return Z_OK;
}

ZlibReturn inflateGetHeader(z_stream * strm, gz_header * head)
{
    inflate_state *state;

    /* check state */
    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateGetHeader(), bad state.");
        return Z_STREAM_ERROR;
    }
    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    if ((state->wrap & 2) == 0) {
        ZSC_WARN("In inflateGetHeader(), strm is not gzip.");
        return Z_STREAM_ERROR;
    }

    /* save header structure */
    state->head = head;
    ZSC_ASSERT(head != Z_NULL);
    head->done = 0;
    return Z_OK;
}

/*
   Search buf[0..len-1] for the pattern: 0, 0, 0xff, 0xff.  Return when found
   or when out of input.  When called, *have is the number of pattern bytes
   found in order so far, in 0..3.  On return *have is updated to the new
   state.  If on return *have equals four, then the pattern was found and the
   return value is how many bytes were read including the last byte of the
   pattern.  If *have is less than four, then the pattern has not been found
   yet and the return value is len.  In the latter case, syncsearch() can be
   called again with more data and the *have state.  *have is initialized to
   zero for the first call.
 */
ZSC_PRIVATE U32 syncsearch(U32 *have, const U8 *buf, U32 len)
{
    ZSC_ASSERT(have != Z_NULL);
    ZSC_ASSERT(buf != Z_NULL);

    U32 got;
    U32 next;

    got = *have;
    next = 0;
    while (next < len && got < 4) {
        if ((I32)(buf[next]) == (got < 2 ? 0 : 0xff)) {
            got++;
        } else if (buf[next]) {
            got = 0;
        } else {
            got = 4 - got;
        }
        next++;
    }
    *have = got;
    return next;
}

ZlibReturn inflateSync(z_stream * strm)
{
    U32 len;               /* number of bytes to look at or looked at */
    U32 in, out;      /* temporary to save total_in and total_out */
    U8 buf[4];       /* to restore bit buffer to byte string */
    inflate_state *state;

    /* check parameters */
    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateSync(), bad state.");
        return Z_STREAM_ERROR;
    }
    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    if (strm->avail_in == 0 && state->bits < 8) {
        ZSC_WARN("In inflateSync(), not enough input.");
        return Z_BUF_ERROR;
    }

    /* if first time, start search in bit buffer */
    if (state->mode != SYNC) {
        state->mode = SYNC;
        state->hold <<= state->bits & 7;
        state->bits -= state->bits & 7;
        len = 0;
        while (state->bits >= 8) {
            buf[len++] = (U8)(state->hold);
            state->hold >>= 8;
            state->bits -= 8;
        }
        state->have = 0;
        (void)syncsearch(&(state->have), buf, len);
    }

    /* search available input */
    len = syncsearch(&(state->have), strm->next_in, strm->avail_in);
    strm->avail_in -= len;
    strm->next_in += len;
    strm->total_in += len;

    /* return no joy or set up to restart inflate() on a new block */
    if (state->have != 4) {
        ZSC_WARN("In inflateSync(), did not find 4 bytes.");
        return Z_DATA_ERROR;
    }
    in = strm->total_in;  out = strm->total_out;
    I32 ir_ret = inflateReset(strm);
    if (ir_ret != Z_OK) {
        ZSC_WARN1("In inflateSync(), inflateReset() returned %d.", ir_ret);
        return ir_ret;
    }
    strm->total_in = in;  strm->total_out = out;
    state->mode = TYPE;
    return Z_OK;
}

/*
   Returns true if inflate is currently at the end of a block generated by
   Z_SYNC_FLUSH or Z_FULL_FLUSH. This function is used by one PPP
   implementation to provide an additional safety check. PPP uses
   Z_SYNC_FLUSH but removes the length bytes of the resulting empty stored
   block. When decompressing, PPP checks that at the end of input packet,
   inflate is waiting for these length bytes.
 */
ZlibReturn inflateSyncPoint(z_stream * strm)
{
    inflate_state *state;

    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateSyncPoint(), bad state.");
        return Z_STREAM_ERROR;
    }

    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    return state->mode == STORED && state->bits == 0;
}

// Abcouwer ZSC - remove inflateCopy()
// copying then doing allocs will spool more memory from the work buffer

ZlibReturn inflateUndermine(z_stream * strm, I32 subvert)
{
    inflate_state *state;

    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateUndermine(), bad state.");
        return Z_STREAM_ERROR;
    }

    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    (void)subvert;
    ZSC_ASSERT(state != Z_NULL);
    state->sane = 1;
    return Z_DATA_ERROR;
}

ZlibReturn inflateValidate(z_stream * strm, I32 check)
{
    inflate_state *state;

    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateValidate(), bad state.");
        return Z_STREAM_ERROR;
    }

    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    if (check) {
        state->wrap |= 4;
    } else {
        state->wrap &= ~4;
    }
    return Z_OK;
}

I32 inflateMark(z_stream * strm)
{
    inflate_state *state;

    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateMark(), bad state.");
        return -(1L << 16);
    }

    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    return (I32)(((U32)((I32)state->back)) << 16) +
        (state->mode == COPY ? state->length :
            (state->mode == MATCH ? state->was - state->length : 0));
}

U32 inflateCodesUsed(z_stream * strm)
{
    inflate_state *state;
    if (inflateStateCheck(strm)) {
        ZSC_WARN("In inflateCodesUsed(), bad state.");
        return U32_MAX;
    }

    ZSC_ASSERT(strm != Z_NULL);
    state = (inflate_state *)strm->state;
    ZSC_ASSERT(state != Z_NULL);
    return (U32)(state->next - state->codes);
}
