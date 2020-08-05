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
 * @file        deflate.c
 * @date        2020-08-05
 * @author      Jean-loup Gailly, Mark Adler, Neil Abcouwer
 * @brief       Compress data using the deflation algorithm
 *
 * Modified version of deflate.c for safety-critical applications.
 * Modifications:
 *   * Provides functions to size an appropriate work buffer.
 *   * Allocates memory for deflate buffers from work buffer.
 *   * Other modifications based on MISRA, P10 safety guidelines.
 * Original file header follows.
 */

/* deflate.c -- compress data using the deflation algorithm
 * Copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/*
 *  ALGORITHM
 *
 *      The "deflation" process depends on being able to identify portions
 *      of the input text which are identical to earlier input (within a
 *      sliding window trailing behind the input currently being processed).
 *
 *      The most straightforward technique turns out to be the fastest for
 *      most input files: try all possible matches and select the longest.
 *      The key feature of this algorithm is that insertions into the string
 *      dictionary are very simple and thus fast, and deletions are avoided
 *      completely. Insertions are performed at each input character, whereas
 *      string matches are performed only when the previous match ends. So it
 *      is preferable to spend more time in matches to allow very fast string
 *      insertions and avoid deletions. The matching algorithm for small
 *      strings is inspired from that of Rabin & Karp. A brute force approach
 *      is used to find longer strings when a small match has been found.
 *      A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
 *      (by Leonid Broukhis).
 *         A previous version of this file used a more sophisticated algorithm
 *      (by Fiala and Greene) which is guaranteed to run in linear amortized
 *      time, but has a larger average cost, uses more memory and is patented.
 *      However the F&G algorithm may be faster for some highly redundant
 *      files if the parameter max_chain_length (described below) is too large.
 *
 *  ACKNOWLEDGEMENTS
 *
 *      The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
 *      I found it in 'freeze' written by Leonid Broukhis.
 *      Thanks to many people for bug reports and testing.
 *
 *  REFERENCES
 *
 *      Deutsch, L.P.,"DEFLATE Compressed Data Format Specification".
 *      Available in http://tools.ietf.org/html/rfc1951
 *
 *      A description of the Rabin and Karp algorithm is given in the book
 *         "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
 *
 *      Fiala,E.R., and Greene,D.H.
 *         Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595
 *
 */

/* @(#) $Id$ */

#include "deflate.h"
#include "zsc_conf_private.h"

const U8 deflate_copyright[] =
   " deflate 1.2.11.f Copyright 1995-2017 Jean-loup Gailly and Mark Adler, Modifications Neil Abcouwer ";
/*
  If you use the zlib library in a product, an acknowledgment is welcome
  in the documentation of your product. If for some reason you cannot
  include such an acknowledgment, I would appreciate that you keep this
  copyright string in the executable of your product.
 */

/* ===========================================================================
 *  Function prototypes.
 */
typedef enum {
    need_more,      /* block not completed, need more input or more output */
    block_done,     /* block flush performed */
    finish_started, /* finish started, need only more output at next deflate */
    finish_done     /* finish done, accept no more input or output */
} block_state;

typedef block_state (*compress_func) (deflate_state *s, ZlibFlush flush);
/* Compression function. Returns the block state after the call. */

ZSC_PRIVATE I32 deflateStateCheck      (z_stream * strm);
ZSC_PRIVATE void slide_hash     (deflate_state *s);
ZSC_PRIVATE void fill_window    (deflate_state *s);
ZSC_PRIVATE block_state deflate_stored (deflate_state *s, ZlibFlush flush);
ZSC_PRIVATE block_state deflate_fast   (deflate_state *s, ZlibFlush flush);
ZSC_PRIVATE block_state deflate_slow   (deflate_state *s, ZlibFlush flush);
ZSC_PRIVATE block_state deflate_rle    (deflate_state *s, ZlibFlush flush);
ZSC_PRIVATE block_state deflate_huff   (deflate_state *s, ZlibFlush flush);
ZSC_PRIVATE void lm_init        (deflate_state *s);
ZSC_PRIVATE void putShortMSB    (deflate_state *s, U32 b);
ZSC_PRIVATE void flush_pending  (z_stream * strm);
ZSC_PRIVATE U32 read_buf   (z_stream * strm, U8 *buf, U32 size);
// Abcouwer ZSC - remove assembly functions
ZSC_PRIVATE U32 longest_match  (deflate_state *s, U32 cur_match);

/* ===========================================================================
 * Local data
 */

#define NIL 0
/* Tail of hash chains */

#  define TOO_FAR 4096
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

/* Values for max_lazy_match, good_match and max_chain_length, depending on
 * the desired pack level (0..9). The values given below have been tuned to
 * exclude worst case performance for pathological files. Better values may be
 * found for specific files.
 */
typedef struct config_s {
   U16 good_length; /* reduce lazy search above this match length */
   U16 max_lazy;    /* do not perform lazy search above this match length */
   U16 nice_length; /* quit search above this match length */
   U16 max_chain;
   compress_func func;
} config;

ZSC_PRIVATE const config configuration_table[10] = {
/*      good lazy nice chain */
/* 0 */ {0,    0,   0,    0, deflate_stored}, /* store only */
/* 1 */ {4,    4,   8,    4, deflate_fast},   /* max speed, no lazy matches */
/* 2 */ {4,    5,  16,    8, deflate_fast},
/* 3 */ {4,    6,  32,   32, deflate_fast},

/* 4 */ {4,    4,  16,   16, deflate_slow},   /* lazy matches */
/* 5 */ {8,   16,  32,   32, deflate_slow},
/* 6 */ {8,   16, 128,  128, deflate_slow},
/* 7 */ {8,   32, 128,  256, deflate_slow},
/* 8 */ {32, 128, 258, 1024, deflate_slow},
/* 9 */ {32, 258, 258, 4096, deflate_slow}};  /* max compression */

/* Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
 * For deflate_fast() (levels <= 3) good is ignored and lazy has a different
 * meaning.
 */

/* rank Z_BLOCK between Z_NO_FLUSH and Z_PARTIAL_FLUSH */
#define RANK(f) (((f) * 2) - ((f) > 4 ? 9 : 0))

/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to UPDATE_HASH are made with consecutive input
 *    characters, so that a running hash key can be computed from the previous
 *    key instead of complete recalculation each time.
 */
#define UPDATE_HASH(s,h,c) \
    ((h) = ((((h)<<((s)->hash_shift)) ^ (c)) & ((s)->hash_mask)))


/* ===========================================================================
 * Insert string str in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * IN  assertion: all calls to INSERT_STRING are made with consecutive input
 *    characters and the first MIN_MATCH bytes of str are valid (except for
 *    the last MIN_MATCH-1 bytes of the input file).
 */
#define INSERT_STRING(s, str, match_head) \
    UPDATE_HASH((s), (s)->ins_h, (s)->window[(str) + (MIN_MATCH-1)]); \
    (match_head) = (s)->prev[(str) & (s)->w_mask] = (s)->head[(s)->ins_h]; \
    (s)->head[(s)->ins_h] = (Pos)(str)

/* ===========================================================================
 * Initialize the hash table (avoiding 64K overflow for 16 bit systems).
 * prev[] will be initialized on the fly.
 */
#define CLEAR_HASH(s) \
    (s)->head[(s)->hash_size-1] = NIL; \
    zmemzero((U8 *)(s)->head, (U32)((s)->hash_size-1)*sizeof(*(s)->head));

/* ===========================================================================
 * Slide the hash table when sliding the window down (could be avoided with 32
 * bit values at the expense of memory usage). We slide even when level == 0 to
 * keep the hash table consistent if we switch back to level > 0 later.
 */
ZSC_PRIVATE void slide_hash(deflate_state *s)
{
    ZSC_ASSERT(s != Z_NULL);

    U32 n, m;
    Pos *p;
    U32 wsize = s->w_size;

    n = s->hash_size;
    p = &s->head[n];
    do {
        p--;
        m = *p;
        *p = (Pos)(m >= wsize ? m - wsize : NIL);
        n--;
    } while (n);
    n = wsize;
    p = &s->prev[n];
    do {
        p--;
        m = *p;
        *p = (Pos)(m >= wsize ? m - wsize : NIL);
        /* If n is not on any hash chain, prev[n] is garbage but
         * its value will never be used.
         */
        n--;
    } while (n);
}

ZSC_PRIVATE void * deflate_get_work_mem(z_stream * strm, U32 items, U32 size)
{
    ZSC_ASSERT(strm != Z_NULL);
    ZSC_ASSERT(items != 0);
    ZSC_ASSERT(size != 0);

    void * new_ptr = Z_NULL;
    U32 bytes = items * size;

    // check overflow
    ZSC_ASSERT3(bytes / items == size, bytes, items, size);

    if (strm->avail_work >= bytes) {
        new_ptr = strm->next_work;
        strm->next_work += bytes;
        strm->avail_work -= bytes;
    }

    return new_ptr;
}


/* ========================================================================= */
ZlibReturn deflateInit_(z_stream * strm, I32 level,
        const U8 * version, I32 stream_size)
{
    return deflateInit2_(strm, level, Z_DEFLATED, MAX_WBITS, DEF_MEM_LEVEL,
                         Z_DEFAULT_STRATEGY, version, stream_size);
    /* To do: ignore strm->next_in if we use it as window */
}

