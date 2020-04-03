/* example.c -- usage example of the zlib compression library
 * Copyright (C) 1995-2006, 2011, 2016 Jean-loup Gailly
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#include "zlib.h"
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
#define DO_CORPUS_TESTS 1


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
    SCENARIO_PARAMS = 1 << 2,     // adjust params
    SCENARIO_FULL_FLUSH = 1 << 3, // adjust params
} ZlibTestScenario;

enum {NUM_CORPUS = 20}; //keep in sync

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


static z_const char hello[] = "hello, hello!";
/* "hello world" would be more standard, but the repeated "hello"
 * stresses the compression code better, sorry...
 */

static const char dictionary[] = "hello";
static uLong dictId;    /* Adler32 value of the dictionary */


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




void zlib_test(
        Byte * source_buf,
        int source_buf_len,
        WrapType wrapper = WRAP_ZLIB,
        unsigned long scenarios = 0, // bit mask of scenarios
        int level = Z_DEFAULT_COMPRESSION,
        int windowBits = DEF_WBITS,
        int memLevel = DEF_MEM_LEVEL,
        int strategy = Z_DEFAULT_STRATEGY)
{

    printf("Zlib Test: buf_len = %d, wrapper = %d, scenarios = %#lx, "
            "level = %d, windowbits = %d, memLevel = %d, strategy = %d.\n",
            source_buf_len, wrapper, scenarios,
            level, windowBits, memLevel, strategy);

    int err;

//    FILE * file = fopen("corpus/cantrbry/alice29.txt", "r");
//    ASSERT_FALSE(file == NULL);
//
//    int source_buf_len = 160000;
//    Byte * source_buf = (Byte *) malloc(source_buf_len);
//    int nread = fread(source_buf, 1, source_buf_len, file);
//    ASSERT_FALSE(ferror(file));
//    fclose(file);
//    printf("Read %d bytes.\n", nread);
//    ASSERT_TRUE(nread <= source_buf_len);
//    source_buf_len = nread;

    // if all params are default, we'll use more basic functions
    bool all_params_default =
            (/*level == Z_DEFAULT_COMPRESSION
             &&*/ windowBits == DEF_WBITS
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
        head_in.comment = (Bytef*) "I am the man with no name.";
        head_in.name = (Bytef*) "Zapp Brannigan.";
        head_in.extra = (Bytef*) "Hello.";
        head_in.extra_len = strlen((char*) head_in.extra);
    }
    head_out.name = (Byte *)malloc(80);
    head_out.name_max = 80;
    head_out.comment = (Byte *)malloc(80);
    head_out.comm_max = 80;
    head_out.extra = (Byte *)malloc(80);
    head_out.extra_max = 80;

    const Bytef * dictionary = (const Bytef *)
            "queen"
                    "time"
                    "thought"
                    "could"
                    "and the"
                    "in the"
                    "would"
                    "went"
                    "like"
                    "know"
                    "in a"
                    "one"
                    "said Alice"
                    "little"
                    "of the"
                    "said the"
                    "Alice"
                    "said";
    int dictLength = strlen((char*) dictionary);

    // adjust window bits
    switch (wrapper) {
    case WRAP_NONE:
        // if not already encoded, signify no wrapper
        if (windowBits > 0) {
            windowBits = -windowBits;
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

    // get max output size
    switch(wrapper) {
    case WRAP_NONE:
        err = compressGetMaxOutputSize2(source_buf_len, windowBits, memLevel,
                &compressed_buf_len);
        break;
    case WRAP_ZLIB:
        if(all_params_default) {
            err = compressGetMaxOutputSize(source_buf_len, &compressed_buf_len);
        } else {
            err = compressGetMaxOutputSize2(source_buf_len, windowBits,
                    memLevel, &compressed_buf_len);
        }
        break;
    case WRAP_GZIP_NULL:
        err = compressGetMaxOutputSizeGzip2(source_buf_len, windowBits,
                memLevel, Z_NULL, &compressed_buf_len);
        break;
    case WRAP_GZIP_BASIC:
    case WRAP_GZIP_BUFFERS:
        if(all_params_default) {
            err = compressGetMaxOutputSizeGzip(source_buf_len, &head_in,
                    &compressed_buf_len);
        } else {
            err = compressGetMaxOutputSizeGzip2(source_buf_len, windowBits,
                memLevel, Z_NULL, &compressed_buf_len);
        }
        break;
    }

    EXPECT_EQ(err, Z_OK);
    compressed_buf = (Byte *) malloc(compressed_buf_len);
    ASSERT_NE(compressed_buf, (Bytef*)NULL);

    err = all_params_default ?
            compressGetMinWorkBufSize(&work_buf_len) :
            compressGetMinWorkBufSize2(windowBits, memLevel, &work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    ASSERT_NE(work_buf, (Bytef*)NULL);
    compressed_buf_len_out = compressed_buf_len;

    printf("Compressed buf size: %lu. Work buf size: %lu.\n",
            compressed_buf_len, work_buf_len);

    if(scenarios == SCENARIO_BASIC) {
        // use compress fn
        switch (wrapper) {
        case WRAP_NONE:
            err = compressSafe2(compressed_buf, &compressed_buf_len_out,
                    source_buf, source_buf_len,
                    work_buf, work_buf_len, level,
                    windowBits, memLevel, strategy);
            break;
        case WRAP_ZLIB:
            if (all_params_default) {
                err = compressSafe(compressed_buf, &compressed_buf_len_out,
                        source_buf, source_buf_len,
                        work_buf, work_buf_len, level);
            } else {
                err = compressSafe2(compressed_buf, &compressed_buf_len_out,
                        source_buf, source_buf_len,
                        work_buf, work_buf_len, level,
                        windowBits, memLevel, strategy);
            }
            break;
        case WRAP_GZIP_NULL:
        case WRAP_GZIP_BASIC:
        case WRAP_GZIP_BUFFERS:
            if (all_params_default) {
                err = compressSafeGzip(compressed_buf, &compressed_buf_len_out,
                        source_buf, source_buf_len,
                        work_buf, work_buf_len, level, &head_in);
            } else {
                err = compressSafeGzip2(compressed_buf, &compressed_buf_len_out,
                        source_buf, source_buf_len,
                        work_buf, work_buf_len, level,
                        windowBits, memLevel, strategy, &head_in);
            }
            break;
        }
    } else {
        // use deflate fns

        z_static_mem mem;
        mem.work = work_buf;
        mem.workLen = work_buf_len;
        mem.workAlloced = 0;

        z_stream stream;
        stream.zalloc = z_static_alloc;
        stream.zfree = z_static_free;
        stream.opaque = (voidpf) &mem;

        if (all_params_default) {
            err = deflateInit(&stream, level);
        } else {
            err = deflateInit2(&stream, level, Z_DEFLATED,
                    windowBits, memLevel, strategy);
        }
        EXPECT_EQ(err, Z_OK);


        if(scenarios & SCENARIO_DICTIONARY) {
            printf("setting dictionary\n");
            err = deflateSetDictionary(&stream, dictionary, dictLength);
            EXPECT_EQ(err, Z_OK);
        }

        unsigned pending_bytes;
        int pending_bits;
        if(scenarios & SCENARIO_PENDING) {
            err = deflatePending(&stream,&pending_bytes, &pending_bits);
            EXPECT_EQ(err, Z_OK);
            printf("pending bytes: %u pending bits: %d.\n",
                    pending_bytes, pending_bits);
        }

        if(scenarios & SCENARIO_PARAMS) {
            err = deflateParams(&stream, 8, Z_HUFFMAN_ONLY);
            EXPECT_EQ(err, Z_OK);
        }


        uInt max_in = (uInt)-1;
        uInt max_out = (uInt)-1;
        uLong bytes_left_dest =  compressed_buf_len_out;
        uLong bytes_left_source =  source_buf_len;


        stream.next_out = compressed_buf;
        stream.avail_out = 0;
        stream.next_in = source_buf;
        stream.avail_in = 0;

        int flush_type = Z_NO_FLUSH;

        if(scenarios & SCENARIO_FULL_FLUSH) {
            printf("doing full flush\n");
            flush_type = Z_FULL_FLUSH;
            max_in = 10000;
            max_out = 10000;
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
            err = deflate(&stream, bytes_left_source ? Z_NO_FLUSH : Z_FINISH);
        } while (err == Z_OK);

        if(cycles > 1) {
            printf("%d cycles\n", cycles);
        }

        compressed_buf_len_out = stream.total_out;

//        // compress in one fell swoop
//        printf("deflating\n");
//        err = deflate(&stream, Z_FINISH);
//        EXPECT_EQ(err, Z_STREAM_END);
//
//        compressed_buf_len_out = stream.total_out;
//
//        if(scenarios & SCENARIO_PENDING) {
//            err = deflatePending(&stream,&pending_bytes, &pending_bits);
//            EXPECT_EQ(err, Z_OK);
//            printf("pending bytes: %u pending bits: %d.\n",
//                    pending_bytes, pending_bits);
//        }


        if(scenarios & SCENARIO_DICTIONARY) {
            Bytef * dictionary_out = (Bytef *) malloc(32768);
            ASSERT_NE(dictionary_out, (Bytef*)NULL);

            uInt dict_out_len = 0;

            printf("getting dictionary\n");
            err = deflateGetDictionary(&stream, dictionary_out, &dict_out_len);
            EXPECT_EQ(err, Z_OK);

            free(dictionary_out);
        }

        err = deflateEnd(&stream);
        EXPECT_EQ(err, Z_OK);

        if(scenarios & SCENARIO_PENDING) {
            err = deflatePending(&stream,&pending_bytes, &pending_bits);
            EXPECT_EQ(err, Z_STREAM_ERROR);
        }

    } // end else scenarios != SCENARIO_BASIC

    free(work_buf);

    printf("Compressed to size: %lu.\n",
            compressed_buf_len_out);

    uLong uncompressed_buf_len = source_buf_len;
    Byte * uncompressed_buf = (Byte *) malloc(uncompressed_buf_len);
    EXPECT_NE(uncompressed_buf, (Byte * )NULL);
    err = uncompressGetMinWorkBufSize(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);

    printf("Work buf size: %lu.\n",
            work_buf_len);

    //compressed_buf_len_out = compressed_buf_len;
    uLong uncompressed_buf_len_out = uncompressed_buf_len;

    if (scenarios == SCENARIO_BASIC) {
        switch (wrapper) {
        case WRAP_NONE:
            err = uncompressSafe2(uncompressed_buf, &uncompressed_buf_len_out,
                    compressed_buf, &compressed_buf_len_out,
                    work_buf, work_buf_len, windowBits);
            break;
        case WRAP_ZLIB:
            if (all_params_default) {
                err = uncompressSafe(uncompressed_buf, &uncompressed_buf_len_out,
                        compressed_buf, &compressed_buf_len_out,
                        work_buf, work_buf_len);
            } else {
                err = uncompressSafe2(uncompressed_buf, &uncompressed_buf_len_out,
                        compressed_buf, &compressed_buf_len_out,
                        work_buf, work_buf_len, windowBits);
            }
            break;
        case WRAP_GZIP_NULL:
        case WRAP_GZIP_BASIC:
        case WRAP_GZIP_BUFFERS:
            if (all_params_default) {
                err = uncompressSafeGzip(uncompressed_buf, &uncompressed_buf_len_out,
                        compressed_buf, &compressed_buf_len_out,
                        work_buf, work_buf_len, &head_out);
            } else {
                err = uncompressSafeGzip2(uncompressed_buf, &uncompressed_buf_len_out,
                        compressed_buf, &compressed_buf_len_out,
                        work_buf, work_buf_len, windowBits, &head_out);
            }
            break;
        }

        EXPECT_EQ(err, Z_OK);

    } else {

        z_static_mem mem_inf;
        mem_inf.work = work_buf;
        mem_inf.workLen = work_buf_len;
        mem_inf.workAlloced = 0;

        z_stream stream_inf;
        stream_inf.next_in = (z_const Bytef *) compressed_buf;
        stream_inf.avail_in = compressed_buf_len;
        stream_inf.next_out = uncompressed_buf;
        stream_inf.avail_out = uncompressed_buf_len;
        stream_inf.zalloc = z_static_alloc;
        stream_inf.zfree = z_static_free;
        stream_inf.opaque = (voidpf) &mem_inf;

        err = inflateInit(&stream_inf);
        EXPECT_EQ(err, Z_OK);

        if(scenarios & SCENARIO_DICTIONARY) {
            // inflate should report dictionary needed
            err = inflate(&stream_inf, Z_NO_FLUSH);
            EXPECT_EQ(err, Z_NEED_DICT);
            if (stream_inf.msg != Z_NULL) {
                printf("msg: %s\n", stream_inf.msg);
            }
            printf("setting dictionary\n");
            err = inflateSetDictionary(&stream_inf, dictionary, dictLength);
            EXPECT_EQ(err, Z_OK);
            if (stream_inf.msg != Z_NULL) {
                printf("msg: %s\n", stream_inf.msg);
            }
        }

        // inflate
        err = inflate(&stream_inf, Z_FINISH);
        uncompressed_buf_len_out = stream_inf.total_out;
        EXPECT_EQ(err, Z_STREAM_END);
        if (stream_inf.msg != Z_NULL) {
            printf("msg: %s\n", stream_inf.msg);
        }

        if(scenarios & SCENARIO_DICTIONARY) {
            Bytef * dictionary_out_inf = (Bytef *) malloc(32768);
            ASSERT_NE(dictionary_out_inf, (Bytef*)NULL);
            uInt dict_out_inf_len = 0;
            printf("getting dictionary\n");
            err = inflateGetDictionary(&stream_inf, dictionary_out_inf,
                    &dict_out_inf_len);
            EXPECT_EQ(err, Z_OK);
            free(dictionary_out_inf);

        }

        err = inflateEnd(&stream_inf);
        EXPECT_EQ(err, Z_OK);

    } // end if not basic

    EXPECT_EQ(uncompressed_buf_len_out, source_buf_len);

    // Compare
    int nbad = 0;
    if (uncompressed_buf_len_out == source_buf_len) {
        for (int i = 0; i < source_buf_len; i++) {
            if (source_buf[i] != uncompressed_buf[i])
                nbad++;
        }
    }
    EXPECT_EQ(nbad, 0);

//    EXPECT_EQ(head.time, head_out.time);
//    EXPECT_EQ(0, strcmp((char*)head_out.name, (char*)head.name));

    free(work_buf);
    free(compressed_buf);
    free(uncompressed_buf);


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

    printf("zlib version %s = 0x%04x, compile flags = 0x%lx\n",
            ZLIB_VERSION, ZLIB_VERNUM, zlibCompileFlags());

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


#if DO_CORPUS_TESTS
TEST_F(ZlibTest, Corpus) {
    for (int j = 0; j < NUM_CORPUS; j++) {
        printf("\n");
        zlib_test_file(corpus_files[j]);
    }
}

TEST_F(ZlibTest, CorpusGzip) {
    for (int j = 0; j < NUM_CORPUS; j++) {
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_GZIP_BASIC);
    }
}

TEST_F(ZlibTest, CorpusLevels) {
    for (int j = 0; j < NUM_CORPUS; j++) {
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_BASIC,
                Z_NO_COMPRESSION);
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_BASIC,
                Z_BEST_SPEED);
        printf("\n");
        zlib_test_file(corpus_files[j], WRAP_ZLIB, SCENARIO_BASIC,
                Z_BEST_COMPRESSION);
    }
}
#endif

TEST_F(ZlibTest, Dictionary) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_DICTIONARY);
}

