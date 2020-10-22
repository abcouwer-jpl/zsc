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
 * @file        adler32.c
 * @date        2020-10-12
 * @author      Mark Adler, Neil Abcouwer
 * @brief       Adler-32 checksums
 *
 * Modified version of adler32-c for safety-critical applications.
 * Safer macros, fixed-length types, other small changes.
 *
 * Original file header follows.
 */

/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-2011, 2016 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#include "zsc/zutil.h"


#define BASE 65521U     /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i)  {adler += (buf)[(i)]; sum2 += adler;}
#define DO2(buf,i)  DO1((buf),(i)); DO1((buf),(i)+1);
#define DO4(buf,i)  DO2((buf),(i)); DO2((buf),(i)+2);
#define DO8(buf,i)  DO4((buf),(i)); DO4((buf),(i)+4);
#define DO16(buf)   DO8((buf),0); DO8((buf),8);

// Abcouwer ZSC - remove NO_DIVIDE conditional
#define MOD(a) (a) %= BASE
#define MOD28(a) (a) %= BASE
#define MOD63(a) (a) %= BASE

/* ========================================================================= */
U32 adler32_z(adler, buf, len)
    U32 adler;
    const U8 *buf;
    z_size_t len;
{
    U32 sum2;
    U32 n;

    /* split Adler-32 into component sums */
    sum2 = (adler >> 16) & 0xffff;
    adler &= 0xffff;

    /* in case user likes doing a byte at a time, keep it fast */
    if (len == 1) {
        adler += buf[0];
        if (adler >= BASE) {
            adler -= BASE;
        }
        sum2 += adler;
        if (sum2 >= BASE) {
            sum2 -= BASE;
        }
        return adler | (sum2 << 16);
    }

    /* initial Adler-32 value (deferred check for len == 1 speed) */
    if (buf == Z_NULL) {
        return 1L;
    }

    /* in case short lengths are provided, keep it somewhat fast */
    if (len < 16) {
        while (len) {
            len--;
            adler += *buf++;
            sum2 += adler;
        }
        if (adler >= BASE) {
            adler -= BASE;
        }
        MOD28(sum2);            /* only added so many BASE's */
        return adler | (sum2 << 16);
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while (len >= NMAX) {
        len -= NMAX;
        n = NMAX / 16;          /* NMAX is divisible by 16 */
        do {
            DO16(buf);          /* 16 sums unrolled */
            buf += 16;
            --n;
        } while (n);
        MOD(adler);
        MOD(sum2);
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if (len) {                  /* avoid modulos if none remaining */
        while (len >= 16) {
            len -= 16;
            DO16(buf);
            buf += 16;
        }
        while (len) {
            len--;
            adler += *buf++;
            sum2 += adler;
        }
        MOD(adler);
        MOD(sum2);
    }

    /* return recombined sums */
    return adler | (sum2 << 16);
}

/* ========================================================================= */
U32 adler32(adler, buf, len)
    U32 adler;
    const U8 *buf;
    U32 len;
{
    return adler32_z(adler, buf, len);
}

// Abcouwer ZSC - Remove adler combine functions
// Joining two compressed buffers is beyond scope of ZSC.