/* ========================================================================= */
ZlibReturn deflateInit2_(z_stream * strm, I32 level,
        ZlibMethod method, I32 windowBits, I32 memLevel,
        ZlibStrategy strategy, const U8 *version, I32 stream_size)
{
    deflate_state *s;
    I32 wrap = 1;
    static const U8 my_version[] = ZLIB_VERSION;

    U16 *overlay;
    /* We overlay pending_buf and d_buf+l_buf. This works since the average
     * output size for (length,distance) codes is <= 24 bits.
     */

    if (version == Z_NULL || version[0] != my_version[0] ||
        stream_size != sizeof(z_stream)) {
        ZSC_WARN3("deflateInit version error: %d %d %d.",
                version == Z_NULL,
                (version == Z_NULL) ? 0 : version[0] != my_version[0],
                stream_size != sizeof(z_stream));
        return Z_VERSION_ERROR;
    }
    if (strm == Z_NULL) {
        ZSC_WARN("deflateInit stream error: stream was null.");
        return Z_STREAM_ERROR;
    }
    U32 work_size = U32_MAX;
    if(strm->next_work == Z_NULL
            || deflateWorkSize2(windowBits, memLevel, &work_size) != Z_OK
            || strm->avail_work < work_size) {
        ZSC_WARN3("deflateInit stream error: %d %d %d.",
                strm->next_work == Z_NULL,
                deflateWorkSize2(windowBits, memLevel, &work_size),
                strm->avail_work < work_size);
        return Z_STREAM_ERROR;
    }

    strm->msg = Z_NULL;

    // Abcouwer ZSC - removed allocation functions - dynamic allocation disallowed

    if (level == Z_DEFAULT_COMPRESSION) {
        level = 6;
    }

    if (windowBits < 0) { /* suppress zlib wrapper */
        wrap = 0;
        windowBits = -windowBits;
    }
    else if (windowBits > 15) {
        wrap = 2;       /* write gzip wrapper instead */
        windowBits -= 16;
    }
    else {
        // windowbits already proper, wrap remains 1
    }

    if (memLevel < 1 || memLevel > MAX_MEM_LEVEL || method != Z_DEFLATED ||
        windowBits < 8 || windowBits > 15 || level < 0 || level > 9 ||
        strategy < 0 || strategy > Z_FIXED || (windowBits == 8 && wrap != 1)) {
        ZSC_WARN5("deflateInit() bad arguments. memLevel:%d method:%d "
                "windowBits:%d level:%d strategy: %d",
                memLevel, method, windowBits, level, strategy);
        return Z_STREAM_ERROR;
    }
    if (windowBits == 8) {
        windowBits = 9;  /* until 256-byte window bug fixed */
    }
    s = (deflate_state *) deflate_get_work_mem(strm, 1, sizeof(deflate_state));
    if (s == Z_NULL) {
        ZSC_WARN("Null deflate-state.");
        return Z_MEM_ERROR;
    }
    strm->state = (struct internal_state *)s;
    s->strm = strm;
    s->status = INIT_STATE;     /* to pass state test in deflateReset() */

    s->wrap = wrap;
    s->gzhead = Z_NULL;
    s->w_bits = (U32)windowBits;
    s->w_size = 1 << s->w_bits;
    s->w_mask = s->w_size - 1;

    s->hash_bits = (U32)memLevel + 7;
    s->hash_size = 1 << s->hash_bits;
    s->hash_mask = s->hash_size - 1;
    s->hash_shift =  ((s->hash_bits+MIN_MATCH-1)/MIN_MATCH);

    s->window = (U8 *) deflate_get_work_mem(strm, s->w_size, 2*sizeof(U8));
    s->prev   = (Pos *)  deflate_get_work_mem(strm, s->w_size, sizeof(Pos));
    s->head   = (Pos *)  deflate_get_work_mem(strm, s->hash_size, sizeof(Pos));

    s->high_water = 0;      /* nothing written to s->window yet */

    s->lit_bufsize = 1 << (memLevel + 6); /* 16K elements by default */

    overlay = (U16 *) deflate_get_work_mem(strm, s->lit_bufsize, sizeof(U16)+2);
    s->pending_buf = (U8 *) overlay;
    s->pending_buf_size = (U32)s->lit_bufsize * (sizeof(U16)+2L);

    if (s->window == Z_NULL || s->prev == Z_NULL || s->head == Z_NULL ||
            s->pending_buf == Z_NULL) {
        ZSC_WARN4("Null pointer from working memory: %d %d %d %d.",
                s->window == Z_NULL, s->prev == Z_NULL,
                s->head == Z_NULL, s->pending_buf == Z_NULL);
        s->status = FINISH_STATE;
        strm->msg = ERR_MSG(Z_MEM_ERROR);
        (void)deflateEnd(strm);
        return Z_MEM_ERROR;
    }
    s->d_buf = overlay + s->lit_bufsize/sizeof(U16);
    s->l_buf = s->pending_buf + (1+sizeof(U16))*s->lit_bufsize;

    s->level = level;
    s->strategy = strategy;
    s->method = (U8)method;

    return deflateReset(strm);
}

/* =========================================================================
 * Check for a valid deflate stream state. Return 0 if ok, 1 if not.
 */
ZSC_PRIVATE I32 deflateStateCheck (z_stream *strm)
{
    deflate_state *s;
    if (strm == Z_NULL) {
        return 1;
    }
    s = strm->state;
    if (s == Z_NULL || s->strm != strm || (s->status != INIT_STATE &&
                                           s->status != GZIP_STATE &&
                                           s->status != EXTRA_STATE &&
                                           s->status != NAME_STATE &&
                                           s->status != COMMENT_STATE &&
                                           s->status != HCRC_STATE &&
                                           s->status != BUSY_STATE &&
                                           s->status != FINISH_STATE)) {
        return 1;
    }
    return 0;
}

/* ========================================================================= */
ZlibReturn deflateSetDictionary (
        z_stream * strm, const U8 *dictionary, U32  dictLength)
{
    deflate_state *s;
    U32 str, n;
    I32 wrap;
    U32 avail;
    const U8 *next;

    if (deflateStateCheck(strm) || dictionary == Z_NULL) {
        return Z_STREAM_ERROR;
    }
    s = strm->state;
    wrap = s->wrap;
    if (wrap == 2 || (wrap == 1 && s->status != INIT_STATE) || s->lookahead) {
        return Z_STREAM_ERROR;
    }

    /* when using zlib wrappers, compute Adler-32 for provided dictionary */
    if (wrap == 1) {
        strm->adler = adler32(strm->adler, dictionary, dictLength);
    }
    s->wrap = 0;                    /* avoid computing Adler-32 in read_buf */

    /* if dictionary would fill window, just replace the history */
    if (dictLength >= s->w_size) {
        if (wrap == 0) {            /* already empty otherwise */
            CLEAR_HASH(s);
            s->strstart = 0;
            s->block_start = 0L;
            s->insert = 0;
        }
        dictionary += dictLength - s->w_size;  /* use the tail */
        dictLength = s->w_size;
    }

    /* insert dictionary into window and hash */
    avail = strm->avail_in;
    next = strm->next_in;
    strm->avail_in = dictLength;
    strm->next_in = (const U8 *)dictionary;
    fill_window(s);
    while (s->lookahead >= MIN_MATCH) {
        str = s->strstart;
        n = s->lookahead - (MIN_MATCH-1);
        do {
            UPDATE_HASH(s, s->ins_h, s->window[str + MIN_MATCH-1]);
            s->prev[str & s->w_mask] = s->head[s->ins_h];
            s->head[s->ins_h] = (Pos)str;
            str++;
            --n;
        } while (n);
        s->strstart = str;
        s->lookahead = MIN_MATCH-1;
        fill_window(s);
    }
    s->strstart += s->lookahead;
    s->block_start = (I32)s->strstart;
    s->insert = s->lookahead;
    s->lookahead = 0;
    s->match_length = s->prev_length = MIN_MATCH-1;
    s->match_available = 0;
    strm->next_in = next;
    strm->avail_in = avail;
    s->wrap = wrap;
    return Z_OK;
}

/* ========================================================================= */
ZlibReturn deflateGetDictionary (z_stream * strm,
        U8 *dictionary, U32 *dictLength)
{
    deflate_state *s;
    U32 len;

    if (deflateStateCheck(strm)) {
        return Z_STREAM_ERROR;
    }
    s = strm->state;
    len = s->strstart + s->lookahead;
    if (len > s->w_size) {
        len = s->w_size;
    }
    if (dictionary != Z_NULL && len) {
        zmemcpy(dictionary, s->window + s->strstart + s->lookahead - len, len);
    }
    if (dictLength != Z_NULL) {
        *dictLength = len;
    }
    return Z_OK;
}

/* ========================================================================= */
ZlibReturn deflateResetKeep(z_stream *strm)
{
    deflate_state *s;

    if (deflateStateCheck(strm)) {
        return Z_STREAM_ERROR;
    }

    strm->total_in = strm->total_out = 0;
    strm->msg = Z_NULL; /* use zfree if we ever allocate msg dynamically */
    strm->data_type = Z_UNKNOWN;

    s = (deflate_state *)strm->state;
    s->pending = 0;
    s->pending_out = s->pending_buf;

    if (s->wrap < 0) {
        s->wrap = -s->wrap; /* was made negative by deflate(..., Z_FINISH) */
    }
    s->status =
            s->wrap == 2 ? GZIP_STATE :
            s->wrap ? INIT_STATE : BUSY_STATE;
    strm->adler =
            s->wrap == 2 ? crc32(0L, Z_NULL, 0) :
                           adler32(0L, Z_NULL, 0);
    s->last_flush = Z_NO_FLUSH;

    _tr_init(s);

    return Z_OK;
}

