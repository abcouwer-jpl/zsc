/* example.c -- usage example of the zlib compression library
 * Copyright (C) 1995-2006, 2011, 2016 Jean-loup Gailly
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#include "zutil.h"
#include "zlib.h"
#include "zsc_pub.h"
#include "zsc.h"
#include "deflate.h"
#include "inftrees.h"
#include "inflate.h"
#include "gtest/gtest.h"

#include <stdio.h>

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#endif



#if defined(VMS) || defined(RISCOS)
#  define TESTFILE "foo-gz"
#else
#  define TESTFILE "foo.gz"
#endif

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

#define MIN(A,B) ( ((A) <= (B)) ? (A) : (B) )
#define MAX(A,B) ( ((A) >= (B)) ? (A) : (B) )

#ifndef DEF_WBITS
#  define DEF_WBITS MAX_WBITS
#endif
/* default windowBits for decompression. MAX_WBITS is for compression only */

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
/* default memLevel */

// disabling corpus test makes testing much faster,
// but gets slightly less coverage
#define DO_LONG_TESTS 1


// max size of a file from a corpus
enum {
    CORPUS_MAX_SIZE_CANTRBRY = 1029744,
    CORPUS_MAX_SIZE_ARTIFICL =  100000,
    CORPUS_MAX_SIZE_LARGE    = 4638690,
    CORPUS_MAX_SIZE_CALGARY  =  768771,
    CORPUS_MAX_SIZE = CORPUS_MAX_SIZE_LARGE
};

typedef enum {
    WRAP_NONE = 0, // raw wrapper
    WRAP_ZLIB, // default wrapper
    WRAP_GZIP_NULL, // null gzip wrapper
    WRAP_GZIP_BASIC, // gzip wrapper with basic data filled
    WRAP_GZIP_BUFFERS, // gzip wrapper with buffers
} WrapType;

typedef enum {
    SCENARIO_BASIC = 0, // use simplified functions
    SCENARIO_DICTIONARY = 1 << 0, // set and get dictionaries
    SCENARIO_PENDING = 1 << 1,    // do get pending
    SCENARIO_TUNE = 1 << 2,    // do deflate tune
    SCENARIO_FULL_FLUSH = 1 << 3, // flush in portions
    SCENARIO_CORRUPT = 1 << 4, // corrupted portion
    SCENARIO_TINY = 1 << 5, // tiny buffers require more flushing
    SCENARIO_PARTIAL_FLUSH = 1 << 6, // do partial flush
    SCENARIO_NO_FLUSH = 1 << 7, // do no flush
    SCENARIO_SYNC_FLUSH = 1 << 8, // do sync flush
    SCENARIO_RESET = 1 << 9, // reset streams
    SCENARIO_BIG_HEADER_BUFFERS_SMALL_AVAIL = 1 << 10,
    SCENARIO_INFLATE_BLOCK = 1 << 11, // inflate stops on block boundaries
    SCENARIO_BIG_DICTIONARY = 1 << 12, // use big dictionary
    SCENARIO_PARAMS = 1 << 13, // change params after first block deflate
    SCENARIO_INFLATE_BITS0 = 1 << 14, // use wsize of compressed stream


    //    SCENARIO_RESET_DIFF_BITS = 1 << 14 // change windowbits during reset

} ZlibTestScenario;

enum {NUM_CORPUS = 20}; //keep in sync

// FIXME try again or delete
//// expose local functions
//extern unsigned long crc32_big(unsigned long crc,
//        const unsigned char FAR *buf, z_size_t len);
//extern unsigned long crc32_little(unsigned long crc,
//        const unsigned char FAR *buf, z_size_t len);


char *corpus_files[NUM_CORPUS] = {
        (char*)"corpus/cantrbry/alice29.txt",
        (char*)"corpus/cantrbry/asyoulik.txt",
        (char*)"corpus/cantrbry/cp.html",
        (char*)"corpus/cantrbry/fields.c",
        (char*)"corpus/cantrbry/grammar.lsp",
        (char*)"corpus/cantrbry/kennedy.xls",
        (char*)"corpus/cantrbry/lcet10.txt",
        (char*)"corpus/cantrbry/plrabn12.txt",
        (char*)"corpus/cantrbry/ptt5",
        (char*)"corpus/cantrbry/sum",
        (char*)"corpus/cantrbry/xargs.1",
        (char*)"corpus/artificl/a.txt",
        (char*)"corpus/artificl/aaa.txt",
        (char*)"corpus/artificl/alphabet.txt",
        (char*)"corpus/artificl/random.txt",
        (char*)"corpus/large/E.coli",
        (char*)"corpus/large/bible.txt",
        (char*)"corpus/large/world192.txt",
        (char*)"corpus/calgary/obj2",
        (char*)"corpus/calgary/pic",
};


//static z_const char hello[] = "hello, hello!";
///* "hello world" would be more standard, but the repeated "hello"
// * stresses the compression code better, sorry...
// */
//
//static const char dictionary[] = "hello";
//static uLong dictId;    /* Adler32 value of the dictionary */


static const Bytef * alice_dictionary = (const Bytef *)
        "queen" "time" "thought" "could" "and the" "in the" "would" "went"
                "like" "know" "in a" "one" "said Alice" "little" "of the"
                "said the" "Alice" "said";

static const Bytef * big_dictionary = (const Bytef *)
    "queen" "time" "thought" "could" "and the" "in the" "would" "went"
    "like" "know" "in a" "one" "said Alice" "little" "of the"
    "said the" "Alice" "said"
    "queen" "time" "thought" "could" "and the" "in the" "would" "went"
    "like" "know" "in a" "one" "said Alice" "little" "of the"
    "said the" "Alice" "said"
    "queen" "time" "thought" "could" "and the" "in the" "would" "went"
    "like" "know" "in a" "one" "said Alice" "little" "of the"
    "said the" "Alice" "said"
    "queen" "time" "thought" "could" "and the" "in the" "would" "went"
    "like" "know" "in a" "one" "said Alice" "little" "of the"
    "said the" "Alice" "said"
    "queen" "time" "thought" "could" "and the" "in the" "would" "went"
    "like" "know" "in a" "one" "said Alice" "little" "of the"
    "said the" "Alice" "said"
    "queen" "time" "thought" "could" "and the" "in the" "would" "went"
    "like" "know" "in a" "one" "said Alice" "little" "of the"
    "said the" "Alice" "said";

// The fixture for testing zlib.
class ZlibTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if their bodies would
  // be empty.

  ZlibTest() {
     // You can do set-up work for each test here.
  }

  ~ZlibTest() override {
     // You can do clean-up work that doesn't throw exceptions here.

  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
     // Code here will be called immediately after the constructor (right
     // before each test).
  }

  void TearDown() override {
     // Code here will be called immediately after each test (right
     // before the destructor).
  }

//  // Class members declared here can be used by all tests in the test suite
//  // for Foo.
};

int good_allocs_allowed = 0;

//// a bad allocator function which only returns NULL, for testing fault cases
//voidpf zlib_test_bad_alloc(voidpf opaque, uInt items, uInt size)
//{
//    if (good_allocs_allowed > 0) {
//        good_allocs_allowed--;
//        return z_static_alloc (opaque, items, size);
//    } else {
//        return Z_NULL;
//    }
//
//}

