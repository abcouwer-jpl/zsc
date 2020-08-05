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
 * @file        deflate.h
 * @date        2020-08-05
 * @author      Jean-loup Gailly, Neil Abcouwer
 * @brief       Internal compression state
 *
 * Modified version of deflate.h for safety-critical applications.
 * Original file header follows.
 */

/* deflate.h -- internal compression state
 * Copyright (C) 1995-2016 Jean-loup Gailly
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* @(#) $Id$ */

#ifndef DEFLATE_H
#define DEFLATE_H

#include "zutil.h"

// Abcouwer ZSC - remove compilation conditional on GZIP/NO_GZIP


/* ===========================================================================
 * Internal compression state.
 */

#define LENGTH_CODES 29
/* number of length codes, not counting the special END_BLOCK code */

#define LITERALS  256
/* number of literal bytes 0..255 */

#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes, including the END_BLOCK code */

#define D_CODES   30
/* number of distance codes */

#define BL_CODES  19
/* number of codes used to transfer the bit lengths */

#define HEAP_SIZE (2*L_CODES+1)
/* maximum heap size */

#define MAX_BITS 15
/* All codes must not exceed MAX_BITS bits */

#define Buf_size 16
/* size of bit buffer in bi_buf */

#define INIT_STATE    42    /* zlib header -> BUSY_STATE */
#define GZIP_STATE    57    /* gzip header -> BUSY_STATE | EXTRA_STATE */
#define EXTRA_STATE   69    /* gzip extra block -> NAME_STATE */
#define NAME_STATE    73    /* gzip file name -> COMMENT_STATE */
#define COMMENT_STATE 91    /* gzip comment -> HCRC_STATE */
#define HCRC_STATE   103    /* gzip header CRC -> BUSY_STATE */
#define BUSY_STATE   113    /* deflate -> FINISH_STATE */
#define FINISH_STATE 666    /* stream complete */
/* Stream status */


/* Data structure describing a single value and its code string. */
typedef struct ct_data_s {
    union {
        U16  freq;       /* frequency count */
        U16  code;       /* bit string */
    } fc;
    union {
        U16  dad;        /* father node in Huffman tree */
        U16  len;        /* length of bit string */
    } dl;
} ct_data;

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

typedef struct static_tree_desc_s  static_tree_desc;

typedef struct tree_desc_s {
    ct_data *dyn_tree;           /* the dynamic tree */
    I32     max_code;            /* largest code with non zero frequency */
    const static_tree_desc *stat_desc;  /* the corresponding static tree */
} tree_desc;

typedef U16 Pos;
typedef U32 IPos;

/* A Pos is an index in the character window. We use short instead of int to
 * save space in the various tables. IPos is used only for parameter passing.
 */