/* ========================================================================= */
ZlibReturn deflateReset(z_stream * strm)
{
    ZlibReturn ret;

    ret = deflateResetKeep(strm);
    if (ret == Z_OK) {
        lm_init(strm->state);
    }
    return ret;
}

/* ========================================================================= */
ZlibReturn deflateSetHeader(z_stream * strm, gz_header * head)
{
    if (deflateStateCheck(strm) || strm->state->wrap != 2) {
        return Z_STREAM_ERROR;
    }
    strm->state->gzhead = head;
    return Z_OK;
}

/* ========================================================================= */
ZlibReturn deflatePending(z_stream * strm, U32 *pending, I32 *bits)
{
    if (deflateStateCheck(strm)) {
        return Z_STREAM_ERROR;
    }
    if (pending != Z_NULL) {
        *pending = strm->state->pending;
    }
    if (bits != Z_NULL) {
        *bits = strm->state->bi_valid;
    }
    return Z_OK;
}

/* ========================================================================= */
ZlibReturn deflatePrime(z_stream * strm, I32 bits, I32 value)
{
    deflate_state *s;
    I32 put;

    if (deflateStateCheck(strm)) {
        return Z_STREAM_ERROR;
    }
    s = strm->state;
    if ((U8 *)(s->d_buf) < s->pending_out + ((Buf_size + 7) >> 3)) {
        return Z_BUF_ERROR;
    }
    do {
        put = Buf_size - s->bi_valid;
        if (put > bits)
            put = bits;
        s->bi_buf |= (U16)((value & ((1 << put) - 1)) << s->bi_valid);
        s->bi_valid += put;
        _tr_flush_bits(s);
        value >>= put;
        bits -= put;
    } while (bits);
    return Z_OK;
}

/* ========================================================================= */
ZlibReturn deflateParams(z_stream * strm, I32 level, ZlibStrategy strategy)
{
    deflate_state *s;
    compress_func func;

    if (deflateStateCheck(strm)) {
        return Z_STREAM_ERROR;
    }
    s = strm->state;

    if (level == Z_DEFAULT_COMPRESSION) {
        level = 6;
    }
    if (level < 0 || level > 9 || strategy < 0 || strategy > Z_FIXED) {
        return Z_STREAM_ERROR;
    }
    func = configuration_table[s->level].func;

    if ((strategy != s->strategy || func != configuration_table[level].func)
            && s->high_water) {
        /* Flush the last buffer: */
        ZlibReturn err = deflate(strm, Z_BLOCK);
        if (err == Z_STREAM_ERROR) {
            return err;
        }
        if (strm->avail_out == 0) {
            return Z_BUF_ERROR;
        }
    }
    if (s->level != level) {
        if (s->level == 0 && s->matches != 0) {
            if (s->matches == 1) {
                slide_hash(s);
            } else {
                CLEAR_HASH(s);
            }
            s->matches = 0;
        }
        s->level = level;
        s->max_lazy_match   = configuration_table[level].max_lazy;
        s->good_match       = configuration_table[level].good_length;
        s->nice_match       = configuration_table[level].nice_length;
        s->max_chain_length = configuration_table[level].max_chain;
    }
    s->strategy = strategy;
    return Z_OK;
}

/* ========================================================================= */
ZlibReturn deflateTune(z_stream * strm,
        I32 good_length, I32 max_lazy, I32 nice_length, I32 max_chain)
{
    deflate_state *s;

    if (deflateStateCheck(strm)) {
        return Z_STREAM_ERROR;
    }
    ZSC_ASSERT(strm != NULL);
    s = strm->state;
    s->good_match = (U32)good_length;
    s->max_lazy_match = (U32)max_lazy;
    s->nice_match = nice_length;
    s->max_chain_length = (U32)max_chain;
    return Z_OK;
}

/* =========================================================================
 * For the default windowBits of 15 and memLevel of 8, this function returns
 * a close to exact, as well as small, upper bound on the compressed size.
 * They are coded as constants here for a reason--if the #define's are
 * changed, then this function needs to be changed as well.  The return
 * value for 15 and 8 only works for those exact settings.
 *
 * For any setting other than those defaults for windowBits and memLevel,
 * the value returned is a conservative worst case for the maximum expansion
 * resulting from using fixed blocks instead of stored blocks, which deflate
 * can emit on compressed data for some combinations of the parameters.
 *
 * This function could be more sophisticated to provide closer upper bounds for
 * every combination of windowBits and memLevel.  But even the conservative
 * upper bound of about 14% expansion does not seem onerous for output buffer
 * allocation.
 */
U32 deflateBound(z_stream * strm, U32 sourceLen)
{
    deflate_state *s;
    U32 complen, wraplen;

    /* conservative upper bound for compressed data */
    complen = sourceLen +
              ((sourceLen + 7) >> 3) + ((sourceLen + 63) >> 6) + 5;

    /* if can't get parameters, return conservative bound plus zlib wrapper */
    if (deflateStateCheck(strm)) {
        return complen + 6;
    }
    ZSC_ASSERT(strm != NULL);

    /* compute wrapper length */
    s = strm->state;
    switch (s->wrap) {
    case 0:                                 /* raw deflate */
        wraplen = 0;
        break;
    case 1:                                 /* zlib wrapper */
        wraplen = 6 + (s->strstart ? 4 : 0);
        break;
    case 2:                                 /* gzip wrapper */
        wraplen = 18;
        if (s->gzhead != Z_NULL) {          /* user-supplied gzip header */
            U8 *str;
            if (s->gzhead->extra != Z_NULL) {
                wraplen += 2 + s->gzhead->extra_len;
            }
            str = s->gzhead->name;
            if (str != Z_NULL) {
                // Abcouwer ZSC - modified to not use result of assignment
                wraplen++;
                while(*str) {
                    str++;
                    wraplen++;
                }
            }
            str = s->gzhead->comment;
            if (str != Z_NULL) {
                // Abcouwer ZSC - modified to not use result of assignment
                wraplen++;
                while(*str) {
                    str++;
                    wraplen++;
                }
            }
            if (s->gzhead->hcrc) {
                wraplen += 2;
            }
        }
        break;
    default:                                /* for compiler happiness */
        wraplen = 6;
    }

    /* if not default parameters, return conservative bound */
    if (s->w_bits != 15 || s->hash_bits != 8 + 7) {
        return complen + wraplen;
    }

    /* default settings: return tight bound for that case */
    return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) +
           (sourceLen >> 25) + 13 - 6 + wraplen;
}

// find the deflate bound when there is no active stream
ZlibReturn deflateBoundNoStream(U32 sourceLen,
        I32 level, I32 windowBits, I32 memLevel, gz_header * gz_head,
        U32 *size_out)
{
    ZSC_ASSERT(size_out != Z_NULL);
    *size_out = U32_MAX;
    U32 complen = 0;
    U32 wraplen = 0;
    I32 wrap = 1;

    /* conservative upper bound for compressed data */
    complen = sourceLen +
              ((sourceLen + 7) >> 3) + ((sourceLen + 63) >> 6) + 5;

    if (windowBits < 0) { /* suppress zlib wrapper */
        wrap = 0;
        windowBits = -windowBits;
    }
    else if (windowBits > 15) {
        wrap = 2;       /* write gzip wrapper instead */
        windowBits -= 16;
    }
    else {
        // windowbits already proper, wrap remains 1
    }
    // check params
    if (memLevel < 1 || memLevel > MAX_MEM_LEVEL ||
        windowBits < 8 || windowBits > 15 ||
        (windowBits == 8 && wrap != 1)) {
        return Z_STREAM_ERROR;
    }


    /* compute wrapper length */
    switch (wrap) {
    case 0:                                 /* raw deflate */
        wraplen = 0;
        break;
    case 1:                                 /* zlib wrapper */
        wraplen = 6 + 4;
        break;
    case 2:                                 /* gzip wrapper */
        wraplen = 18;
        if (gz_head != Z_NULL) {          /* user-supplied gzip header */
            U8 *str;
            if (gz_head->extra != Z_NULL)
                wraplen += 2 + gz_head->extra_len;
            str = gz_head->name;
            if (str != Z_NULL) {
                // Abcouwer ZSC - modified to not use result of assignment
                wraplen++;
                while(*str) {
                    str++;
                    wraplen++;
                }
            }
            str = gz_head->comment;
            if (str != Z_NULL) {
                // Abcouwer ZSC - modified to not use result of assignment
                wraplen++;
                while(*str) {
                    str++;
                    wraplen++;
                }
            }
            if (gz_head->hcrc) {
                wraplen += 2;
            }
        }
        break;
    default:                                /* for compiler happiness */
        wraplen = 6;
    }

    /* if not default parameters, or not compressing, return conservative bound */
    U32 hash_bits = (U32) memLevel + 7;
    if (windowBits != 15 || hash_bits != 8 + 7 || level == Z_NO_COMPRESSION) {
        *size_out = complen + wraplen;
    } else {
        /* default settings: return tight bound for that case */
        *size_out = sourceLen + (sourceLen >> 12) + (sourceLen >> 14) +
                    (sourceLen >> 25)
                    + 13
                    - 6
                    + wraplen;
    }
    return Z_OK;

}