void zlib_test(
        Byte * source_buf,
        int source_buf_len,
        WrapType wrapper = WRAP_ZLIB,
        unsigned long scenarios = 0, // bit mask of scenarios
        int level = Z_DEFAULT_COMPRESSION,
        int windowBits = DEF_WBITS,
        int memLevel = DEF_MEM_LEVEL,
        int strategy = Z_DEFAULT_STRATEGY,
        int max_block_size = 100000)
{

    printf("Zlib Test: buf_len = %d, wrapper = %d, scenarios = %#lx, "
            "level = %d, windowbits = %d, memLevel = %d, strategy = %d, "
            "max block size = %d.\n",
            source_buf_len, wrapper, scenarios,
            level, windowBits, memLevel, strategy, max_block_size);

    int err;

    if (scenarios & SCENARIO_BIG_DICTIONARY) {
        scenarios |= SCENARIO_DICTIONARY;
    }

    if ( (scenarios & SCENARIO_TINY)
            || (scenarios & SCENARIO_BIG_HEADER_BUFFERS_SMALL_AVAIL)) {
        windowBits = 9;
        memLevel = 1;
    }

    // if all params are default, we'll use more basic functions
    bool all_params_default =
            ( windowBits == DEF_WBITS
             && memLevel == DEF_MEM_LEVEL
             && strategy == Z_DEFAULT_STRATEGY);

    uLong compressed_buf_len;
    Byte * compressed_buf;
    uLong work_buf_len;
    Byte * work_buf;
    uLong compressed_buf_len_out;
    uLong source_buf_len_out;

    gz_header head_in;
    memset(&head_in, 0, sizeof(head_in));
    gz_header head_out;
    memset(&head_out, 0, sizeof(head_out));

    head_in.text = true;
    head_in.time = 42;
    head_in.os = 3;
    head_in.extra = Z_NULL;
    head_in.name = Z_NULL;
    head_in.comment = Z_NULL;
    head_in.hcrc = true;
    if (wrapper == WRAP_GZIP_BUFFERS) {
        int len;
        if(scenarios & SCENARIO_BIG_HEADER_BUFFERS_SMALL_AVAIL) {
            len = 1000;
        } else {
            len = 50;
        }

        head_in.comment = (Byte *) malloc(len);
        head_in.name = (Byte *) malloc(len);
        head_in.extra = (Byte *) malloc(len);
        head_in.extra_len = len;

        sprintf((char*)head_in.comment,"I am the man with no name.");
        sprintf((char*)head_in.name,"Zapp Brannigan.");
        sprintf((char*)head_in.extra,"Hello.");
        head_in.extra_len = strlen((char*) head_in.extra);

        if(scenarios & SCENARIO_BIG_HEADER_BUFFERS_SMALL_AVAIL) {
            for(int j = 0; j < len-1; j++) {
                head_in.comment[j] = 'A';
                head_in.name[j] = 'A';
                head_in.extra[j] = 'A';
            }
            head_in.comment[len-1] = '\0';
            head_in.name[len-1] = '\0';
            head_in.extra[len-1] = '\0';
            head_in.extra_len = len;
        }
    }
    head_out.name = (Byte *)malloc(80);
    head_out.name_max = 80;
    head_out.comment = (Byte *)malloc(80);
    head_out.comm_max = 80;
    head_out.extra = (Byte *)malloc(80);
    head_out.extra_max = 80;

    const Bytef * dictionary = alice_dictionary;
    if (scenarios & SCENARIO_BIG_DICTIONARY) {
        dictionary = big_dictionary;
    }
    int dictLength = strlen((char*) dictionary);

    // adjust window bits
    switch (wrapper) {
    case WRAP_NONE:
        // if not already encoded, signify no wrapper
        if (windowBits > 0) {
            windowBits = -windowBits;
            printf("Changed windowbits to %d, signifying no wrapper.\n",
                    windowBits);
        }
        break;
    case WRAP_ZLIB: // do nothing
        break;
    case WRAP_GZIP_NULL:
    case WRAP_GZIP_BASIC:
    case WRAP_GZIP_BUFFERS:
        // if not already encoded, signify gzip wrapper
        if (windowBits <= 15) {
            windowBits += 16;
        }
        break;
    }

    uLong predicted_max_output_size;
    // get max output size
    switch(wrapper) {
    case WRAP_NONE:
        err = zsc_compress_get_max_output_size2(source_buf_len, max_block_size,
                level, windowBits, memLevel, &predicted_max_output_size);
        break;
    case WRAP_ZLIB:
        if(all_params_default) {
            err = zsc_compress_get_max_output_size(source_buf_len, max_block_size,
                    level, &predicted_max_output_size);
        } else {
            err = zsc_compress_get_max_output_size2(source_buf_len, max_block_size,
                    level, windowBits, memLevel, &predicted_max_output_size);
        }
        break;
    case WRAP_GZIP_NULL:
        err = zsc_compress_get_max_output_size_gzip2(source_buf_len, max_block_size,
                level, windowBits, memLevel, Z_NULL, &predicted_max_output_size);
        break;
    case WRAP_GZIP_BASIC:
    case WRAP_GZIP_BUFFERS:
        if(all_params_default) {
            err = zsc_compress_get_max_output_size_gzip(source_buf_len, max_block_size,
                    level, &head_in, &predicted_max_output_size);
        } else {
            err = zsc_compress_get_max_output_size_gzip2(source_buf_len, max_block_size,
                    level, windowBits, memLevel, &head_in, &predicted_max_output_size);
        }
        break;
    }
    EXPECT_EQ(err, Z_OK);


    printf("max_output calculated at %lu.\n", predicted_max_output_size);
    compressed_buf_len = predicted_max_output_size + 1000;

    compressed_buf = (Byte *) malloc(compressed_buf_len);
    ASSERT_NE(compressed_buf, (Bytef*)NULL);

    // get work buf

    if(all_params_default && wrapper == WRAP_ZLIB){
        err = zsc_compress_get_min_work_buf_size(&work_buf_len);
    } else {
        err = zsc_compress_get_min_work_buf_size2(windowBits, memLevel, &work_buf_len);
    }
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    ASSERT_NE(work_buf, (Bytef*)NULL);
    compressed_buf_len_out = compressed_buf_len;
    // fill work buffer with garbage
    memset((void*)work_buf, 0xa5,work_buf_len);

    printf("Compressed buf size: %lu. Work buf size: %lu.\n",
            compressed_buf_len, work_buf_len);

    if(scenarios == SCENARIO_BASIC
            || (all_params_default && (scenarios & SCENARIO_CORRUPT))) {
        printf("Using compress function\n");
        switch (wrapper) {
        case WRAP_NONE:
            err = zsc_compress2(compressed_buf, &compressed_buf_len_out,
                    source_buf, source_buf_len, max_block_size,
                    work_buf, work_buf_len, level,
                    windowBits, memLevel, strategy);
            break;
        case WRAP_ZLIB:
            if (all_params_default) {
                err = zsc_compress(compressed_buf, &compressed_buf_len_out,
                        source_buf, source_buf_len, max_block_size,
                        work_buf, work_buf_len, level);
            } else {
                err = zsc_compress2(compressed_buf, &compressed_buf_len_out,
                        source_buf, source_buf_len, max_block_size,
                        work_buf, work_buf_len, level,
                        windowBits, memLevel, strategy);
            }
            break;
        case WRAP_GZIP_NULL:
            if (all_params_default) {
                 err = zsc_compress_gzip(compressed_buf, &compressed_buf_len_out,
                         source_buf, source_buf_len, max_block_size,
                         work_buf, work_buf_len, level, Z_NULL);
             } else {
                 err = zsc_compress_gzip2(compressed_buf, &compressed_buf_len_out,
                         source_buf, source_buf_len, max_block_size,
                         work_buf, work_buf_len, level,
                         windowBits, memLevel, strategy, Z_NULL);
             }
            break;
        case WRAP_GZIP_BASIC:
        case WRAP_GZIP_BUFFERS:
            if (all_params_default) {
                err = zsc_compress_gzip(compressed_buf, &compressed_buf_len_out,
                        source_buf, source_buf_len, max_block_size,
                        work_buf, work_buf_len, level, &head_in);
            } else {
                err = zsc_compress_gzip2(compressed_buf, &compressed_buf_len_out,
                        source_buf, source_buf_len, max_block_size,
                        work_buf, work_buf_len, level,
                        windowBits, memLevel, strategy, &head_in);
            }
            break;
        }
        EXPECT_EQ(err, Z_OK);


    } else {
        printf("Using deflate functions\n");

//        z_static_mem mem;
//        mem.work = work_buf;
//        mem.work_len = work_buf_len;
//        mem.work_alloced = 0;

        z_stream stream;
//        stream.zalloc = z_static_alloc;
//        stream.zfree = z_static_free;
//        stream.opaque = (voidpf) &mem;

        stream.next_work = work_buf;
        stream.avail_work = work_buf_len;


        printf("deflateInit with windowBits=%d\n", windowBits);

        if (all_params_default && wrapper == WRAP_ZLIB) {
            err = deflateInit(&stream, level);
        } else {
            err = deflateInit2(&stream, level, Z_DEFLATED,
                    windowBits, memLevel, strategy);
        }
        EXPECT_EQ(err, Z_OK);

        if(scenarios & SCENARIO_TUNE) {
            err = deflateTune(&stream, 4, 5, 16, 16);
            EXPECT_EQ(err, Z_OK);
        }

        unsigned pending_bytes;
        int pending_bits;

        int times_compress = 1;
        if (scenarios & SCENARIO_RESET) {
            times_compress = 2;
        }
        bool changed_params = false;

        do {
            // set header if gzip
            switch (wrapper) {
            case WRAP_GZIP_NULL:
                err = deflateSetHeader(&stream, Z_NULL);
                break;
            case WRAP_GZIP_BASIC:
                case WRAP_GZIP_BUFFERS:
                err = deflateSetHeader(&stream, &head_in);
                break;
            default:
                break;
            }
            EXPECT_EQ(err, Z_OK);

            if ( scenarios & SCENARIO_DICTIONARY) {
                printf("setting dictionary\n");
                err = deflateSetDictionary(&stream, dictionary, dictLength);
                EXPECT_EQ(err, Z_OK);
            }


            if (scenarios & SCENARIO_PENDING) {
                err = deflatePending(&stream, &pending_bytes, &pending_bits);
                EXPECT_EQ(err, Z_OK);
                printf("pending bytes: %u pending bits: %d.\n",
                        pending_bytes, pending_bits);
            }

            uInt max_in = (uInt) -1;
            uInt max_out = (uInt) -1;
            uLong bytes_left_dest = compressed_buf_len_out;
            uLong bytes_left_source = source_buf_len;

            stream.next_out = compressed_buf;
            stream.avail_out = 0;
            stream.next_in = source_buf;
            stream.avail_in = 0;

            int flush_type = Z_NO_FLUSH;

            if (scenarios & SCENARIO_NO_FLUSH) {
                printf("doing no flush\n");
                flush_type = Z_NO_FLUSH;
                max_in = 10000;
                max_out = 10000;
            } else if (scenarios & SCENARIO_FULL_FLUSH) {
                printf("doing full flush\n");
                flush_type = Z_FULL_FLUSH;
                max_in = 10000;
                max_out = 10000;
            } else if (scenarios & SCENARIO_PARTIAL_FLUSH) {
                printf("doing partial flush\n");
                flush_type = Z_PARTIAL_FLUSH;
                max_in = 10000;
                max_out = 10000;
            } else if (scenarios & SCENARIO_SYNC_FLUSH) {
                printf("doing sync flush\n");
                flush_type = Z_SYNC_FLUSH;
                max_in = 10000;
                max_out = 10000;
            }

            if(scenarios & SCENARIO_BIG_HEADER_BUFFERS_SMALL_AVAIL) {
                max_in = 80;
                max_out = 80;
            }

            if (scenarios & SCENARIO_TINY) {
                max_out = 4;
                max_in = 4;
            }

            int cycles = 0;
            do {
                cycles++;
                if (stream.avail_out == 0) {
                    // provide more output
                    stream.avail_out =
                            bytes_left_dest > (uLong) max_out ?
                                    max_out : (uInt) bytes_left_dest;
                    bytes_left_dest -= stream.avail_out;
                }
                if (stream.avail_in == 0) {
                    stream.avail_in =
                            bytes_left_source > (uLong) max_in ?
                                    max_in : (uInt) bytes_left_source;
                    bytes_left_source -= stream.avail_in;
                }
                err = deflate(&stream,
                        bytes_left_source ? flush_type : Z_FINISH);


                if ( (scenarios & SCENARIO_PARAMS) && !changed_params) {
                    printf("Changing params\n");
                    if (stream.avail_out == 0) {
                        // provide more output
                        stream.avail_out =
                                bytes_left_dest > (uLong) max_out ?
                                        max_out : (uInt) bytes_left_dest;
                        bytes_left_dest -= stream.avail_out;
                    }
                    err = deflateParams(&stream, 8, Z_HUFFMAN_ONLY);
                    EXPECT_EQ(err, Z_OK);
                    changed_params = true;
                }

            } while (err == Z_OK);

            if (cycles > 1) {
                printf("%d cycles\n", cycles);
            }

            compressed_buf_len_out = stream.total_out;

            if (scenarios & SCENARIO_DICTIONARY) {
                Bytef * dictionary_out = (Bytef *) malloc(32768);
                ASSERT_NE(dictionary_out, (Bytef* )NULL);
                memset(dictionary_out, 0, 32768);

                uInt dict_out_len = 0;

                printf("getting dictionary\n");
                err = deflateGetDictionary(&stream, dictionary_out,
                        &dict_out_len);
                EXPECT_EQ(err, Z_OK);

                int print_out = dict_out_len < 512 ? dict_out_len : 512;
                printf("Dictionary of length %u, printing %d:\n", dict_out_len,
                        print_out);
                for (int i = 0; i < print_out; i++) {
                    if (isprint(dictionary_out[i])) {
                        printf("%c", dictionary_out[i]);
                    } else {
                        printf(" %#x ", dictionary_out[i]);
                    }
                }
                printf("\n");

                free(dictionary_out);
            }

            times_compress--;

            if (times_compress > 0) {
                printf("reseting deflate stream\n");
                err = deflateReset(&stream);
                EXPECT_EQ(err, Z_OK);
            }
        } while (times_compress > 0);

        err = deflateEnd(&stream);
        EXPECT_EQ(err, Z_OK);

        if(scenarios & SCENARIO_PENDING) {
            err = deflatePending(&stream,&pending_bytes, &pending_bits);
            EXPECT_EQ(err, Z_STREAM_ERROR);
        }

    } // end else scenarios != SCENARIO_BASIC

    EXPECT_LE(compressed_buf_len_out, predicted_max_output_size);


    free(work_buf);

    printf("Compressed to size: %lu.\n",
            compressed_buf_len_out);

    // sync search
    unsigned got = 0;
    unsigned next = 0;
    int first_sync_loc = -1;
    int sync_count = 0;
    printf("Looking for 0 0 FF FF sync pattern:");
    while (next < compressed_buf_len_out) {
        if ((int)(compressed_buf[next]) == (got < 2 ? 0 : 0xff)) {
            got++;
            if(got == 4) {
                if (sync_count == 0) {
                    first_sync_loc = next - 3;
                    printf(" Found at: %d", next - 3);
                } else {
                    printf(", %d", next - 3);
                }
                sync_count++;
            }
        } else if (compressed_buf[next]) {
            got = 0;
        } else {
            got = 4 - got;
        }
        next++;
    }
    if (sync_count == 0) {
        printf(" Not found.\n");
    } else {
        printf(".\nFound %d instances.\n", sync_count);
    }


    printf("Compressed buffer:\n");
    for (int i = 0; i < MIN(compressed_buf_len_out,200); i++) {
        if(i%20 == 0) {
            printf("\n%04d: ", i);
        }
        printf("%02x ", compressed_buf[i]);
    }
    printf("\n...\n");

    if (first_sync_loc != -1) {
        for (int i = MAX(0, first_sync_loc-100);
                 i < MIN(first_sync_loc+100, compressed_buf_len_out);
                 i++) {
             if(i%20 == 0) {
                 printf("\n%04d: ", i);
             }
             printf("%02x ", compressed_buf[i]);
         }
         printf("\n...\n");
    }
//    if(max_block_size < compressed_buf_len_out) {
//
//        for (int i = max_block_size-100;
//                i < max_block_size+100;
//                i++) {
//            if(i%20 == 0) {
//                printf("\n%04d: ", i);
//            }
//            printf("%02x ", compressed_buf[i]);
//        }
//        printf("\n...\n");
//    }
    for (int i = MAX(0,compressed_buf_len_out-100); i < compressed_buf_len_out; i++) {
        if(i%20 == 0) {
            printf("\n%04d: ", i);
        }
        printf("%02x ", compressed_buf[i]);
    }
    printf("\n...\n");






    if(scenarios & SCENARIO_CORRUPT) {
        printf("Corrupting compressed buffer\n");
        compressed_buf[first_sync_loc > 0 ? first_sync_loc / 2 : 42]++;
    }

    uLong uncompressed_buf_len = source_buf_len;
    Byte * uncompressed_buf = (Byte *) malloc(uncompressed_buf_len);
    EXPECT_NE(uncompressed_buf, (Byte * )NULL);
    err = zsc_uncompress_get_min_work_buf_size(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    ASSERT_NE(work_buf, (Byte *)Z_NULL);
    // fill work buffer with garbage
    memset((void*)work_buf, 0x5a,work_buf_len);

    printf("Uncompress. Work buf size: %lu.\n",
            work_buf_len);

    uLong uncompressed_buf_len_out = uncompressed_buf_len;



    if (scenarios == SCENARIO_BASIC
            || (all_params_default && (scenarios & SCENARIO_CORRUPT))) {

        printf("Using uncompress function\n");

        switch (wrapper) {
        case WRAP_NONE:
            err = zsc_uncompress_safe2(uncompressed_buf, &uncompressed_buf_len_out,
                    compressed_buf, &compressed_buf_len_out,
                    work_buf, work_buf_len, windowBits);
            break;
        case WRAP_ZLIB:
            if (all_params_default) {
                err = zsc_uncompress(uncompressed_buf, &uncompressed_buf_len_out,
                        compressed_buf, &compressed_buf_len_out,
                        work_buf, work_buf_len);
            } else {
                err = zsc_uncompress_safe2(uncompressed_buf, &uncompressed_buf_len_out,
                        compressed_buf, &compressed_buf_len_out,
                        work_buf, work_buf_len, windowBits);
            }
            break;
        case WRAP_GZIP_NULL:
        case WRAP_GZIP_BASIC:
        case WRAP_GZIP_BUFFERS:
            if (all_params_default) {
                err = zsc_uncompress_safe_gzip(uncompressed_buf, &uncompressed_buf_len_out,
                        compressed_buf, &compressed_buf_len_out,
                        work_buf, work_buf_len, &head_out);
            } else {
                err = zsc_uncompress_safe_gzip2(uncompressed_buf, &uncompressed_buf_len_out,
                        compressed_buf, &compressed_buf_len_out,
                        work_buf, work_buf_len, windowBits, &head_out);
            }
            break;
        }

        if(scenarios & SCENARIO_CORRUPT) {
            EXPECT_EQ(err, Z_DATA_ERROR);
        } else {
            EXPECT_EQ(err, Z_OK);
        }

        printf("uncompressed len: %lu\n", uncompressed_buf_len_out);

    } else {

        printf("Using inflate functions\n");

//        z_static_mem mem_inf;
//        mem_inf.work = work_buf;
//        mem_inf.work_len = work_buf_len;
//        mem_inf.work_alloced = 0;

        z_stream stream;

        uInt max_in = (uInt)-1;
        uInt max_out = (uInt)-1;

        stream.next_in = (z_const Bytef *) compressed_buf;
        stream.avail_in = 0;

        stream.next_work = work_buf;
        stream.avail_work = work_buf_len;

//        stream.zalloc = z_static_alloc;
//        stream.zfree = z_static_free;
//        stream.opaque = (voidpf) &mem_inf;

        if (scenarios & SCENARIO_INFLATE_BITS0) {
            if (wrapper == WRAP_ZLIB) {
                err = inflateInit2(&stream, 0); // use wsize of compressed stream
            } else {
                err = inflateInit2(&stream, 16 + 0); // use wsize of compressed stream
            }
        } else if (windowBits == DEF_WBITS
                   && wrapper == WRAP_ZLIB) {
            err = inflateInit(&stream);
        } else {
            err = inflateInit2(&stream, windowBits);
        }
        EXPECT_EQ(err, Z_OK);

        int times_decompress = 1;
        if(scenarios & SCENARIO_RESET) {
            times_decompress = 2;
        }


        do {
            uLong bytes_left_dest =  uncompressed_buf_len;
            uLong bytes_left_source =  compressed_buf_len_out;

            stream.next_in = (z_const Bytef *) compressed_buf;
            stream.avail_in = 0;
            stream.next_out = uncompressed_buf;
            stream.avail_out = 0;

            // for raw inflate, dictionary can be set any time
            if( (scenarios & SCENARIO_DICTIONARY) && wrapper == WRAP_NONE) {
                printf("after init, setting dictionary for raw inflate\n");
                err = inflateSetDictionary(&stream, dictionary, dictLength);
                EXPECT_EQ(err, Z_OK);
                if (stream.msg != Z_NULL) {
                    printf("msg: %s\n", stream.msg);
                }
            }

            int flush = Z_NO_FLUSH;
            if (scenarios & SCENARIO_INFLATE_BLOCK) {
                flush = Z_BLOCK;
                printf("flush = BLOCK. inflate stops on block boundaries\n");
            }

            int cycles = 0;
            do {
                cycles++;
                printf("inflate cycle %d\n", cycles);

                if (stream.avail_out == 0) {
                    stream.avail_out =
                            bytes_left_dest > (uLong) max_out ?
                                    max_out : (uInt) bytes_left_dest;
                    bytes_left_dest -= stream.avail_out;
                }
                if (stream.avail_in == 0) {
                    stream.avail_in =
                            bytes_left_source > (uLong) max_in ?
                                    max_in : (uInt) bytes_left_source;
                    bytes_left_source -= stream.avail_in;
                }
                printf("before: stream.avail_in = %d. stream.avail_out = %d.\n",
                        stream.avail_in, stream.avail_out);
                err = inflate(&stream, flush);//Z_NO_FLUSH);//Z_BLOCK);
                printf("after:  stream.avail_in = %d. stream.avail_out = %d.\n",
                        stream.avail_in, stream.avail_out);

                if (err == Z_NEED_DICT) {
                    EXPECT_TRUE(scenarios & SCENARIO_DICTIONARY);
                    printf("inflate() returned Z_NEED_DICT. adler-32 val = %lu. "
                            "setting dictionary for inflate\n",
                            stream.adler);
                    err = inflateSetDictionary(&stream, dictionary, dictLength);
                    EXPECT_EQ(err, Z_OK);
                    if (stream.msg != Z_NULL) {
                        printf("msg: %s\n", stream.msg);
                    }
                    // inflate(Z_BLOCK) after inflateSetDictionary()
                    // always returns Z_BUF_ERROR,
                    // because no progress gets made. ZLib Bug?
                    if(flush == Z_BLOCK) {
                        printf("inflate(Z_BLOCK) after inflateSetDictionary()\n");
                        printf(
                                "before: stream.avail_in = %d. stream.avail_out = %d.\n",
                                stream.avail_in, stream.avail_out);
                        err = inflate(&stream, Z_BLOCK);
                        printf(
                                "after:  stream.avail_in = %d. stream.avail_out = %d.\n",
                                stream.avail_in, stream.avail_out);
                        EXPECT_EQ(err, Z_BUF_ERROR);
                        if(err == Z_BUF_ERROR) {
                            err = Z_OK;
                        }
                    }
                }

                if (err == Z_DATA_ERROR) {
                    printf("data error.\n");
                    if (stream.msg != Z_NULL) {
                        printf("msg: %s\n", stream.msg);
                    }
                    // try to find a new flush point
                    err = inflateSync(&stream);
                    printf("inflateSync returned %d\n", err);
                    printf("stream.avail_in = %d. stream.avail_out = %d.\n",
                            stream.avail_in, stream.avail_out);
                    if (stream.msg != Z_NULL) {
                        printf("msg: %s\n", stream.msg);
                    }

                }

                bool at_sync_point = inflateSyncPoint(&stream);
                EXPECT_FALSE(at_sync_point);

            } while (err == Z_OK);

            if(cycles>1) {
                printf("cycles = %d\n", cycles);
            }

            if(scenarios & SCENARIO_CORRUPT) {
                EXPECT_EQ(err, Z_DATA_ERROR);
            } else {
                EXPECT_EQ(err, Z_STREAM_END);
                if(err != Z_STREAM_END) {
                    printf("msg: %s\n", stream.msg);
                }
            }
            uncompressed_buf_len_out = stream.total_out;

            if(scenarios & SCENARIO_DICTIONARY) {
                Bytef * dictionary_out_inf = (Bytef *) malloc(32768);
                ASSERT_NE(dictionary_out_inf, (Bytef*)NULL);
                uInt dict_out_inf_len = 0;
                printf("getting dictionary\n");
                err = inflateGetDictionary(&stream, dictionary_out_inf,
                        &dict_out_inf_len);
                EXPECT_EQ(err, Z_OK);
                free(dictionary_out_inf);
            }

            times_decompress--;


            if(times_decompress > 0) {
                printf("reseting inflate stream\n");
//                if(scenarios & SCENARIO_RESET_DIFF_BITS) {
//                    err = inflateReset2(&stream, DEF_WBITS);
//                } else {
                    err = inflateReset(&stream);
//                }
                EXPECT_EQ(err, Z_OK);
            }
        } while (times_decompress > 0);


        err = inflateEnd(&stream);
        EXPECT_EQ(err, Z_OK);

    } // end if not basic


    // Compare
    if (!(scenarios & SCENARIO_CORRUPT)) {
        EXPECT_EQ(uncompressed_buf_len_out, source_buf_len);

        int nbad = 0;
        if (uncompressed_buf_len_out == source_buf_len) {
            for (int i = 0; i < source_buf_len; i++) {
                if (source_buf[i] != uncompressed_buf[i])
                    nbad++;
            }
        }
        EXPECT_EQ(nbad, 0);
        printf("Decompressed buffer had %d errors.\n", nbad);
    } else {
        printf("Input:\n");
        for (int i = 0; i < (source_buf_len < 240 ? source_buf_len : 240); i++) {
            if (isprint(source_buf[i]) || source_buf[i] == 0xA
                || source_buf[i] == 0xD) {
                printf("%c", source_buf[i]);
            } else {
                printf(" %#x ", source_buf[i]);
            }
        }
        printf("...\n");
        printf("Output:\n");
        for (int i = 0; i < (uncompressed_buf_len_out < 240 ? uncompressed_buf_len_out : 240); i++) {
            if (isprint(uncompressed_buf[i]) || uncompressed_buf[i] == 0xA
                || uncompressed_buf[i] == 0xD) {
                printf("%c", uncompressed_buf[i]);
            } else {
                printf(" %#x ", uncompressed_buf[i]);
            }
        }
        printf("...\n");
    }


    free(work_buf);
    free(compressed_buf);
    free(uncompressed_buf);


    free(head_in.name);
    free(head_in.comment);
    free(head_in.extra);
    free(head_out.name);
    free(head_out.comment);
    free(head_out.extra);

}

// test with alice as input
void zlib_test_file(
        const char * filename = "corpus/cantrbry/alice29.txt",
        WrapType wrapper = WRAP_ZLIB,
        unsigned long scenarios = 0, // bit mask of scenarios
        int level = Z_DEFAULT_COMPRESSION,
        int windowBits = DEF_WBITS,
        int memLevel = DEF_MEM_LEVEL,
        int strategy = Z_DEFAULT_STRATEGY)
{
    printf("Compress file %s.\n", filename);
    FILE * file = fopen(filename, "r");
    ASSERT_FALSE(file == NULL);

    int source_buf_len = CORPUS_MAX_SIZE;
    Byte * source_buf = (Byte *) malloc(source_buf_len);
    int nread = fread(source_buf, 1, source_buf_len, file);
    ASSERT_FALSE(ferror(file));
    fclose(file);
    ASSERT_TRUE(nread <= source_buf_len);
    source_buf_len = nread;

    zlib_test(source_buf, source_buf_len,
            wrapper, scenarios, level, windowBits, memLevel, strategy);

    free(source_buf);
}

// test with alice as input
void zlib_test_alice(
        WrapType wrapper = WRAP_ZLIB,
        unsigned long scenarios = 0, // bit mask of scenarios
        int level = Z_DEFAULT_COMPRESSION,
        int windowBits = DEF_WBITS,
        int memLevel = DEF_MEM_LEVEL,
        int strategy = Z_DEFAULT_STRATEGY)
{
    zlib_test_file("corpus/cantrbry/alice29.txt",
            wrapper, scenarios, level, windowBits, memLevel, strategy);
}

TEST_F(ZlibTest, Version) {
    static const char* myVersion = ZLIB_VERSION;

    // assert compatible zlib
    ASSERT_EQ(zlibVersion()[0], myVersion[0]);
    if (zlibVersion()[0] != myVersion[0]) {
        fprintf(stderr, "incompatible zlib version\n");
        exit(1);

    } else if (strcmp(zlibVersion(), ZLIB_VERSION) != 0) {
        fprintf(stderr, "warning: different zlib version\n");
    }

    uLong compile_flags = zlibCompileFlags();
    printf("zlib version %s = 0x%04x, compile flags = 0x%lx\n",
            ZLIB_VERSION, ZLIB_VERNUM, zlibCompileFlags());

    // Neil's unit testing was conducted on machine where
    // sizeof(uInt) = 4, sizeof(uLong) = 8,
    // sizeof(voidpf) = 8, sizeof(z_off_t) = 8,
    // allow debug flag
    // results may not hold for different machine
    // FIXME test on more machines
    EXPECT_TRUE(compile_flags == 0xA9 || compile_flags == 0x1A9);
}


TEST_F(ZlibTest, Compress) {
    zlib_test_alice();
}

TEST_F(ZlibTest, CompressGzip) {
    zlib_test_alice(WRAP_GZIP_BASIC);
}

TEST_F(ZlibTest, CompressGzipAlloced) {
    zlib_test_alice(WRAP_GZIP_BUFFERS);
}




TEST_F(ZlibTest, CompressGzipNull) {
    zlib_test_alice(WRAP_GZIP_NULL);
}

TEST_F(ZlibTest, CompressRaw) {
    zlib_test_alice(WRAP_NONE);
}

TEST_F(ZlibTest, CompressNone) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_BASIC, Z_NO_COMPRESSION);
}