TEST_F(ZlibTest, Pending) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_PENDING);
}

TEST_F(ZlibTest, Params) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_PARAMS);
}

TEST_F(ZlibTest, FullFlush) {
    zlib_test_alice(WRAP_ZLIB, SCENARIO_FULL_FLUSH);
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

    uLong compressed_buf_len;
    Byte * compressed_buf;
    uLong work_buf_len;
    Byte * work_buf;
    uLong compressed_buf_len_out;

    err = compressGetMaxOutputSize(source_buf_len, &compressed_buf_len);
    EXPECT_EQ(err, Z_OK);
    compressed_buf = (Byte *) malloc(compressed_buf_len);
    ASSERT_NE(compressed_buf, (Bytef*)NULL);


    err = compressGetMinWorkBufSize(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    ASSERT_NE(work_buf, (Bytef*)NULL);
    compressed_buf_len_out = compressed_buf_len;

    printf("bad window and mem level\n");
    err = compressSafe2(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len,
            work_buf, work_buf_len, Z_DEFAULT_COMPRESSION, 42, 1337, Z_DEFAULT_STRATEGY);
    EXPECT_EQ(err, Z_STREAM_ERROR);
    printf("zError:%s\n", zError(err));

    printf("bad strategy\n");
    err = compressSafe2(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len,
            work_buf, work_buf_len, Z_DEFAULT_COMPRESSION, DEF_WBITS, DEF_MEM_LEVEL, 42);
    EXPECT_EQ(err, Z_STREAM_ERROR);
    printf("zError:%s\n", zError(err));


    printf("work buf size = 0\n");
    err = compressSafe(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len,
            work_buf, 0, Z_DEFAULT_COMPRESSION);
    EXPECT_EQ(err, Z_MEM_ERROR);
    printf("zError:%s\n", zError(err));

    printf("output buf size small\n");
    compressed_buf_len_out = 42;
    err = compressSafe(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len,
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

    err = compressGetMaxOutputSize(source_buf_len, &compressed_buf_len);
    EXPECT_EQ(err, Z_OK);
    compressed_buf = (Byte *) malloc(compressed_buf_len);
    ASSERT_NE(compressed_buf, (Bytef*)NULL);


    err = compressGetMinWorkBufSize(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    ASSERT_NE(work_buf, (Bytef*)NULL);
    compressed_buf_len_out = compressed_buf_len;

    err = compressSafe(compressed_buf, &compressed_buf_len_out,
            source_buf, source_buf_len,
            work_buf, work_buf_len, Z_DEFAULT_COMPRESSION);
    EXPECT_EQ(err, Z_OK);

    free(work_buf);

    uLong uncompressed_buf_len = source_buf_len;
    Byte * uncompressed_buf = (Byte *) malloc(uncompressed_buf_len);
    EXPECT_NE(uncompressed_buf, (Byte * )NULL);
    err = uncompressGetMinWorkBufSize(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);

    uLong compressed_buf_len_out2;
    uLong uncompressed_buf_len_out;

    printf("work buf size = 0\n");
    compressed_buf_len_out2 = compressed_buf_len_out;
    uncompressed_buf_len_out = uncompressed_buf_len;
    err = uncompressSafe(uncompressed_buf, &uncompressed_buf_len_out,
                            compressed_buf, &compressed_buf_len_out2,
                            work_buf, 0);
    EXPECT_EQ(err, Z_MEM_ERROR);
    printf("zError:%s\n", zError(err));

    printf("bad window bits arg\n");
    compressed_buf_len_out2 = compressed_buf_len_out;
    uncompressed_buf_len_out = uncompressed_buf_len;
    err = uncompressSafe2(uncompressed_buf, &uncompressed_buf_len_out,
                            compressed_buf, &compressed_buf_len_out2,
                            work_buf, work_buf_len, 500);
    EXPECT_EQ(err, Z_STREAM_ERROR);
    printf("zError:%s\n", zError(err));

    printf("output buf size small\n");
    compressed_buf_len_out2 = compressed_buf_len_out;
    uncompressed_buf_len_out = 42;
    err = uncompressSafe(uncompressed_buf, &uncompressed_buf_len_out,
                            compressed_buf, &compressed_buf_len_out2,
                            work_buf, work_buf_len);
    EXPECT_EQ(err, Z_BUF_ERROR);
    printf("zError:%s\n", zError(err));

    printf("gzip header when not gzip\n");
    gz_header header;
    memset(&header, 0, sizeof(header));
    compressed_buf_len_out2 = compressed_buf_len_out;
    uncompressed_buf_len_out = uncompressed_buf_len;
    err = uncompressSafeGzip2(uncompressed_buf, &uncompressed_buf_len_out,
                            compressed_buf, &compressed_buf_len_out2,
                            work_buf, work_buf_len, DEF_WBITS, &header);
    EXPECT_EQ(err, Z_STREAM_ERROR);
    printf("zError:%s\n", zError(err));


//    // repeat with NULL GIP HEADER
//
//    free(uncompressed_buf);
//    free(compressed_buf);
//    free(source_buf);
//    free(work_buf);
//
//    file = fopen("corpus/cantrbry/alice29.txt", "r");
//    ASSERT_FALSE(file == NULL);
//
//    source_buf_len = CORPUS_MAX_SIZE;
//    source_buf = (Byte *) malloc(source_buf_len);
//    nread = fread(source_buf, 1, source_buf_len, file);
//    ASSERT_FALSE(ferror(file));
//    fclose(file);
//    ASSERT_TRUE(nread <= source_buf_len);
//    source_buf_len = nread;
//
//
//    err = compressGetMaxOutputSize(source_buf_len, &compressed_buf_len);
//    EXPECT_EQ(err, Z_OK);
//    compressed_buf = (Byte *) malloc(compressed_buf_len);
//    ASSERT_NE(compressed_buf, (Bytef*)NULL);
//
//
//    err = compressGetMinWorkBufSize(&work_buf_len);
//    EXPECT_EQ(err, Z_OK);
//    work_buf = (Byte *) malloc(work_buf_len);
//    ASSERT_NE(work_buf, (Bytef*)NULL);
//    compressed_buf_len_out = compressed_buf_len;
//
//    err = compressSafeGzip(compressed_buf, &compressed_buf_len_out,
//            source_buf, source_buf_len,
//            work_buf, work_buf_len, Z_DEFAULT_COMPRESSION, Z_NULL);
//    EXPECT_EQ(err, Z_OK);
//
//    free(work_buf);
//
//    uncompressed_buf_len = source_buf_len;
//    uncompressed_buf = (Byte *) malloc(uncompressed_buf_len);
//    EXPECT_NE(uncompressed_buf, (Byte * )NULL);
//    err = uncompressGetMinWorkBufSize(&work_buf_len);
//    EXPECT_EQ(err, Z_OK);
//    work_buf = (Byte *) malloc(work_buf_len);
//
//
//    printf("null out header\n");
//    compressed_buf_len_out2 = compressed_buf_len_out;
//    uncompressed_buf_len_out = uncompressed_buf_len;
//    err = uncompressSafeGzip(uncompressed_buf, &uncompressed_buf_len_out,
//                            compressed_buf, &compressed_buf_len_out2,
//                            work_buf, work_buf_len, Z_NULL);
//    EXPECT_EQ(err, Z_MEM_ERROR);
//    printf("zError:%s\n", zError(err));
//
//    printf("bad window bits arg\n");
//    compressed_buf_len_out2 = compressed_buf_len_out;
//    uncompressed_buf_len_out = uncompressed_buf_len;
//    err = uncompressSafe2(uncompressed_buf, &uncompressed_buf_len_out,
//                            compressed_buf, &compressed_buf_len_out2,
//                            work_buf, work_buf_len, 500);
//    EXPECT_EQ(err, Z_STREAM_ERROR);
//    printf("zError:%s\n", zError(err));
//
//    printf("output buf size small\n");
//    compressed_buf_len_out2 = compressed_buf_len_out;
//    uncompressed_buf_len_out = 42;
//    err = uncompressSafe(uncompressed_buf, &uncompressed_buf_len_out,
//                            compressed_buf, &compressed_buf_len_out2,
//                            work_buf, work_buf_len);
//    EXPECT_EQ(err, Z_BUF_ERROR);
//    printf("zError:%s\n", zError(err));
//
//
//
//    free(uncompressed_buf);
//    free(compressed_buf);
//    free(source_buf);
//    free(work_buf);



}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