// given window_bits and mem_level,
// calculate the minimum size of the work buffer required for deflation
// return as output param
// return error if any
ZlibReturn deflateWorkSize2(I32 window_bits, I32 mem_level, U32 *size_out)
{
    ZSC_ASSERT(size_out != Z_NULL);

    // give known value
    *size_out = U32_MAX;

    // properize window_bits
    if (window_bits < 0) { /* suppress zlib wrapper */
        window_bits = -window_bits;
    }
    else if (window_bits > 15) {
        window_bits -= 16;
    }
    else {
        // already proper, no change
    }

    if (window_bits == 8) {
        window_bits = 9; /* until 256-byte window bug fixed */
    }

    if (mem_level < 1 || mem_level > MAX_MEM_LEVEL ||
        window_bits < 8 || window_bits > 15) {
        // FIXME warn
        return Z_STREAM_ERROR;
    }

    // see deflateInit2_() for these allocations
    U32 window_size = 1 << window_bits;
    U32 hash_bits = (U32) mem_level + 7;
    U32 hash_size = 1 << hash_bits;
    U32 lit_bufsize = 1 << (mem_level + 6); /* 16K elements by default */

    U32 size = 0;
    size += sizeof(deflate_state);           // strm->state
    size += window_size * 2 * sizeof(U8);  // strm->state->window
    size += window_size * 2 * sizeof(Pos);   // strm->state->prev
    size += hash_size * sizeof(Pos);         // strm->state->head
    size += lit_bufsize * (sizeof(U16) + 2); // strm->state->pending_buf

    *size_out = size;

    return Z_OK;
}

ZlibReturn deflateWorkSize(U32 *size_out)
{
    return deflateWorkSize2(DEF_WBITS, DEF_MEM_LEVEL, size_out);
}

/* =========================================================================
 * Put a short in the pending buffer. The 16-bit value is put in MSB order.
 * IN assertion: the stream state is correct and there is enough room in
 * pending_buf.
 */
ZSC_PRIVATE void putShortMSB (deflate_state *s, U32 b)
{
    ZSC_ASSERT(s != Z_NULL);
    put_byte(s, (U8)(b >> 8));
    put_byte(s, (U8)(b & 0xff));
}

/* =========================================================================
 * Flush as much pending output as possible. All deflate() output, except for
 * some deflate_stored() output, goes through this function so some
 * applications may wish to modify it to avoid allocating a large
 * strm->next_out buffer and copying into it. (See also read_buf()).
 */
ZSC_PRIVATE void flush_pending(z_stream *strm)
{
    ZSC_ASSERT(strm != Z_NULL);

    U32 len;
    deflate_state *s = strm->state;

    _tr_flush_bits(s);
    len = s->pending;
    if (len > strm->avail_out) {
        len = strm->avail_out;
    }
    if (len == 0) {
        return;
    }

    zmemcpy(strm->next_out, s->pending_out, len);
    strm->next_out  += len;
    s->pending_out  += len;
    strm->total_out += len;
    strm->avail_out -= len;
    s->pending      -= len;
    if (s->pending == 0) {
        s->pending_out = s->pending_buf;
    }
}

/* ===========================================================================
 * Update the header CRC with the bytes s->pending_buf[beg..s->pending - 1].
 */
#define HCRC_UPDATE(beg) \
    do { \
        if (s->gzhead->hcrc && s->pending > (beg)) \
            strm->adler = crc32(strm->adler, s->pending_buf + (beg), \
                                s->pending - (beg)); \
    } while (0)

/* ========================================================================= */
ZlibReturn deflate (z_stream *strm, ZlibFlush flush)
{
    ZlibFlush old_flush; /* value of flush param for previous deflate call */
    deflate_state *s;

    if (deflateStateCheck(strm) || flush > Z_BLOCK || flush < 0) {
        return Z_STREAM_ERROR;
    }
    ZSC_ASSERT(strm != Z_NULL);
    s = strm->state;

    if (strm->next_out == Z_NULL ||
        (strm->avail_in != 0 && strm->next_in == Z_NULL) ||
        (s->status == FINISH_STATE && flush != Z_FINISH)) {
        ERR_RETURN(strm, Z_STREAM_ERROR);
    }
    if (strm->avail_out == 0) {
        ERR_RETURN(strm, Z_BUF_ERROR);
    }

    old_flush = s->last_flush;
    s->last_flush = flush;

    /* Flush as much pending output as possible */
    if (s->pending != 0) {
        flush_pending(strm);
        if (strm->avail_out == 0) {
            /* Since avail_out is 0, deflate will be called again with
             * more output space, but possibly with both pending and
             * avail_in equal to zero. There won't be anything to do,
             * but this is not an error situation so make sure we
             * return OK instead of BUF_ERROR at next call of deflate:
             */
            s->last_flush = -1;
            return Z_OK;
        }

    /* Make sure there is something to do and avoid duplicate consecutive
     * flushes. For repeated and useless calls with Z_FINISH, we keep
     * returning Z_STREAM_END instead of Z_BUF_ERROR.
     */
    } else if (strm->avail_in == 0 && RANK(flush) <= RANK(old_flush) &&
               flush != Z_FINISH) {
        ERR_RETURN(strm, Z_BUF_ERROR);
    } else {
        // no issue so far
    }

    /* User must not provide more input after the first FINISH: */
    if (s->status == FINISH_STATE && strm->avail_in != 0) {
        ERR_RETURN(strm, Z_BUF_ERROR);
    }

    /* Write the header */
    if (s->status == INIT_STATE) {
        /* zlib header */
        U32 header = (Z_DEFLATED + ((s->w_bits-8)<<4)) << 8;
        U32 level_flags;

        if (s->strategy >= Z_HUFFMAN_ONLY || s->level < 2)
            level_flags = 0;
        else if (s->level < 6)
            level_flags = 1;
        else if (s->level == 6)
            level_flags = 2;
        else
            level_flags = 3;
        header |= (level_flags << 6);
        if (s->strstart != 0) header |= PRESET_DICT;
        header += 31 - (header % 31);

        putShortMSB(s, header);

        /* Save the adler32 of the preset dictionary: */
        if (s->strstart != 0) {
            putShortMSB(s, (U32)(strm->adler >> 16));
            putShortMSB(s, (U32)(strm->adler & 0xffff));
        }
        strm->adler = adler32(0L, Z_NULL, 0);
        s->status = BUSY_STATE;

        /* Compression must start with an empty pending buffer */
        flush_pending(strm);
        if (s->pending != 0) {
            s->last_flush = -1;
            return Z_OK;
        }
    }
    if (s->status == GZIP_STATE) {
        /* gzip header */
        strm->adler = crc32(0L, Z_NULL, 0);
        put_byte(s, 31);
        put_byte(s, 139);
        put_byte(s, 8);
        if (s->gzhead == Z_NULL) {
            put_byte(s, 0);
            put_byte(s, 0);
            put_byte(s, 0);
            put_byte(s, 0);
            put_byte(s, 0);
            put_byte(s, s->level == 9 ? 2 :
                     (s->strategy >= Z_HUFFMAN_ONLY || s->level < 2 ?
                      4 : 0));
            put_byte(s, OS_CODE);
            s->status = BUSY_STATE;

            /* Compression must start with an empty pending buffer */
            flush_pending(strm);
            if (s->pending != 0) {
                s->last_flush = -1;
                return Z_OK;
            }
        }
        else {
            put_byte(s, (s->gzhead->text ? 1 : 0) +
                     (s->gzhead->hcrc ? 2 : 0) +
                     (s->gzhead->extra == Z_NULL ? 0 : 4) +
                     (s->gzhead->name == Z_NULL ? 0 : 8) +
                     (s->gzhead->comment == Z_NULL ? 0 : 16)
                     );
            put_byte(s, (U8)(s->gzhead->time & 0xff));
            put_byte(s, (U8)((s->gzhead->time >> 8) & 0xff));
            put_byte(s, (U8)((s->gzhead->time >> 16) & 0xff));
            put_byte(s, (U8)((s->gzhead->time >> 24) & 0xff));
            put_byte(s, s->level == 9 ? 2 :
                     (s->strategy >= Z_HUFFMAN_ONLY || s->level < 2 ?
                      4 : 0));
            put_byte(s, s->gzhead->os & 0xff);
            if (s->gzhead->extra != Z_NULL) {
                put_byte(s, s->gzhead->extra_len & 0xff);
                put_byte(s, (s->gzhead->extra_len >> 8) & 0xff);
            }
            if (s->gzhead->hcrc)
                strm->adler = crc32(strm->adler, s->pending_buf,
                                    s->pending);
            s->gzindex = 0;
            s->status = EXTRA_STATE;
        }
    }
    if (s->status == EXTRA_STATE) {
        if (s->gzhead->extra != Z_NULL) {
            U32 beg = s->pending;   /* start of bytes to update crc */
            U32 left = (s->gzhead->extra_len & 0xffff) - s->gzindex;
            while (s->pending + left > s->pending_buf_size) {
                U32 copy = s->pending_buf_size - s->pending;
                zmemcpy(s->pending_buf + s->pending,
                        s->gzhead->extra + s->gzindex, copy);
                s->pending = s->pending_buf_size;
                HCRC_UPDATE(beg);
                s->gzindex += copy;
                flush_pending(strm);
                if (s->pending != 0) {
                    s->last_flush = -1;
                    return Z_OK;
                }
                beg = 0;
                left -= copy;
            }
            zmemcpy(s->pending_buf + s->pending,
                    s->gzhead->extra + s->gzindex, left);
            s->pending += left;
            HCRC_UPDATE(beg);
            s->gzindex = 0;
        }
        s->status = NAME_STATE;
    }
    if (s->status == NAME_STATE) {
        if (s->gzhead->name != Z_NULL) {
            U32 beg = s->pending;   /* start of bytes to update crc */
            I32 val;
            do {
                if (s->pending == s->pending_buf_size) {
                    HCRC_UPDATE(beg);
                    flush_pending(strm);
                    if (s->pending != 0) {
                        s->last_flush = -1;
                        return Z_OK;
                    }
                    beg = 0;
                }
                val = s->gzhead->name[s->gzindex++];
                put_byte(s, val);
            } while (val != 0);
            HCRC_UPDATE(beg);
            s->gzindex = 0;
        }
        s->status = COMMENT_STATE;
    }
    if (s->status == COMMENT_STATE) {
        if (s->gzhead->comment != Z_NULL) {
            U32 beg = s->pending;   /* start of bytes to update crc */
            I32 val;
            do {
                if (s->pending == s->pending_buf_size) {
                    HCRC_UPDATE(beg);
                    flush_pending(strm);
                    if (s->pending != 0) {
                        s->last_flush = -1;
                        return Z_OK;
                    }
                    beg = 0;
                }
                val = s->gzhead->comment[s->gzindex++];
                put_byte(s, val);
            } while (val != 0);
            HCRC_UPDATE(beg);
        }
        s->status = HCRC_STATE;
    }
    if (s->status == HCRC_STATE) {
        if (s->gzhead->hcrc) {
            if (s->pending + 2 > s->pending_buf_size) {
                flush_pending(strm);
                if (s->pending != 0) {
                    s->last_flush = -1;
                    return Z_OK;
                }
            }
            put_byte(s, (U8)(strm->adler & 0xff));
            put_byte(s, (U8)((strm->adler >> 8) & 0xff));
            strm->adler = crc32(0L, Z_NULL, 0);
        }
        s->status = BUSY_STATE;

        /* Compression must start with an empty pending buffer */
        flush_pending(strm);
        if (s->pending != 0) {
            s->last_flush = -1;
            return Z_OK;
        }
    }

    /* Start a new block or continue the current one.
     */
    if (strm->avail_in != 0 || s->lookahead != 0 ||
        (flush != Z_NO_FLUSH && s->status != FINISH_STATE)) {
        block_state bstate;

        bstate = s->level == 0 ? deflate_stored(s, flush) :
                 s->strategy == Z_HUFFMAN_ONLY ? deflate_huff(s, flush) :
                 s->strategy == Z_RLE ? deflate_rle(s, flush) :
                 (*(configuration_table[s->level].func))(s, flush);

        if (bstate == finish_started || bstate == finish_done) {
            s->status = FINISH_STATE;
        }
        if (bstate == need_more || bstate == finish_started) {
            if (strm->avail_out == 0) {
                s->last_flush = -1; /* avoid BUF_ERROR next call, see above */
            }
            return Z_OK;
            /* If flush != Z_NO_FLUSH && avail_out == 0, the next call
             * of deflate should use the same flush parameter to make sure
             * that the flush is complete. So we don't have to output an
             * empty block here, this will be done at next call. This also
             * ensures that for a very small output buffer, we emit at most
             * one empty block.
             */
        }
        if (bstate == block_done) {
            if (flush == Z_PARTIAL_FLUSH) {
                _tr_align(s);
            } else if (flush != Z_BLOCK) { /* FULL_FLUSH or SYNC_FLUSH */
                _tr_stored_block(s, (U8*)0, 0L, 0);
                /* For a full flush, this empty block will be recognized
                 * as a special marker by inflate_sync().
                 */
                if (flush == Z_FULL_FLUSH) {
                    CLEAR_HASH(s);             /* forget history */
                    if (s->lookahead == 0) {
                        s->strstart = 0;
                        s->block_start = 0L;
                        s->insert = 0;
                    }
                }
            } else { // flush == Z_BLOCK
                // do not align to byte boundary
            }
            flush_pending(strm);
            if (strm->avail_out == 0) {
              s->last_flush = -1; /* avoid BUF_ERROR at next call, see above */
              return Z_OK;
            }
        }
    }

    if (flush != Z_FINISH) {
        return Z_OK;
    }
    if (s->wrap <= 0) {
        return Z_STREAM_END;
    }

    /* Write the trailer */
    if (s->wrap == 2) {
        put_byte(s, (U8)(strm->adler & 0xff));
        put_byte(s, (U8)((strm->adler >> 8) & 0xff));
        put_byte(s, (U8)((strm->adler >> 16) & 0xff));
        put_byte(s, (U8)((strm->adler >> 24) & 0xff));
        put_byte(s, (U8)(strm->total_in & 0xff));
        put_byte(s, (U8)((strm->total_in >> 8) & 0xff));
        put_byte(s, (U8)((strm->total_in >> 16) & 0xff));
        put_byte(s, (U8)((strm->total_in >> 24) & 0xff));
    }
    else
    {
        putShortMSB(s, (U32)(strm->adler >> 16));
        putShortMSB(s, (U32)(strm->adler & 0xffff));
    }
    flush_pending(strm);
    /* If avail_out is zero, the application will call deflate again
     * to flush the rest.
     */
    if (s->wrap > 0) {
        s->wrap = -s->wrap; /* write the trailer only once! */
    }
    return s->pending != 0 ? Z_OK : Z_STREAM_END;
}