TEST_F(ZlibTest, CompressBestSpeed) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_BASIC, Z_BEST_SPEED);
}

TEST_F(ZlibTest, CompressBestCompression) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_BASIC, Z_BEST_COMPRESSION);
}

TEST_F(ZlibTest, CompressFiltered) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_BASIC, Z_DEFAULT_COMPRESSION,
            DEF_WBITS, DEF_MEM_LEVEL, Z_FILTERED);
}

TEST_F(ZlibTest, CompressHuffmanOnly) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_BASIC, Z_DEFAULT_COMPRESSION,
            DEF_WBITS, DEF_MEM_LEVEL, Z_HUFFMAN_ONLY);
}

TEST_F(ZlibTest, CompressRLE) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_BASIC, Z_DEFAULT_COMPRESSION,
            DEF_WBITS, DEF_MEM_LEVEL, Z_RLE);
}

TEST_F(ZlibTest, CompressFixed) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_BASIC, Z_DEFAULT_COMPRESSION,
            DEF_WBITS, DEF_MEM_LEVEL, Z_FIXED);
}


#if DO_LONG_TESTS
TEST_F(ZlibTest, CorpusGzip) {
    for (int j = 0; j < NUM_CORPUS; j++) {
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_GZIP_BASIC);
    }
}

TEST_F(ZlibTest, CorpusNoComp) {
    for (int j = 0; j < NUM_CORPUS; j++) {
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_BASIC,
                Z_NO_COMPRESSION);
    }
}