typedef struct internal_state {
    z_stream * strm;      /* pointer back to this zlib stream */
    I32   status;        /* as the name implies */
    U8 *pending_buf;  /* output still pending */
    U32   pending_buf_size; /* size of pending_buf */
    U8 *pending_out;  /* next pending byte to output to the stream */
    U32   pending;       /* nb of bytes in the pending buffer */
    I32   wrap;          /* bit 0 true for zlib, bit 1 true for gzip */
    gz_header *  gzhead;  /* gzip header information to write */
    U32   gzindex;       /* where in extra, name, or comment */
    ZlibMethod  method;        /* can only be DEFLATED */
    ZlibFlush   last_flush;    /* value of flush param for previous deflate call */

                /* used by deflate.c: */

    U32  w_size;        /* LZ77 window size (32K by default) */
    U32  w_bits;        /* log2(w_size)  (8..16) */
    U32  w_mask;        /* w_size - 1 */

    U8 *window;
    /* Sliding window. Input bytes are read into the second half of the window,
     * and move to the first half later to keep a dictionary of at least wSize
     * bytes. With this organization, matches are limited to a distance of
     * wSize-MAX_MATCH bytes, but this ensures that IO is always
     * performed with a length multiple of the block size. Also, it limits
     * the window size to 64K, which is quite useful on MSDOS.
     * To do: use the user input buffer as sliding window.
     */

    U32 window_size;
    /* Actual size of window: 2*wSize, except when the user input buffer
     * is directly used as sliding window.
     */

    Pos *prev;
    /* Link to older string with same hash index. To limit the size of this
     * array to 64K, this link is maintained only for the last 32K strings.
     * An index in this array is thus a window index modulo 32K.
     */

    Pos *head; /* Heads of the hash chains or NIL. */

    U32  ins_h;          /* hash index of string to be inserted */
    U32  hash_size;      /* number of elements in hash table */
    U32  hash_bits;      /* log2(hash_size) */
    U32  hash_mask;      /* hash_size-1 */

    U32  hash_shift;
    /* Number of bits by which ins_h must be shifted at each input
     * step. It must be such that after MIN_MATCH steps, the oldest
     * byte no longer takes part in the hash key, that is:
     *   hash_shift * MIN_MATCH >= hash_bits
     */

    I32 block_start;
    /* Window position at the beginning of the current output block. Gets
     * negative when the window is moved backwards.
     */

    U32 match_length;           /* length of best match */
    IPos prev_match;             /* previous match */
    I32 match_available;         /* set if previous match exists */
    U32 strstart;               /* start of string to insert */
    U32 match_start;            /* start of matching string */
    U32 lookahead;              /* number of valid bytes ahead in window */

    U32 prev_length;
    /* Length of the best match at previous step. Matches not greater than this
     * are discarded. This is used in the lazy match evaluation.
     */

    U32 max_chain_length;
    /* To speed up deflation, hash chains are never searched beyond this
     * length.  A higher limit improves compression ratio but degrades the
     * speed.
     */

    U32 max_lazy_match;
    /* Attempt to find a better match only when the current match is strictly
     * smaller than this value. This mechanism is used only for compression
     * levels >= 4.
     */
#   define max_insert_length  max_lazy_match
    /* Insert new strings in the hash table only if the match length is not
     * greater than this length. This saves time but degrades compression.
     * max_insert_length is used only for compression levels <= 3.
     */

    I32 level;    /* compression level (1..9) */
    ZlibStrategy strategy; /* favor or force Huffman coding*/

    U32 good_match;
    /* Use a faster search when the previous match is longer than this */

    I32 nice_match; /* Stop searching when current match exceeds this */

                /* used by trees.c: */
    /* Didn't use ct_data typedef below to suppress compiler warning */
    struct ct_data_s dyn_ltree[HEAP_SIZE];   /* literal and length tree */
    struct ct_data_s dyn_dtree[2*D_CODES+1]; /* distance tree */
    struct ct_data_s bl_tree[2*BL_CODES+1];  /* Huffman tree for bit lengths */

    struct tree_desc_s l_desc;               /* desc. for literal tree */
    struct tree_desc_s d_desc;               /* desc. for distance tree */
    struct tree_desc_s bl_desc;              /* desc. for bit length tree */

    U16 bl_count[MAX_BITS+1];
    /* number of codes at each bit length for an optimal tree */

    I32 heap[2*L_CODES+1];      /* heap used to build the Huffman trees */
    I32 heap_len;               /* number of elements in the heap */
    I32 heap_max;               /* element of largest frequency */
    /* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
     * The same heap array is used to build all trees.
     */

    U8 depth[2*L_CODES+1];
    /* Depth of each subtree used as tie breaker for trees of equal frequency
     */

    U8 *l_buf;          /* buffer for literals or lengths */

    U32  lit_bufsize;
    /* Size of match buffer for literals/lengths.  There are 4 reasons for
     * limiting lit_bufsize to 64K:
     *   - frequencies can be kept in 16 bit counters
     *   - if compression is not successful for the first block, all input
     *     data is still in the window so we can still emit a stored block even
     *     when input comes from standard input.  (This can also be done for
     *     all blocks if lit_bufsize is not greater than 32K.)
     *   - if compression is not successful for a file smaller than 64K, we can
     *     even emit a stored file instead of a stored block (saving 5 bytes).
     *     This is applicable only for zip (not gzip or zlib).
     *   - creating new Huffman trees less frequently may not provide fast
     *     adaptation to changes in the input data statistics. (Take for
     *     example a binary file with poorly compressible code followed by
     *     a highly compressible string table.) Smaller buffer sizes give
     *     fast adaptation but have of course the overhead of transmitting
     *     trees more frequently.
     *   - I can't count above 4
     */

    U32 last_lit;      /* running index in l_buf */

    U16 *d_buf;
    /* Buffer for distances. To simplify the code, d_buf and l_buf have
     * the same number of elements. To use different lengths, an extra flag
     * array would be necessary.
     */

    U32 opt_len;        /* bit length of current block with optimal trees */
    U32 static_len;     /* bit length of current block with static trees */
    U32 matches;       /* number of string matches in current block */
    U32 insert;        /* bytes at end of window left to insert */

    U16 bi_buf;
    /* Output buffer. bits are inserted starting at the bottom (least
     * significant bits).
     */
    I32 bi_valid;
    /* Number of valid bits in bi_buf.  All bits above the last valid bit
     * are always zero.
     */

    U32 high_water;
    /* High water mark offset in window for initialized bytes -- bytes above
     * this are set to zero in order to avoid memory check warnings when
     * longest match routines access bytes past the input.  This is then
     * updated to the new high water mark.
     */

} deflate_state;