/* ========================================================================= */
ZlibReturn deflateEnd (z_stream * strm)
{
    I32 status;

    if (deflateStateCheck(strm)) {
        return Z_STREAM_ERROR;
    }
    ZSC_ASSERT(strm != Z_NULL);

    status = strm->state->status;

    // Abcouwer ZSC - no dynamic allocation, removed frees

    strm->state = Z_NULL;

    return status == BUSY_STATE ? Z_DATA_ERROR : Z_OK;
}

// Abcouwer ZSC - removed deflateCopy
// copying then doing allocs will spool more memory from the work buffer.

/* ===========================================================================
 * Read a new buffer from the current input stream, update the adler32
 * and total number of bytes read.  All deflate() input goes through
 * this function so some applications may wish to modify it to avoid
 * allocating a large strm->next_in buffer and copying from it.
 * (See also flush_pending()).
 */
ZSC_PRIVATE U32 read_buf(z_stream * strm, U8 *buf, U32 size)
{
    ZSC_ASSERT(strm != Z_NULL);
    ZSC_ASSERT(buf != Z_NULL);

    U32 len = strm->avail_in;

    if (len > size) len = size;
    if (len == 0) {
        return 0;
    }

    strm->avail_in  -= len;

    zmemcpy(buf, strm->next_in, len);
    if (strm->state->wrap == 1) {
        strm->adler = adler32(strm->adler, buf, len);
    }
    else if (strm->state->wrap == 2) {
        strm->adler = crc32(strm->adler, buf, len);
    }
    else {
        // no check to maintain
    }
    strm->next_in  += len;
    strm->total_in += len;

    return len;
}

/* ===========================================================================
 * Initialize the "longest match" routines for a new zlib stream
 */
ZSC_PRIVATE void lm_init (deflate_state *s)
{
    ZSC_ASSERT(s != Z_NULL);

    s->window_size = (U32)2L*s->w_size;

    CLEAR_HASH(s);

    /* Set the default configuration parameters:
     */
    s->max_lazy_match   = configuration_table[s->level].max_lazy;
    s->good_match       = configuration_table[s->level].good_length;
    s->nice_match       = configuration_table[s->level].nice_length;
    s->max_chain_length = configuration_table[s->level].max_chain;

    s->strstart = 0;
    s->block_start = 0L;
    s->lookahead = 0;
    s->insert = 0;
    s->match_length = s->prev_length = MIN_MATCH-1;
    s->match_available = 0;
    s->ins_h = 0;

    // Abcouwer ZSC - remove assembly functions
}

/* ===========================================================================
 * Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to prev_length are discarded,
 * in which case the result is equal to prev_length and match_start is
 * garbage.
 * IN assertions: cur_match is the head of the hash chain for the current
 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
 * OUT assertion: the match length is not greater than s->lookahead.
 */