TEST_F(ZlibTest, CorpusNoFlushNoComp) {
    for (int j = 0; j < NUM_CORPUS; j++) {
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_NO_FLUSH,
                Z_NO_COMPRESSION);
    }
}


TEST_F(ZlibTest, CorpusLevels) {
    for (int j = 0; j < NUM_CORPUS; j++) {
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_BASIC,
                Z_BEST_SPEED);
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_BASIC,
                Z_DEFAULT_COMPRESSION);
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_BASIC,
                Z_BEST_COMPRESSION);
    }
}

TEST_F(ZlibTest, AliceAllLevels) {
    for (int j = Z_BEST_SPEED; j <= Z_BEST_COMPRESSION; j++) {
        printf("\n");
        zlib_test_alice(WRAP_ZLIB, SCENARIO_BASIC, j);
    }
}

TEST_F(ZlibTest, CorpusFullFlush) {
    for (int j = 0; j < NUM_CORPUS; j++) {
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_FULL_FLUSH);
    }
}

TEST_F(ZlibTest, CorpusPartialFlush) {
    for (int j = 0; j < NUM_CORPUS; j++) {
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_PARTIAL_FLUSH);
    }
}

#endif

TEST_F(ZlibTest, Dictionary) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_DICTIONARY);
}

TEST_F(ZlibTest, Pending) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_PENDING);
}