// check that our macro for size of the private deflate state is correct
ZSC_COMPILE_ASSERT(Z_DEFLATE_STATE_SIZE >= sizeof(deflate_state),
        bad_deflate_state_size);


/* Output a byte on the stream.
 * IN assertion: there is enough room in pending_buf.
 */
#define put_byte(s, c) {(s)->pending_buf[(s)->pending++] = (U8)(c);}


#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST(s)  ((s)->w_size-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

#define WIN_INIT MAX_MATCH
/* Number of bytes after end of data in window to initialize in order to avoid
   memory checker errors from longest match routines */

        /* in trees.c */
void _tr_init (deflate_state *s);
void _tr_flush_block (deflate_state *s, U8 *buf,
                        U32 stored_len, I32 last);
void _tr_flush_bits (deflate_state *s);
void _tr_align (deflate_state *s);
void _tr_stored_block (deflate_state *s, U8 *buf,
                        U32 stored_len, I32 last);

#define d_code(dist) \
   ((dist) < 256 ? _dist_code[(dist)] : _dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. _dist_code[256] and _dist_code[257] are never
 * used.
 */

/* Inline versions of _tr_tally for speed: */

extern const U8 _length_code[];
extern const U8 _dist_code[];

# define _tr_tally_lit(s, c, flush) \
  { U8 cc = (c); \
    (s)->d_buf[(s)->last_lit] = 0; \
    (s)->l_buf[(s)->last_lit++] = cc; \
    (s)->dyn_ltree[cc].Freq++; \
    (flush) = ((s)->last_lit == (s)->lit_bufsize-1); \
   }
# define _tr_tally_dist(s, distance, length, flush) \
  { U8 len = (U8)(length); \
    U16 dist = (U16)(distance); \
    (s)->d_buf[(s)->last_lit] = dist; \
    (s)->l_buf[(s)->last_lit++] = len; \
    dist--; \
    (s)->dyn_ltree[_length_code[len]+LITERALS+1].Freq++; \
    (s)->dyn_dtree[d_code(dist)].Freq++; \
    (flush) = ((s)->last_lit == (s)->lit_bufsize-1); \
  }

#endif /* DEFLATE_H */