// Abcouwer ZSC - remove assembly functions
ZSC_PRIVATE U32 longest_match( deflate_state *s, U32 cur_match)
{
    ZSC_ASSERT(s != Z_NULL);

    U32 chain_length = s->max_chain_length;/* max hash chain length */
    register U8 *scan = s->window + s->strstart; /* current string */
    register U8 *match;                      /* matched string */
    register I32 len;                           /* length of current match */
    I32 best_len = (I32)s->prev_length;         /* best match length so far */
    I32 nice_match = s->nice_match;             /* stop if match long enough */
    U32 limit = s->strstart > (U32)MAX_DIST(s) ?
        s->strstart - (U32)MAX_DIST(s) : NIL;
    /* Stop when cur_match becomes <= limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */
    Pos *prev = s->prev;
    U32 wmask = s->w_mask;

    register U8 *strend = s->window + s->strstart + MAX_MATCH;
    register U8 scan_end1  = scan[best_len-1];
    register U8 scan_end   = scan[best_len];

    /* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
     * It is easy to get rid of this optimization if necessary.
     */
    // Assert: "Code too clever"
    ZSC_COMPILE_ASSERT(MAX_MATCH == 258, bad_max_match);
    ZSC_ASSERT1(s->hash_bits >= 8, s->hash_bits);

    /* Do not waste too much time if we already have a good match: */
    if (s->prev_length >= s->good_match) {
        chain_length >>= 2;
    }
    /* Do not look for matches beyond the end of the input. This is necessary
     * to make deflate deterministic.
     */
    if ((U32)nice_match > s->lookahead) nice_match = (I32)s->lookahead;

    // Assert: "need lookahead"
    ZSC_ASSERT3((U32)s->strstart <= s->window_size-MIN_LOOKAHEAD,
            (U32)s->strstart, s->window_size, MIN_LOOKAHEAD);


    do {
        // Assert: cur_match < s->strstart, "no future"
        ZSC_ASSERT2(cur_match < s->strstart, cur_match, s->strstart);
        match = s->window + cur_match;

        /* Skip to next match if the match length cannot increase
         * or if the match length is less than 2.  Note that the checks below
         * for insufficient lookahead only occur occasionally for performance
         * reasons.  Therefore uninitialized memory will be accessed, and
         * conditional jumps will be made that depend on those values.
         * However the length of the match is limited to the lookahead, so
         * the output of deflate is not affected by the uninitialized values.
         */


        // Abcouwer ZSC - modified to comply with MISRA C 13.5
        // match no longer modified as side effect in right hand operand of ||

        if (match[best_len]   != scan_end  ||
            match[best_len-1] != scan_end1 ||
            *match            != *scan     ||
            match[1]          != scan[1]) {
            continue;
        }

        /* The check at best_len-1 can be removed because it will be made
         * again later. (This heuristic is not always a win.)
         * It is not necessary to compare scan[2] and match[2] since they
         * are always equal when the other bytes match, given that
         * the hash keys are equal and that HASH_BITS >= 8.
         */
        scan += 2; match += 2;
        // Assert: "match[2]?"
        ZSC_ASSERT2(*scan == *match, *scan, *match);

        /* We check for insufficient lookahead only every 8th comparison;
         * the 256th check will be made at strstart+258. */
        // Abcouwer ZSC - Modified to scan by to follow MISRA 13.5
        do {
            scan++;
            match++;
        } while (*scan == *match && scan < strend);

        // Assert: "wild scan"
        ZSC_ASSERT3(scan <= s->window+(U32)(s->window_size-1),
                scan, s->window, s->window_size);

        len = MAX_MATCH - (I32)(strend - scan);
        scan = strend - MAX_MATCH;


        if (len > best_len) {
            s->match_start = cur_match;
            best_len = len;
            if (len >= nice_match) {
                break;
            }
            scan_end1  = scan[best_len-1];
            scan_end   = scan[best_len];
        }
        // assert no underflow
        ZSC_ASSERT(chain_length>0);
        chain_length--;
    } while ((cur_match = prev[cur_match & wmask]) > limit
             && chain_length != 0);

    if ((U32)best_len <= s->lookahead) {
        return (U32)best_len;
    }
    return s->lookahead;
}

// Abcouwer ZSC - removed DEBUG-only definition of check_match

/* ===========================================================================
 * Fill the window when the lookahead becomes insufficient.
 * Updates strstart and lookahead.
 *
 * IN assertion: lookahead < MIN_LOOKAHEAD
 * OUT assertions: strstart <= window_size-MIN_LOOKAHEAD
 *    At least one byte has been read, or avail_in == 0; reads are
 *    performed for at least two bytes (required for the zip translate_eol
 *    option -- not supported here).
 */
ZSC_PRIVATE void fill_window(deflate_state *s)
{
    ZSC_ASSERT(s != Z_NULL);

    U32 n;
    U32 more;    /* Amount of free space at the end of the window. */
    U32 wsize = s->w_size;

    // Assert: "already enough lookahead"
    ZSC_ASSERT2(s->lookahead < MIN_LOOKAHEAD, s->lookahead, MIN_LOOKAHEAD);

    do {
        more = s->window_size - s->lookahead - s->strstart;

        /* Deal with !@#$% 64K limit: */
        if (sizeof(int) <= 2) {
            if (more == 0 && s->strstart == 0 && s->lookahead == 0) {
                more = wsize;
            } else if (more == (unsigned)(-1)) {
                /* Very unlikely, but possible on 16 bit machine if
                 * strstart == 0 && lookahead == 1 (input done a byte at time)
                 */
                more--;
            } else {
                // do nothing
            }
        }

        /* If the window is almost full and there is insufficient lookahead,
         * move the upper half to the lower one to make room in the upper half.
         */
        if (s->strstart >= wsize+MAX_DIST(s)) {
            zmemcpy(s->window, s->window+wsize, wsize - more);
            s->match_start -= wsize;
            s->strstart    -= wsize; /* we now have strstart >= MAX_DIST */
            s->block_start -= (I32) wsize;
            slide_hash(s);
            more += wsize;
        }
        if (s->strm->avail_in == 0) {
            break;
        }

        /* If there was no sliding:
         *    strstart <= WSIZE+MAX_DIST-1 && lookahead <= MIN_LOOKAHEAD - 1 &&
         *    more == window_size - lookahead - strstart
         * => more >= window_size - (MIN_LOOKAHEAD-1 + WSIZE + MAX_DIST-1)
         * => more >= window_size - 2*WSIZE + 2
         * In the BIG_MEM or MMAP case (not yet supported),
         *   window_size == input_size + MIN_LOOKAHEAD  &&
         *   strstart + s->lookahead <= input_size => more >= MIN_LOOKAHEAD.
         * Otherwise, window_size == 2*WSIZE so more >= 2.
         * If there was sliding, more >= WSIZE. So in all cases, more >= 2.
         */
        // Assert: "more < 2"
        ZSC_ASSERT1(more >= 2, more);

        n = read_buf(s->strm, s->window + s->strstart + s->lookahead, more);
        s->lookahead += n;

        /* Initialize the hash value now that we have some input: */
        if (s->lookahead + s->insert >= MIN_MATCH) {
            U32 str = s->strstart - s->insert;
            s->ins_h = s->window[str];
            UPDATE_HASH(s, s->ins_h, s->window[str + 1]);
            // if MIN_MATCH != 3,  Call UPDATE_HASH() MIN_MATCH-3 more times
            ZSC_COMPILE_ASSERT(MIN_MATCH == 3, bad_min_match);
            while (s->insert) {
                UPDATE_HASH(s, s->ins_h, s->window[str + MIN_MATCH-1]);
                s->prev[str & s->w_mask] = s->head[s->ins_h];
                s->head[s->ins_h] = (Pos)str;
                str++;
                s->insert--;
                if (s->lookahead + s->insert < MIN_MATCH)
                    break;
            }
        }
        /* If the whole input has less than MIN_MATCH bytes, ins_h is garbage,
         * but this is not important since only literal bytes will be emitted.
         */

    } while (s->lookahead < MIN_LOOKAHEAD && s->strm->avail_in != 0);

    /* If the WIN_INIT bytes after the end of the current data have never been
     * written, then zero those bytes in order to avoid memory check reports of
     * the use of uninitialized (or uninitialised as Julian writes) bytes by
     * the longest match routines.  Update the high water mark for the next
     * time through here.  WIN_INIT is set to MAX_MATCH since the longest match
     * routines allow scanning to strstart + MAX_MATCH, ignoring lookahead.
     */
    if (s->high_water < s->window_size) {
        U32 curr = s->strstart + (U32)(s->lookahead);
        U32 init;

        if (s->high_water < curr) {
            /* Previous high water mark below current data -- zero WIN_INIT
             * bytes or up to end of window, whichever is less.
             */
            init = s->window_size - curr;
            if (init > WIN_INIT)
                init = WIN_INIT;
            zmemzero(s->window + curr, (U32)init);
            s->high_water = curr + init;
        } else if (s->high_water < (U32)curr + WIN_INIT) {
            /* High water mark at or above current data, but below current data
             * plus WIN_INIT -- zero out to current data plus WIN_INIT, or up
             * to end of window, whichever is less.
             */
            init = (U32)curr + WIN_INIT - s->high_water;
            if (init > s->window_size - s->high_water)
                init = s->window_size - s->high_water;
            zmemzero(s->window + s->high_water, (U32)init);
            s->high_water += init;
        } else {
            // high water is above, no action required
        }
    }

    // Assert: "not enough room for search"
    ZSC_ASSERT3((U32)s->strstart <= s->window_size - MIN_LOOKAHEAD,
            s->strstart, s->window_size, MIN_LOOKAHEAD);
}

/* ===========================================================================
 * Flush the current block, with given end-of-file flag.
 * IN assertion: strstart is set to the end of the current match.
 */
#define FLUSH_BLOCK_ONLY(s, last) { \
   _tr_flush_block((s), ((s)->block_start >= 0L ? \
                   (U8*)&(s)->window[(U32)(s)->block_start] : \
                   (U8*)Z_NULL), \
                (U32)((I32)(s)->strstart - (s)->block_start), \
                (last)); \
   (s)->block_start = (s)->strstart; \
   flush_pending((s)->strm); \
}

/* Same but force premature exit if necessary. */
#define FLUSH_BLOCK(s, last) { \
   FLUSH_BLOCK_ONLY((s), (last)); \
   if ((s)->strm->avail_out == 0) return (last) ? finish_started : need_more; \
}

/* Maximum stored block length in deflate format (not including header). */
#define MAX_STORED 65535