TEST_F(ZlibTest, Tune) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_TUNE);
}


TEST_F(ZlibTest, Params) {
    // change params after first full flush
    zlib_test_alice(WRAP_ZLIB, SCENARIO_PARAMS | SCENARIO_FULL_FLUSH);
}

TEST_F(ZlibTest, ParamsFullFlushFromNoCompression) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_PARAMS | SCENARIO_FULL_FLUSH,
            Z_NO_COMPRESSION);
}


TEST_F(ZlibTest, FullFlush) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_FULL_FLUSH);
}

TEST_F(ZlibTest, PartialFlush) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_PARTIAL_FLUSH);
}

TEST_F(ZlibTest, SyncFlush) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_SYNC_FLUSH);
}

TEST_F(ZlibTest, NoFlush) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_NO_FLUSH);
}

TEST_F(ZlibTest, NoFlushInflateBlock) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_NO_FLUSH | SCENARIO_INFLATE_BLOCK);
}

TEST_F(ZlibTest, NoFlushHuff) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_NO_FLUSH, Z_DEFAULT_COMPRESSION,
            DEF_WBITS, DEF_MEM_LEVEL, Z_HUFFMAN_ONLY);
}

TEST_F(ZlibTest, NoFlushRLE) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_NO_FLUSH, Z_DEFAULT_COMPRESSION,
            DEF_WBITS, DEF_MEM_LEVEL, Z_RLE);
}

TEST_F(ZlibTest, FullFlushRLE) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_FULL_FLUSH, Z_DEFAULT_COMPRESSION,
            DEF_WBITS, DEF_MEM_LEVEL, Z_RLE);
}

TEST_F(ZlibTest, FullFlushHuff) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_FULL_FLUSH, Z_DEFAULT_COMPRESSION,
            DEF_WBITS, DEF_MEM_LEVEL, Z_HUFFMAN_ONLY);
}

TEST_F(ZlibTest, FullFlushFast) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_FULL_FLUSH, Z_BEST_SPEED);
}

TEST_F(ZlibTest, NoFlushFast) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_NO_FLUSH, Z_BEST_SPEED);
}

TEST_F(ZlibTest, NoFlushFastInflateBlock) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_NO_FLUSH | SCENARIO_INFLATE_BLOCK,
            Z_BEST_SPEED);
}



TEST_F(ZlibTest, NoFlushNoComp) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_NO_FLUSH, Z_NO_COMPRESSION);
}

TEST_F(ZlibTest, PartialFlushNoComp) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_PARTIAL_FLUSH, Z_NO_COMPRESSION);
}

TEST_F(ZlibTest, SyncFlushNoComp) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_SYNC_FLUSH, Z_NO_COMPRESSION);
}

TEST_F(ZlibTest, FullFlushNoComp) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_FULL_FLUSH, Z_NO_COMPRESSION);
    zlib_test_alice(WRAP_GZIP_BASIC, SCENARIO_FULL_FLUSH, Z_NO_COMPRESSION);
    zlib_test_file("corpus/large/E.coli", WRAP_ZLIB, SCENARIO_FULL_FLUSH, Z_NO_COMPRESSION);
    zlib_test_file("corpus/large/E.coli", WRAP_GZIP_BASIC, SCENARIO_FULL_FLUSH, Z_NO_COMPRESSION);
}

TEST_F(ZlibTest, NoWrapPartialFlushNoComp) {
    zlib_test_alice(WRAP_NONE, SCENARIO_PARTIAL_FLUSH, Z_NO_COMPRESSION);
}

TEST_F(ZlibTest, NoWrapSyncFlushNoComp) {
    zlib_test_alice(WRAP_NONE, SCENARIO_SYNC_FLUSH, Z_NO_COMPRESSION);
}

TEST_F(ZlibTest, NoWrapFullFlushNoComp) {
    zlib_test_alice(WRAP_NONE, SCENARIO_FULL_FLUSH, Z_NO_COMPRESSION);
}

TEST_F(ZlibTest, Corrupt) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_CORRUPT);
}

TEST_F(ZlibTest, FullFlushCorrupt) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_FULL_FLUSH | SCENARIO_CORRUPT);
}

TEST_F(ZlibTest, TinyOutput) {
    zlib_test_alice(WRAP_GZIP_BUFFERS, SCENARIO_TINY);
}

TEST_F(ZlibTest, TinyOutputInflateBlock) {
    zlib_test_alice(WRAP_GZIP_BUFFERS, SCENARIO_TINY | SCENARIO_INFLATE_BLOCK);
}

TEST_F(ZlibTest, DictionaryWouldFillWindow) {
    zlib_test_alice(WRAP_ZLIB,
            SCENARIO_BIG_DICTIONARY | SCENARIO_TINY);
}

TEST_F(ZlibTest, DictionaryWouldFillWindowInflateBlock) {
    zlib_test_alice(WRAP_ZLIB,
            SCENARIO_BIG_DICTIONARY | SCENARIO_TINY | SCENARIO_INFLATE_BLOCK);
}

TEST_F(ZlibTest, NoWrapDictionaryWouldFillWindow) {
    zlib_test_alice(WRAP_NONE,
            SCENARIO_BIG_DICTIONARY | SCENARIO_TINY);
}

TEST_F(ZlibTest, NoWrapDictionaryWouldFillWindowInflateBlock) {
    zlib_test_alice(WRAP_NONE,
            SCENARIO_BIG_DICTIONARY | SCENARIO_TINY | SCENARIO_INFLATE_BLOCK);
}

TEST_F(ZlibTest, NoWrapReset) {
    zlib_test_alice(WRAP_NONE, SCENARIO_RESET);
}

TEST_F(ZlibTest, NoWrapResetInflateBlock) {
    zlib_test_alice(WRAP_NONE, SCENARIO_RESET | SCENARIO_INFLATE_BLOCK);
}

TEST_F(ZlibTest, Reset) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_RESET);
}

TEST_F(ZlibTest, ResetInflateBlock) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_RESET | SCENARIO_INFLATE_BLOCK);
}

//TEST_F(ZlibTest, ResetDiffBitsInflateBlock) {
//    zlib_test_alice(WRAP_ZLIB, SCENARIO_RESET_DIFF_BITS | SCENARIO_INFLATE_BLOCK);
//}


TEST_F(ZlibTest, GzipReset) {
    zlib_test_alice(WRAP_GZIP_BASIC, SCENARIO_RESET);
}