/* ===========================================================================
 * Copy without compression as much as possible from the input stream, return
 * the current block state.
 *
 * In case deflateParams() is used to later switch to a non-zero compression
 * level, s->matches (otherwise unused when storing) keeps track of the number
 * of hash table slides to perform. If s->matches is 1, then one hash table
 * slide will be done when switching. If s->matches is 2, the maximum value
 * allowed here, then the hash table will be cleared, since two or more slides
 * is the same as a clear.
 *
 * deflate_stored() is written to minimize the number of times an input byte is
 * copied. It is most efficient with large input and output buffers, which
 * maximizes the opportunities to have a single copy from next_in to next_out.
 */
ZSC_PRIVATE block_state deflate_stored(deflate_state *s, ZlibFlush flush)
{
    ZSC_ASSERT(s != Z_NULL);

    /* Smallest worthy block size when not flushing or finishing. By default
     * this is 32K. This can be as small as 507 bytes for memLevel == 1. For
     * large input and output buffers, the stored block size will be larger.
     */
    U32 min_block = ZMIN(s->pending_buf_size - 5, s->w_size);

    /* Copy as many min_block or larger stored blocks directly to next_out as
     * possible. If flushing, copy the remaining available input to next_out as
     * stored blocks, if there is enough space.
     */
    U32 len, left, have, last = 0;
    U32 used = s->strm->avail_in;
    do {
        /* Set len to the maximum size block that we can copy directly with the
         * available input data and output space. Set left to how much of that
         * would be copied from what's left in the window.
         */
        len = MAX_STORED;       /* maximum deflate stored block length */
        have = (s->bi_valid + 42) >> 3;         /* number of header bytes */
        if (s->strm->avail_out < have)          /* need room for header */
            break;
            /* maximum stored block length that will fit in avail_out: */
        have = s->strm->avail_out - have;
        left = s->strstart - s->block_start;    /* bytes left in window */
        if (len > (U32)left + s->strm->avail_in)
            len = left + s->strm->avail_in;     /* limit len to the input */
        if (len > have)
            len = have;                         /* limit len to the output */

        /* If the stored block would be less than min_block in length, or if
         * unable to copy all of the available input when flushing, then try
         * copying to the window and the pending buffer instead. Also don't
         * write an empty block when flushing -- deflate() does that.
         */
        if (len < min_block && ((len == 0 && flush != Z_FINISH) ||
                                flush == Z_NO_FLUSH ||
                                len != left + s->strm->avail_in))
            break;

        /* Make a dummy stored block in pending to get the header bytes,
         * including any pending bits. This also updates the debugging counts.
         */
        last = flush == Z_FINISH && len == left + s->strm->avail_in ? 1 : 0;
        _tr_stored_block(s, (U8*)0, 0L, last);

        /* Replace the lengths in the dummy stored block with len. */
        s->pending_buf[s->pending - 4] = len;
        s->pending_buf[s->pending - 3] = len >> 8;
        s->pending_buf[s->pending - 2] = ~len;
        s->pending_buf[s->pending - 1] = ~len >> 8;

        /* Write the stored block header bytes. */
        flush_pending(s->strm);

        /* Copy uncompressed bytes from the window to next_out. */
        if (left) {
            if (left > len)
                left = len;
            zmemcpy(s->strm->next_out, s->window + s->block_start, left);
            s->strm->next_out += left;
            s->strm->avail_out -= left;
            s->strm->total_out += left;
            s->block_start += left;
            len -= left;
        }

        /* Copy uncompressed bytes directly from next_in to next_out, updating
         * the check value.
         */
        if (len) {
            (void)read_buf(s->strm, s->strm->next_out, len);
            s->strm->next_out += len;
            s->strm->avail_out -= len;
            s->strm->total_out += len;
        }
    } while (last == 0);

    /* Update the sliding window with the last s->w_size bytes of the copied
     * data, or append all of the copied data to the existing window if less
     * than s->w_size bytes were copied. Also update the number of bytes to
     * insert in the hash tables, in the event that deflateParams() switches to
     * a non-zero compression level.
     */
    used -= s->strm->avail_in;      /* number of input bytes directly copied */
    if (used) {
        /* If any input was used, then no unused input remains in the window,
         * therefore s->block_start == s->strstart.
         */
        if (used >= s->w_size) {    /* supplant the previous history */
            s->matches = 2;         /* clear hash */
            zmemcpy(s->window, s->strm->next_in - s->w_size, s->w_size);
            s->strstart = s->w_size;
        }
        else {
            if (s->window_size - s->strstart <= used) {
                /* Slide the window down. */
                s->strstart -= s->w_size;
                zmemcpy(s->window, s->window + s->w_size, s->strstart);
                if (s->matches < 2) {
                    s->matches++;   /* add a pending slide_hash() */
                }
            } else {
                // no sliding needed
            }
            zmemcpy(s->window + s->strstart, s->strm->next_in - used, used);
            s->strstart += used;
        }
        s->block_start = s->strstart;
        s->insert += ZMIN(used, s->w_size - s->insert);
    }
    if (s->high_water < s->strstart)
        s->high_water = s->strstart;

    /* If the last block was written to next_out, then done. */
    if (last)
        return finish_done;

    /* If flushing and all input has been consumed, then done. */
    if (flush != Z_NO_FLUSH && flush != Z_FINISH &&
        s->strm->avail_in == 0 && (I32)s->strstart == s->block_start)
        return block_done;

    /* Fill the window with any remaining input. */
    have = s->window_size - s->strstart - 1;
    if (s->strm->avail_in > have && s->block_start >= (I32)s->w_size) {
        /* Slide the window down. */
        s->block_start -= s->w_size;
        s->strstart -= s->w_size;
        zmemcpy(s->window, s->window + s->w_size, s->strstart);
        if (s->matches < 2)
            s->matches++;           /* add a pending slide_hash() */
        have += s->w_size;          /* more space now */
    }
    if (have > s->strm->avail_in)
        have = s->strm->avail_in;
    if (have) {
        (void)read_buf(s->strm, s->window + s->strstart, have);
        s->strstart += have;
    }
    if (s->high_water < s->strstart)
        s->high_water = s->strstart;

    /* There was not enough avail_out to write a complete worthy or flushed
     * stored block to next_out. Write a stored block to pending instead, if we
     * have enough input for a worthy block, or if flushing and there is enough
     * room for the remaining input as a stored block in the pending buffer.
     */
    have = (s->bi_valid + 42) >> 3;         /* number of header bytes */
        /* maximum stored block length that will fit in pending: */
    have = ZMIN(s->pending_buf_size - have, MAX_STORED);
    min_block = ZMIN(have, s->w_size);
    left = s->strstart - s->block_start;
    if (left >= min_block ||
        ((left || flush == Z_FINISH) && flush != Z_NO_FLUSH &&
         s->strm->avail_in == 0 && left <= have)) {
        len = ZMIN(left, have);
        last = flush == Z_FINISH && s->strm->avail_in == 0 &&
               len == left ? 1 : 0;
        _tr_stored_block(s, (U8*)s->window + s->block_start, len, last);
        s->block_start += len;
        flush_pending(s->strm);
    }

    /* We've done all we can with the available input and output. */
    return last ? finish_started : need_more;
}

/* ===========================================================================
 * Compress as much as possible from the input stream, return the current
 * block state.
 * This function does not perform lazy evaluation of matches and inserts
 * new strings in the dictionary only for unmatched strings or for short
 * matches. It is used only for the fast compression options.
 */
ZSC_PRIVATE block_state deflate_fast(deflate_state *s, ZlibFlush flush)
{
    ZSC_ASSERT(s != Z_NULL);

    U32 hash_head;       /* head of the hash chain */
    I32 bflush;           /* set if current block must be flushed */

    for (;;) {
        /* Make sure that we always have enough lookahead, except
         * at the end of the input file. We need MAX_MATCH bytes
         * for the next match, plus MIN_MATCH bytes to insert the
         * string following the next match.
         */
        if (s->lookahead < MIN_LOOKAHEAD) {
            fill_window(s);
            if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) {
                return need_more;
            }
            if (s->lookahead == 0) break; /* flush the current block */
        }

        /* Insert the string window[strstart .. strstart+2] in the
         * dictionary, and set hash_head to the head of the hash chain:
         */
        hash_head = NIL;
        if (s->lookahead >= MIN_MATCH) {
            INSERT_STRING(s, s->strstart, hash_head);
        }

        /* Find the longest match, discarding those <= prev_length.
         * At this point we have always match_length < MIN_MATCH
         */
        if (hash_head != NIL && s->strstart - hash_head <= MAX_DIST(s)) {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input file).
             */
            s->match_length = longest_match (s, hash_head);
            /* longest_match() sets match_start */
        }
        if (s->match_length >= MIN_MATCH) {
            // Abcouwer ZSC - remove check_match() which is defined
            //                as nothing when not debugging

            _tr_tally_dist(s, s->strstart - s->match_start,
                           s->match_length - MIN_MATCH, bflush);

            s->lookahead -= s->match_length;

            /* Insert new strings in the hash table only if the match length
             * is not too large. This saves time but degrades compression.
             */
            if (s->match_length <= s->max_insert_length
                    && s->lookahead >= MIN_MATCH) {
                s->match_length--; /* string at strstart already in table */
                do {
                    s->strstart++;
                    INSERT_STRING(s, s->strstart, hash_head);
                    /* strstart never exceeds WSIZE-MAX_MATCH, so there are
                     * always MIN_MATCH bytes ahead. */
                    s->match_length--;
                } while (s->match_length != 0);
                s->strstart++;
            } else
            {
                s->strstart += s->match_length;
                s->match_length = 0;
                s->ins_h = s->window[s->strstart];
                UPDATE_HASH(s, s->ins_h, s->window[s->strstart+1]);
                // if MIN_MATCH != 3,  Call UPDATE_HASH() MIN_MATCH-3 more times
                ZSC_COMPILE_ASSERT(MIN_MATCH == 3, bad_min_match);
                /* If lookahead < MIN_MATCH, ins_h is garbage, but it does not
                 * matter since it will be recomputed at next deflate call.
                 */
            }
        } else {
            /* No match, output a literal byte */
            _tr_tally_lit (s, s->window[s->strstart], bflush);
            s->lookahead--;
            s->strstart++;
        }
        if (bflush) FLUSH_BLOCK(s, 0);
    }
    s->insert = s->strstart < MIN_MATCH-1 ? s->strstart : MIN_MATCH-1;
    if (flush == Z_FINISH) {
        FLUSH_BLOCK(s, 1);
        return finish_done;
    }
    if (s->last_lit)
        FLUSH_BLOCK(s, 0);
    return block_done;
}

/* ===========================================================================
 * Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no better match at the next window position.
 */
ZSC_PRIVATE block_state deflate_slow(deflate_state *s, ZlibFlush flush)
{
    ZSC_ASSERT(s != Z_NULL);

    U32 hash_head;          /* head of hash chain */
    I32 bflush;              /* set if current block must be flushed */

    /* Process the input block. */
    for (;;) {
        /* Make sure that we always have enough lookahead, except
         * at the end of the input file. We need MAX_MATCH bytes
         * for the next match, plus MIN_MATCH bytes to insert the
         * string following the next match.
         */
        if (s->lookahead < MIN_LOOKAHEAD) {
            fill_window(s);
            if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) {
                return need_more;
            }
            if (s->lookahead == 0) break; /* flush the current block */
        }

        /* Insert the string window[strstart .. strstart+2] in the
         * dictionary, and set hash_head to the head of the hash chain:
         */
        hash_head = NIL;
        if (s->lookahead >= MIN_MATCH) {
            INSERT_STRING(s, s->strstart, hash_head);
        }

        /* Find the longest match, discarding those <= prev_length.
         */
        s->prev_length = s->match_length;
        s->prev_match = s->match_start;
        s->match_length = MIN_MATCH-1;

        if (hash_head != NIL && s->prev_length < s->max_lazy_match &&
            s->strstart - hash_head <= MAX_DIST(s)) {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input file).
             */
            s->match_length = longest_match (s, hash_head);
            /* longest_match() sets match_start */

            if (s->match_length <= 5 && (s->strategy == Z_FILTERED
#if TOO_FAR <= 32767
                || (s->match_length == MIN_MATCH &&
                    s->strstart - s->match_start > TOO_FAR)
#endif
                )) {

                /* If prev_match is also MIN_MATCH, match_start is garbage
                 * but we will ignore the current match anyway.
                 */
                s->match_length = MIN_MATCH-1;
            }
        }
        /* If there was a match at the previous step and the current
         * match is not better, output the previous match:
         */
        if (s->prev_length >= MIN_MATCH && s->match_length <= s->prev_length) {
            U32 max_insert = s->strstart + s->lookahead - MIN_MATCH;
            /* Do not insert strings in hash table beyond this. */

            // Abcouwer ZSC - remove check_match() which is defined
            //                as nothing when not debugging

            _tr_tally_dist(s, s->strstart -1 - s->prev_match,
                           s->prev_length - MIN_MATCH, bflush);

            /* Insert in hash table all strings up to the end of the match.
             * strstart-1 and strstart are already inserted. If there is not
             * enough lookahead, the last two strings are not inserted in
             * the hash table.
             */
            s->lookahead -= s->prev_length-1;
            s->prev_length -= 2;
            do {
                s->strstart++;
                if (s->strstart <= max_insert) {
                    INSERT_STRING(s, s->strstart, hash_head);
                }
                s->prev_length--;
            } while (s->prev_length != 0);
            s->match_available = 0;
            s->match_length = MIN_MATCH-1;
            s->strstart++;

            if (bflush) FLUSH_BLOCK(s, 0);

        } else if (s->match_available) {
            /* If there was no match at the previous position, output a
             * single literal. If there was a match but the current match
             * is longer, truncate the previous match to a single literal.
             */
            _tr_tally_lit(s, s->window[s->strstart-1], bflush);
            if (bflush) {
                FLUSH_BLOCK_ONLY(s, 0);
            }
            s->strstart++;
            s->lookahead--;
            if (s->strm->avail_out == 0) return need_more;
        } else {
            /* There is no previous match to compare with, wait for
             * the next step to decide.
             */
            s->match_available = 1;
            s->strstart++;
            s->lookahead--;
        }
    }
    // Assert: "no flush?"
    ZSC_ASSERT2(flush != Z_NO_FLUSH, flush, Z_NO_FLUSH);
    if (s->match_available) {
        _tr_tally_lit(s, s->window[s->strstart-1], bflush);
        s->match_available = 0;
    }
    s->insert = s->strstart < MIN_MATCH-1 ? s->strstart : MIN_MATCH-1;
    if (flush == Z_FINISH) {
        FLUSH_BLOCK(s, 1);
        return finish_done;
    }
    if (s->last_lit)
        FLUSH_BLOCK(s, 0);
    return block_done;
}

/* ===========================================================================
 * For Z_RLE, simply look for runs of bytes, generate matches only of distance
 * one.  Do not maintain a hash table.  (It will be regenerated if this run of
 * deflate switches away from Z_RLE.)
 */
ZSC_PRIVATE block_state deflate_rle(deflate_state *s, ZlibFlush flush)
{
    ZSC_ASSERT(s != Z_NULL);

    I32 bflush;             /* set if current block must be flushed */
    U32 prev;              /* byte at distance one to match */
    U8 *scan, *strend;   /* scan goes up to strend for length of run */

    for (;;) {
        /* Make sure that we always have enough lookahead, except
         * at the end of the input file. We need MAX_MATCH bytes
         * for the longest run, plus one for the unrolled loop.
         */
        if (s->lookahead <= MAX_MATCH) {
            fill_window(s);
            if (s->lookahead <= MAX_MATCH
                    && flush == Z_NO_FLUSH) {
                return need_more;
            }
            if (s->lookahead == 0) break; /* flush the current block */
        }

        /* See how many times the previous byte repeats */
        s->match_length = 0;
        if (s->lookahead >= MIN_MATCH && s->strstart > 0) {
            scan = s->window + s->strstart - 1;
            prev = *scan;
            if (prev == scan[1] && prev == scan[2] && prev == scan[3]) {
                scan+=3;
                strend = s->window + s->strstart + MAX_MATCH;
                // Abcouwer ZSC - Modified scan to follow MISRA 13.5
                do {
                    scan++;
                } while (prev == *scan && scan < strend);
                s->match_length = MAX_MATCH - (U32)(strend - scan);
                if (s->match_length > s->lookahead)
                    s->match_length = s->lookahead;
            }
            // Assert: "wild scan"
            ZSC_ASSERT3(scan <= s->window+(U32)(s->window_size-1),
                    scan, s->window, s->window_size);
        }

        /* Emit match if have run of MIN_MATCH or longer, else emit literal */
        if (s->match_length >= MIN_MATCH) {
            // Abcouwer ZSC - remove check_match() which is defined
            //                as nothing when not debugging

            _tr_tally_dist(s, 1, s->match_length - MIN_MATCH, bflush);

            s->lookahead -= s->match_length;
            s->strstart += s->match_length;
            s->match_length = 0;
        } else {
            /* No match, output a literal byte */
            _tr_tally_lit (s, s->window[s->strstart], bflush);
            s->lookahead--;
            s->strstart++;
        }
        if (bflush) FLUSH_BLOCK(s, 0);
    }
    s->insert = 0;
    if (flush == Z_FINISH) {
        FLUSH_BLOCK(s, 1);
        return finish_done;
    }
    if (s->last_lit)
        FLUSH_BLOCK(s, 0);
    return block_done;
}

/* ===========================================================================
 * For Z_HUFFMAN_ONLY, do not look for matches.  Do not maintain a hash table.
 * (It will be regenerated if this run of deflate switches away from Huffman.)
 */
ZSC_PRIVATE block_state deflate_huff(deflate_state *s, ZlibFlush flush)
{
    ZSC_ASSERT(s != Z_NULL);

    I32 bflush;             /* set if current block must be flushed */

    for (;;) {
        /* Make sure that we have a literal to write. */
        if (s->lookahead == 0) {
            fill_window(s);
            if (s->lookahead == 0) {
                if (flush == Z_NO_FLUSH)
                    return need_more;
                break;      /* flush the current block */
            }
        }

        /* Output a literal byte */
        s->match_length = 0;
        _tr_tally_lit (s, s->window[s->strstart], bflush);
        s->lookahead--;
        s->strstart++;
        if (bflush) FLUSH_BLOCK(s, 0);
    }
    s->insert = 0;
    if (flush == Z_FINISH) {
        FLUSH_BLOCK(s, 1);
        return finish_done;
    }
    if (s->last_lit)
        FLUSH_BLOCK(s, 0);
    return block_done;
}