TEST_F(ZlibTest, GzipResetInflateBlock) {
    zlib_test_alice(WRAP_GZIP_BASIC, SCENARIO_RESET | SCENARIO_INFLATE_BLOCK);
}

TEST_F(ZlibTest, BigHeaderBufferSmallAvail) {
    zlib_test_alice(WRAP_GZIP_BUFFERS, SCENARIO_BIG_HEADER_BUFFERS_SMALL_AVAIL);
}

TEST_F(ZlibTest, InflateWindowBits0) {
    // default to window size from buffer
    zlib_test_alice(WRAP_ZLIB, SCENARIO_INFLATE_BITS0);
}

TEST_F(ZlibTest, GZipInflateWindowBits0) {
    // default to 15 for gzip
    zlib_test_alice(WRAP_GZIP_BASIC, SCENARIO_INFLATE_BITS0);
}


TEST_F(ZlibTest, CompressSafeErrors) {
    printf("test errors in compress_safe\n");


    FILE * file = fopen("corpus/cantrbry/alice29.txt", "r");
    ASSERT_FALSE(file == NULL);

    int source_buf_len = CORPUS_MAX_SIZE;
    Byte * source_buf = (Byte *) malloc(source_buf_len);
    int nread = fread(source_buf, 1, source_buf_len, file);
    ASSERT_FALSE(ferror(file));
    fclose(file);
    ASSERT_TRUE(nread <= source_buf_len);
    source_buf_len = nread;

    int err;
    int level = Z_DEFAULT_COMPRESSION;

    uLong compressed_buf_len;
    Byte * compressed_buf;
    uLong work_buf_len;
    Byte * work_buf;
    uLong compressed_buf_len_out;
    int max_block_size = 10000;

    err = zsc_compress_get_max_output_size(source_buf_len, max_block_size, level, &compressed_buf_len);
    EXPECT_EQ(err, Z_OK);
    compressed_buf = (Byte *) malloc(compressed_buf_len);
    ASSERT_NE(compressed_buf, (Bytef*)NULL);


    err = zsc_compress_get_min_work_buf_size(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    ASSERT_NE(work_buf, (Bytef*)NULL);
    compressed_buf_len_out = compressed_buf_len;

    printf("bad window and mem level\n");
    err = zsc_compress2(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len, max_block_size,
            work_buf, work_buf_len, Z_DEFAULT_COMPRESSION, 42, 1337, Z_DEFAULT_STRATEGY);
    EXPECT_EQ(err, Z_STREAM_ERROR);
    printf("zError:%s\n", zError(err));

    printf("bad strategy\n");
    err = zsc_compress2(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len, max_block_size,
            work_buf, work_buf_len, Z_DEFAULT_COMPRESSION, DEF_WBITS, DEF_MEM_LEVEL, 42);
    EXPECT_EQ(err, Z_STREAM_ERROR);
    printf("zError:%s\n", zError(err));


    printf("work buf size = 0\n");
    err = zsc_compress(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len, max_block_size,
            work_buf, 0, Z_DEFAULT_COMPRESSION);
    EXPECT_EQ(err, Z_MEM_ERROR);
    printf("zError:%s\n", zError(err));

    printf("output buf size small\n");
    compressed_buf_len_out = 42;
    err = zsc_compress(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len, max_block_size,
            work_buf, work_buf_len, Z_DEFAULT_COMPRESSION);
    EXPECT_EQ(err, Z_BUF_ERROR);
    printf("zError:%s\n", zError(err));


    free(compressed_buf);
    free(source_buf);
    free(work_buf);
}

TEST_F(ZlibTest, UncompressSafeErrors) {
    printf("test errors in uncompress_safe\n");


    FILE * file = fopen("corpus/cantrbry/alice29.txt", "r");
    ASSERT_FALSE(file == NULL);

    int source_buf_len = CORPUS_MAX_SIZE;
    Byte * source_buf = (Byte *) malloc(source_buf_len);
    int nread = fread(source_buf, 1, source_buf_len, file);
    ASSERT_FALSE(ferror(file));
    fclose(file);
    ASSERT_TRUE(nread <= source_buf_len);
    source_buf_len = nread;

    int err;

    uLong compressed_buf_len;
    Byte * compressed_buf;
    uLong work_buf_len;
    Byte * work_buf;
    uLong compressed_buf_len_out;
    int max_block_size = 10000;

    int level = Z_DEFAULT_COMPRESSION;

    err = zsc_compress_get_max_output_size(source_buf_len, max_block_size,
            level, &compressed_buf_len);
    EXPECT_EQ(err, Z_OK);
    compressed_buf = (Byte *) malloc(compressed_buf_len);
    ASSERT_NE(compressed_buf, (Bytef*)NULL);


    err = zsc_compress_get_min_work_buf_size(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    ASSERT_NE(work_buf, (Bytef*)NULL);
    compressed_buf_len_out = compressed_buf_len;

    err = zsc_compress(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len,  max_block_size,
            work_buf, work_buf_len, level);
    EXPECT_EQ(err, Z_OK);

    free(work_buf);

    uLong uncompressed_buf_len = source_buf_len;
    Byte * uncompressed_buf = (Byte *) malloc(uncompressed_buf_len);
    EXPECT_NE(uncompressed_buf, (Byte * )NULL);
    err = zsc_uncompress_get_min_work_buf_size(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);

    uLong compressed_buf_len_out2;
    uLong uncompressed_buf_len_out;

    printf("work buf size = 0\n");
    compressed_buf_len_out2 = compressed_buf_len_out;
    uncompressed_buf_len_out = uncompressed_buf_len;
    err = zsc_uncompress(uncompressed_buf, &uncompressed_buf_len_out,
                            compressed_buf, &compressed_buf_len_out2,
                            work_buf, 0);
    EXPECT_EQ(err, Z_MEM_ERROR);
    printf("zError:%s\n", zError(err));

    printf("bad window bits arg\n");
    compressed_buf_len_out2 = compressed_buf_len_out;
    uncompressed_buf_len_out = uncompressed_buf_len;
    err = zsc_uncompress_safe2(uncompressed_buf, &uncompressed_buf_len_out,
                            compressed_buf, &compressed_buf_len_out2,
                            work_buf, work_buf_len, 500);
    EXPECT_EQ(err, Z_STREAM_ERROR);
    printf("zError:%s\n", zError(err));

    printf("output buf size small\n");
    compressed_buf_len_out2 = compressed_buf_len_out;
    uncompressed_buf_len_out = 42;
    err = zsc_uncompress(uncompressed_buf, &uncompressed_buf_len_out,
                            compressed_buf, &compressed_buf_len_out2,
                            work_buf, work_buf_len);
    EXPECT_EQ(err, Z_BUF_ERROR);
    printf("zError:%s\n", zError(err));

    printf("gzip header when not gzip\n");
    gz_header header;
    memset(&header, 0, sizeof(header));
    compressed_buf_len_out2 = compressed_buf_len_out;
    uncompressed_buf_len_out = uncompressed_buf_len;
    err = zsc_uncompress_safe_gzip2(uncompressed_buf, &uncompressed_buf_len_out,
                            compressed_buf, &compressed_buf_len_out2,
                            work_buf, work_buf_len, DEF_WBITS, &header);
    EXPECT_EQ(err, Z_STREAM_ERROR);
    printf("zError:%s\n", zError(err));

}

TEST_F(ZlibTest, DeflateErrors) {
    printf("test errors in deflate.c\n");

    int err;

    printf("null version gives error\n");
    err = deflateInit2_(Z_NULL, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
            DEF_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
            Z_NULL, sizeof(z_stream));
    EXPECT_EQ(err, Z_VERSION_ERROR);

    printf("bad version gives error\n");
    err = deflateInit2_(Z_NULL, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
            DEF_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
            "2.0.0.0", sizeof(z_stream));
    EXPECT_EQ(err, Z_VERSION_ERROR);

    printf("bad stream size gives error\n");
    err = deflateInit2_(Z_NULL, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
            DEF_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
            ZLIB_VERSION, sizeof(z_stream)+1);
    EXPECT_EQ(err, Z_VERSION_ERROR);

    printf("null stream gives error\n");
    err = deflateInit2(Z_NULL, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
            DEF_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    printf("null alloc gives error\n");

    uLong work_buf_size = (uLong)(10000);
    err = zsc_compress_get_min_work_buf_size2(DEF_WBITS, DEF_MEM_LEVEL,
            &work_buf_size);
    ASSERT_EQ(err, Z_OK);
    Bytef *work_buf = (Bytef*)malloc(work_buf_size);
    ASSERT_NE(work_buf, (Bytef*)NULL);


//    z_static_mem mem;
//    memset(&mem, 0, sizeof(mem));
//    mem.work = work_buf;
//    mem.work_len = work_buf_size;
//    mem.work_alloced = 0;
//
//    z_stream stream;
//    memset(&stream, 0, sizeof(stream));
//    stream.zalloc = (alloc_func)0;
//    stream.zfree = (free_func)0;
//    stream.opaque = (voidpf)0;
//
//    err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
//            DEF_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
//    EXPECT_EQ(err, Z_STREAM_ERROR);
//
//
//    printf("null free gives error\n");
//    memset(&stream, 0, sizeof(stream));
//    stream.zalloc = (alloc_func)z_static_alloc;
//    stream.zfree = (free_func)0;
//    stream.opaque = (voidpf)0;
//
//    err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
//            DEF_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
//    EXPECT_EQ(err, Z_STREAM_ERROR);
//
//
//    printf("null allocation gives error\n");
//    memset(&mem, 0, sizeof(mem));
//    mem.work = work_buf;
//    mem.work_len = work_buf_size;
//    mem.work_alloced = 0;
//    memset(&stream, 0, sizeof(stream));
//    stream.zalloc = (alloc_func)zlib_test_bad_alloc;
//    stream.zfree = (free_func)z_static_free;
//    stream.opaque = (voidpf)&mem;
//
//    good_allocs_allowed = 0;
//
//    err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
//            DEF_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
//    EXPECT_EQ(err, Z_MEM_ERROR);
//
//
//    memset(&mem, 0, sizeof(mem));
//    mem.work = work_buf;
//    mem.work_len = work_buf_size;
//    mem.work_alloced = 0;
//    memset(&stream, 0, sizeof(stream));
//    stream.zalloc = (alloc_func)zlib_test_bad_alloc;
//    stream.zfree = (free_func)z_static_free;
//    stream.opaque = (voidpf)&mem;
//
//    good_allocs_allowed = 1;
//
//    err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
//            DEF_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
//    EXPECT_EQ(err, Z_MEM_ERROR);




    printf("deflate fns should fail with null stream\n");
    // FIXME make assert eventually

    err = deflateSetDictionary(Z_NULL, Z_NULL, 0);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = deflateGetDictionary(Z_NULL, Z_NULL, Z_NULL);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = deflateResetKeep(Z_NULL);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = deflateReset(Z_NULL);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = deflateSetHeader(Z_NULL, Z_NULL);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = deflatePending(Z_NULL, Z_NULL, Z_NULL);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = deflatePrime(Z_NULL, 0, 0);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = deflateParams(Z_NULL, Z_DEFAULT_COMPRESSION, Z_DEFAULT_STRATEGY);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = deflateTune(Z_NULL, 0, 0, 0, 0);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    uLong bound = deflateBound(Z_NULL, 424242);
    EXPECT_EQ(bound, 424242
            + ((424242 + 7) >> 3) + ((424242 + 63) >> 6) +5 + 6);

    err = deflate(Z_NULL, Z_NO_FLUSH);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    free(work_buf);

}

TEST_F(ZlibTest, InflateErrors) {
    printf("test errors in inflate.c\n");

    int err;

    printf("null version gives error\n");
    err = inflateInit2_(Z_NULL, DEF_WBITS,
            Z_NULL, sizeof(z_stream));
    EXPECT_EQ(err, Z_VERSION_ERROR);

    printf("bad version gives error\n");
    err = inflateInit2_(Z_NULL, DEF_WBITS,
            "2.0.0.0", sizeof(z_stream));
    EXPECT_EQ(err, Z_VERSION_ERROR);

    printf("bad stream size gives error\n");
    err = inflateInit2_(Z_NULL, DEF_WBITS,
            ZLIB_VERSION, sizeof(z_stream)+1);
    EXPECT_EQ(err, Z_VERSION_ERROR);

    printf("null stream gives error\n");
    err = inflateInit2(Z_NULL, DEF_WBITS);
    EXPECT_EQ(err, Z_STREAM_ERROR);


    printf("null alloc gives error\n");

    uLong work_buf_size = (uLong)(10000);
    err = zsc_compress_get_min_work_buf_size2(DEF_WBITS, DEF_MEM_LEVEL,
            &work_buf_size);
    ASSERT_EQ(err, Z_OK);
    Bytef *work_buf = (Bytef*)malloc(work_buf_size);
    ASSERT_NE(work_buf, (Bytef*)NULL);

    uLong out_buf_size = (uLong)(10000);
    Bytef *out_buf = (Bytef*)malloc(out_buf_size);
    ASSERT_NE(out_buf, (Bytef*)NULL);


//    z_static_mem mem;
//    memset(&mem, 0, sizeof(mem));
//    mem.work = work_buf;
//    mem.work_len = work_buf_size;
//    mem.work_alloced = 0;
//
//    z_stream stream;
//    stream.zalloc = (alloc_func)Z_NULL;
//    stream.zfree = (free_func)Z_NULL;
//    stream.opaque = (voidpf) Z_NULL;
//
//    err = inflateInit2(&stream, DEF_WBITS);
//    EXPECT_EQ(err, Z_STREAM_ERROR);
//
//    printf("null free gives error\n");
//    stream.zalloc = z_static_alloc;
//    err = inflateInit2(&stream, DEF_WBITS);
//    EXPECT_EQ(err, Z_STREAM_ERROR);
//
//
//    printf("null allocation gives error\n");
//    memset(&mem, 0, sizeof(mem));
//    mem.work = work_buf;
//    mem.work_len = work_buf_size;
//    mem.work_alloced = 0;
//    memset(&stream, 0, sizeof(stream));
//    stream.zalloc = (alloc_func)zlib_test_bad_alloc;
//    stream.zfree = (free_func)z_static_free;
//    stream.opaque = (voidpf)&mem;
//    err = inflateInit2(&stream, DEF_WBITS);
//    EXPECT_EQ(err, Z_MEM_ERROR);

    printf("init with bad windowbits gives error\n");
    z_stream stream;
//    memset(&mem, 0, sizeof(mem));
//    mem.work = work_buf;
//    mem.work_len = work_buf_size;
//    mem.work_alloced = 0;
    memset(&stream, 0, sizeof(stream));
//    stream.zalloc = (alloc_func)z_static_alloc;
//    stream.zfree = (free_func)z_static_free;
//    stream.opaque = (voidpf)&mem;
    stream.next_work = work_buf;
    stream.avail_work = work_buf_size;
    err = inflateInit2(&stream, 2);
    EXPECT_EQ(err, Z_STREAM_ERROR);

//    memset(&mem, 0, sizeof(mem));
//    mem.work = work_buf;
//    mem.work_len = work_buf_size;
//    mem.work_alloced = 0;
    memset(&stream, 0, sizeof(stream));
//    stream.zalloc = (alloc_func)z_static_alloc;
//    stream.zfree = (free_func)z_static_free;
//    stream.opaque = (voidpf)&mem;
    stream.next_work = work_buf;
    stream.avail_work = work_buf_size;
    err = inflateInit2(&stream, DEF_WBITS);
    EXPECT_EQ(err, Z_OK);


    printf("reset with different windowbits and allocated window gives error\n");
    struct inflate_state FAR *state;
    state = (struct inflate_state FAR *)stream.state;
    state->window = work_buf;
    err = inflateReset2(&stream, DEF_WBITS-1);
    EXPECT_EQ(err, Z_STREAM_ERROR);


    printf("null stream gives error\n");
    err = inflate(Z_NULL, Z_SYNC_FLUSH);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    printf("stream with null state gives error\n");

    z_stream stream_null_state;
//    stream.zalloc = (alloc_func)z_static_alloc;
//    stream.zfree = (free_func)z_static_free;
//    stream.opaque = (voidpf)&mem;
    stream_null_state.state = Z_NULL;
    err = inflate(&stream_null_state, Z_SYNC_FLUSH);
    EXPECT_EQ(err, Z_STREAM_ERROR);


    printf("incorrect header check\n");

    uInt header = (Z_DEFLATED + ((DEF_WBITS-8)<<4)) << 8;
    uInt level_flags = 2;
    header |= (level_flags << 6);
    header += 31 - (header % 31);
    header++; // BAD!
    Byte header_arr[2];
    header_arr[0] = (Byte)(header >> 8);
    header_arr[1] = (Byte)(header & 0xFF);
    printf("header = 0x%02x 0x%02x\n", header_arr[0], header_arr[1]);

    uLong out_buf_len_out = out_buf_size;
    uLong source_len_out = 2;
    err = zsc_uncompress(out_buf, &out_buf_len_out,
            header_arr, &source_len_out,
                            work_buf, work_buf_size);
    EXPECT_EQ(err, Z_DATA_ERROR);

    printf("unknown compression method\n");

    header = (5 /* BAD*/ + ((DEF_WBITS-8)<<4)) << 8;
    level_flags = 2;
    header |= (level_flags << 6);
    header += 31 - (header % 31);
    header_arr[0] = (Byte)(header >> 8);
    header_arr[1] = (Byte)(header & 0xFF);

    printf("header = 0x%02x 0x%02x\n", header_arr[0], header_arr[1]);

    out_buf_len_out = out_buf_size;
    source_len_out = 2;
    err = zsc_uncompress(out_buf, &out_buf_len_out,
            header_arr, &source_len_out,
                            work_buf, work_buf_size);
    EXPECT_EQ(err, Z_DATA_ERROR);

    printf("bad window size\n");

    header = (Z_DEFLATED + ((16/*BAD*/-8)<<4)) << 8;
    level_flags = 2;
    header |= (level_flags << 6);
    header += 31 - (header % 31);
    header_arr[0] = (Byte)(header >> 8);
    header_arr[1] = (Byte)(header & 0xFF);

    printf("header = 0x%02x 0x%02x\n", header_arr[0], header_arr[1]);

    out_buf_len_out = out_buf_size;
    source_len_out = 2;
    err = zsc_uncompress(out_buf, &out_buf_len_out,
            header_arr, &source_len_out,
                            work_buf, work_buf_size);
    EXPECT_EQ(err, Z_DATA_ERROR);


    printf("incorrect header check, gzip\n");

    Byte header_arr4[4];
    header_arr4[0] = 31;
    header_arr4[1] = 139+1;
    header_arr4[2] = 8;
    header_arr4[3] = 0;
    printf("header = 0x%02x 0x%02x 0x%02x 0x%02x\n",
            header_arr4[0], header_arr4[1], header_arr4[2], header_arr4[3]);

    out_buf_len_out = out_buf_size;
    source_len_out = 4;
    err = zsc_uncompress_safe_gzip(out_buf, &out_buf_len_out,
            header_arr4, &source_len_out,
                            work_buf, work_buf_size, Z_NULL);
    EXPECT_EQ(err, Z_DATA_ERROR);

    printf("unknown compression method, gzip\n");

    header_arr4[0] = 31;
    header_arr4[1] = 139;
    header_arr4[2] = 5;
    header_arr4[3] = 0;
    printf("header = 0x%02x 0x%02x 0x%02x 0x%02x\n",
            header_arr4[0], header_arr4[1], header_arr4[2], header_arr4[3]);

    out_buf_len_out = out_buf_size;
    source_len_out = 4;
    err = zsc_uncompress_safe_gzip(out_buf, &out_buf_len_out,
            header_arr4, &source_len_out,
                            work_buf, work_buf_size, Z_NULL);
    EXPECT_EQ(err, Z_DATA_ERROR);

    printf("unknown gzip flags\n");

    header_arr4[0] = 31;
    header_arr4[1] = 139;
    header_arr4[2] = 8;
    header_arr4[3] = 255;
    printf("header = 0x%02x 0x%02x 0x%02x 0x%02x\n",
            header_arr4[0], header_arr4[1], header_arr4[2], header_arr4[3]);

    out_buf_len_out = out_buf_size;
    source_len_out = 4;
    err = zsc_uncompress_safe_gzip(out_buf, &out_buf_len_out,
            header_arr4, &source_len_out,
            work_buf, work_buf_size, Z_NULL);
    EXPECT_EQ(err, Z_DATA_ERROR);

//    printf("bad gzip crc\n");
//
//    Byte header_arr_bad_crc[] = { 31, 139, 8, 2,
//            0, 0, 0, 0, // time
//            0, // level
//            0, // os
//
//            };
//    header_arr4[0] = 31;
//    header_arr4[1] = 139 + 1;
//    header_arr4[2] = 8;
//    header_arr4[3] = 0;
//    printf("header = 0x%02x 0x%02x 0x%02x 0x%02x\n",
//            header_arr4[0], header_arr4[1], header_arr4[2], header_arr4[3]);
//
//    out_buf_len_out = out_buf_size;
//    source_len_out = 4;
//    err = uncompressSafeGzip(out_buf, &out_buf_len_out,
//            header_arr4, &source_len_out,
//            work_buf, work_buf_size, Z_NULL);
//    EXPECT_EQ(err, Z_DATA_ERROR);

    printf("null stream as function input\n");
    err = inflateEnd(Z_NULL);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = inflateValidate(Z_NULL, 0);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = inflateUndermine(Z_NULL, 0);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = inflateSyncPoint(Z_NULL);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = inflateGetHeader(Z_NULL, Z_NULL);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = inflateSetDictionary(Z_NULL,Z_NULL, 0);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = inflateEnd(Z_NULL);
    EXPECT_EQ(err, Z_STREAM_ERROR);



    free(work_buf);
    free(out_buf);

}


TEST_F(ZlibTest, DeflatePrime) {

    int err;

    err = deflatePrime(Z_NULL, 5, 31);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    int windowBits = -15;

    uLong work_buf_len;
    Byte * work_buf;

    Byte outbuf[256];


    err = zsc_compress_get_min_work_buf_size(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    ASSERT_NE(work_buf, (Byte *)Z_NULL);

//    z_static_mem mem_inf;
//    mem_inf.work = work_buf;
//    mem_inf.work_len = work_buf_len;
//    mem_inf.work_alloced = 0;

    z_stream stream;
    stream.next_work = work_buf;
    stream.avail_work = work_buf_len;
    stream.next_in = Z_NULL;
    stream.avail_in = 0;
//    stream.zalloc = z_static_alloc;
//    stream.zfree = z_static_free;
//    stream.opaque = (voidpf) &mem_inf;

    err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits,
            DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    EXPECT_EQ(err, Z_OK);

    stream.next_out = outbuf;
    stream.avail_out = 256;

    err = deflatePrime(&stream,5, 31);
    EXPECT_EQ(err, Z_OK);

    err = deflatePrime(&stream,11, 31);
    EXPECT_EQ(err, Z_OK);

    err = deflatePrime(&stream,1, 1);
    EXPECT_EQ(err, Z_OK);


    free(work_buf);

}

TEST_F(ZlibTest, InflatePrime) {

    int err;

    err = inflatePrime(Z_NULL, 5, 31);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    int windowBits = -15;

    uLong work_buf_len;
    Byte * work_buf;

    err = zsc_uncompress_get_min_work_buf_size(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    ASSERT_NE(work_buf, (Byte *)Z_NULL);

//    z_static_mem mem_inf;
//    mem_inf.work = work_buf;
//    mem_inf.work_len = work_buf_len;
//    mem_inf.work_alloced = 0;

    z_stream stream;

    stream.next_work = work_buf;
    stream.avail_work = work_buf_len;
    stream.next_in = Z_NULL;
    stream.avail_in = 0;
//    stream.zalloc = z_static_alloc;
//    stream.zfree = z_static_free;
//    stream.opaque = (voidpf) &mem_inf;

    err = inflateInit2(&stream, windowBits);
    EXPECT_EQ(err, Z_OK);

    err = inflatePrime(&stream, 5, 31);
    EXPECT_EQ(err, Z_OK);
    err = inflatePrime(&stream, -1, 0);
    EXPECT_EQ(err, Z_OK);

    // can't do more than 16 bits at a time
    err = inflatePrime(&stream, 17, 42);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    // can't accumulate more than 32 bits
    err = inflatePrime(&stream, 16, 42);
    EXPECT_EQ(err, Z_OK);
    err = inflatePrime(&stream, 15, 42);
    EXPECT_EQ(err, Z_OK);
    err = inflatePrime(&stream, 1, 1);
    EXPECT_EQ(err, Z_OK);
    err = inflatePrime(&stream, 1, 1);
    EXPECT_EQ(err, Z_STREAM_ERROR);


    free(work_buf);


}

TEST_F(ZlibTest, InflateUndocumented) {
    printf("cover undocumented inflate functions\n");

    int err;

    err = inflateUndermine(Z_NULL, 42);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    err = inflateValidate(Z_NULL, true);
    EXPECT_EQ(err, Z_STREAM_ERROR);

    long mark;

    mark = inflateMark(Z_NULL);
    EXPECT_EQ(mark, -(1L << 16));

    unsigned long codes_used;

    codes_used = inflateCodesUsed(Z_NULL);
    EXPECT_EQ(codes_used, (unsigned long)-1);


}

TEST_F(ZlibTest, GetCRCTable) {
    const z_crc_t * table = get_crc_table();
    EXPECT_EQ(table[0], 0x00000000UL);
    EXPECT_EQ(table[1], 0x77073096UL);
    EXPECT_EQ(table[255], 0x2d02ef8dUL);

}


TEST_F(ZlibTest, CRC32Combine) {
    unsigned char buf[] = { 5, 7, 21, 17, 35, 77, 201, 170, 85, 14 };

    unsigned int crc3;
    unsigned int crc7;
    unsigned int crc4;
    unsigned int crc6;
    unsigned int crc37;
    unsigned int crc46;

    crc3 = crc32(0, NULL, 0);
    crc3 = crc32(crc3, buf, 3);

    crc7 = crc32(0, NULL, 0);
    crc7 = crc32(crc7, buf+3, 7);

    crc4 = crc32(0, NULL, 0);
    crc4 = crc32(crc4, buf, 4);

    crc6 = crc32(0, NULL, 0);
    crc6 = crc32(crc6, buf+4, 6);

    crc37 = crc32_combine64(crc3, crc7, 7);

    crc46 = crc32_combine(crc4, crc6, 6);

    EXPECT_EQ(crc37, crc46);

// FIXME try again or delete
//    crc3 = crc32_big(0, NULL, 0);
//    crc3 = crc32_big(crc3, buf, 3);
//
//    crc3 = crc32_little(0, NULL, 0);
//    crc3 = crc32_little(crc3, buf, 3);




    unsigned int adler1;
    unsigned int adler9;
    unsigned int adler3;
    unsigned int adler7;
    unsigned int adler4;
    unsigned int adler6;
    unsigned int adler19;
    unsigned int adler37;
    unsigned int adler46;

    adler1 = adler32(0, NULL, 0);
    adler1 = adler32(adler1, buf, 1);

    adler9 = adler32(0, NULL, 0);
    adler9 = adler32(adler9, buf+1, 9);

    adler3 = adler32(0, NULL, 0);
    adler3 = adler32(adler3, buf, 3);

    adler7 = adler32(0, NULL, 0);
    adler7 = adler32(adler7, buf+3, 7);

    adler4 = adler32(0, NULL, 0);
    adler4 = adler32(adler4, buf, 4);

    adler6 = adler32(0, NULL, 0);
    adler6 = adler32(adler6, buf+4, 6);

    adler19 = adler32_combine(adler1, adler9, 9);

    adler37 = adler32_combine(adler3, adler7, 7);

    adler46 = adler32_combine64(adler4, adler6, 6);

    EXPECT_EQ(adler37, adler46);
    EXPECT_EQ(adler19, adler46);

    // adler greater than BASE
    (void)adler32(65522, buf, 1);
    (void)adler32(65522, buf, 2);

    unsigned char buf2[] = { 20, 10 };
    (void)adler32(65500, buf, 2);

    // combine should have len >= 0
    EXPECT_EQ(adler32_combine(87, 42, -1), 0xffffffff);



}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

