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

// max size of a file from a corpus
enum {
    CORPUS_MAX_SIZE_CANTRBRY = 1029744,
    CORPUS_MAX_SIZE_ARTIFICL =  100000,
    CORPUS_MAX_SIZE_LARGE    = 4638690,
    CORPUS_MAX_SIZE_CALGARY  =  768771,
    CORPUS_MAX_SIZE = CORPUS_MAX_SIZE_LARGE
};

typedef enum {
    WRAP_NONE = 0,
    WRAP_ZLIB,
    WRAP_GZIP,
    WRAP_GZIP_ALLOCED, // non-null stuff in header
} WrapType;

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


void zlib_test_compress(WrapType wrap_type, int level, int strategy)
{
    int err;

    FILE * file = fopen("corpus/cantrbry/alice29.txt", "r");
    ASSERT_FALSE(file == NULL);

    int source_buf_len = 160000;
    Byte * source_buf = (Byte *)malloc(source_buf_len);
    int nread = fread(source_buf, 1, source_buf_len, file);
    ASSERT_FALSE(ferror(file));
    fclose(file);
    printf("Read %d bytes.\n", nread);
    ASSERT_TRUE(nread <= source_buf_len);
    source_buf_len = nread;


    uLong compressed_buf_len;
    Byte * compressed_buf;
    uLong work_buf_len;
    Byte * work_buf;
    uLong compressed_buf_len_out;
    gz_header head;
    memset(&head, 0, sizeof(head));
    head.text = true;
    head.time = 42;
    head.os = 3;
    head.extra = Z_NULL;
    head.name = Z_NULL;
    head.comment = Z_NULL;
    if(wrap_type == WRAP_GZIP_ALLOCED) {
        head.comment = (Bytef*)"I am the man with no name.";
        head.name = (Bytef*)"Zapp Brannigan.";
        head.extra = (Bytef*)"Hello.";
        head.extra_len = strlen((char*)head.extra);
    }

    gz_header head_out;
    head_out.name = (Byte *)malloc(80);
    head_out.name_max = 80;
    head_out.comment = (Byte *)malloc(80);
    head_out.comm_max = 80;
    head_out.extra = (Byte *)malloc(80);
    head_out.extra_max = 80;

    switch (wrap_type) {
    case WRAP_NONE:
        err = compressGetMaxOutputSize2(source_buf_len, -DEF_WBITS, DEF_MEM_LEVEL,
                &compressed_buf_len);
        break;
    case WRAP_ZLIB:
        err = compressGetMaxOutputSize(source_buf_len, &compressed_buf_len);
        break;
    case WRAP_GZIP:
    case WRAP_GZIP_ALLOCED:
        err = compressGetMaxOutputSizeGzip(source_buf_len, &head, &compressed_buf_len);
        break;
    }
    EXPECT_EQ(err, Z_OK);
    compressed_buf = (Byte *) malloc(compressed_buf_len);
    err = compressGetMinWorkBufSize(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    compressed_buf_len_out = compressed_buf_len;

    printf("Compressed buf size: %lu. Work buf size: %lu.\n",
            compressed_buf_len, work_buf_len);

    switch (wrap_type) {
    case WRAP_NONE:
        err = compressSafe2(compressed_buf, &compressed_buf_len_out,
                source_buf, source_buf_len,
                work_buf, work_buf_len, level,
                -DEF_WBITS, DEF_MEM_LEVEL, strategy);
        break;
    case WRAP_ZLIB:
        if (strategy == Z_DEFAULT_STRATEGY) {
            err = compressSafe(compressed_buf, &compressed_buf_len_out,
                    source_buf, source_buf_len,
                    work_buf, work_buf_len, level);
        } else {
            err = compressSafe2(compressed_buf, &compressed_buf_len_out,
                    source_buf, source_buf_len,
                    work_buf, work_buf_len, level,
                    DEF_WBITS, DEF_MEM_LEVEL, strategy);
        }
        break;
    case WRAP_GZIP:
        case WRAP_GZIP_ALLOCED:
        if (strategy == Z_DEFAULT_STRATEGY) {
            err = compressSafeGzip(compressed_buf, &compressed_buf_len_out,
                    source_buf, source_buf_len,
                    work_buf, work_buf_len, level, &head);
        } else {
            err = compressSafeGzip2(compressed_buf, &compressed_buf_len_out,
                    source_buf, source_buf_len,
                    work_buf, work_buf_len, level,
                    DEF_WBITS, DEF_MEM_LEVEL, strategy, &head);

        }
        break;
    }



//    if (use_gzip) {
//        err = compressGetMaxOutputSizeGzip(len, &head, &compressed_buf_len);
//        EXPECT_EQ(err, Z_OK);
//        compressed_buf = (Byte *) malloc(compressed_buf_len);
//        err = compressGetMinWorkBufSize(&work_buf_len);
//        EXPECT_EQ(err, Z_OK);
//        work_buf = (Byte *) malloc(work_buf_len);
//        compressed_buf_len_out = compressed_buf_len;
//
//        printf("Compressed buf size: %lu. Work buf size: %lu.\n",
//                compressed_buf_len, work_buf_len);
//
//        err = compressSafeGzip(compressed_buf, &compressed_buf_len_out,
//                (const Bytef*) hello, len,
//                work_buf, work_buf_len, &head);
//
//    } else {
//        err = compressGetMaxOutputSize(len, &compressed_buf_len);
//        EXPECT_EQ(err, Z_OK);
//        compressed_buf = (Byte *) malloc(compressed_buf_len);
//        err = compressGetMinWorkBufSize(&work_buf_len);
//        EXPECT_EQ(err, Z_OK);
//        work_buf = (Byte *) malloc(work_buf_len);
//        compressed_buf_len_out = compressed_buf_len;
//
//        printf("Compressed buf size: %lu. Work buf size: %lu.\n",
//                compressed_buf_len, work_buf_len);
//
//        err = compressSafe(compressed_buf, &compressed_buf_len_out,
//                (const Bytef*) hello, len,
//                work_buf, work_buf_len);
//
//
//    }

    EXPECT_EQ(err, Z_OK);

    free(work_buf);


    printf("Compressed to size: %lu.\n",
            compressed_buf_len_out);


    uLong uncompressed_buf_len = source_buf_len;
    Byte * uncompressed_buf = (Byte *)malloc(uncompressed_buf_len);
    err = uncompressGetMinWorkBufSize(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *)malloc(work_buf_len);

    printf("Work buf size: %lu.\n",
            work_buf_len);

    //compressed_buf_len_out = compressed_buf_len;
    uLong uncompressed_buf_len_out = uncompressed_buf_len;

    switch (wrap_type) {
    case WRAP_NONE:
        err = uncompressSafe2(uncompressed_buf, &uncompressed_buf_len_out,
                compressed_buf, &compressed_buf_len_out,
                work_buf, work_buf_len, -DEF_WBITS);
        break;
    case WRAP_ZLIB:
        err = uncompressSafe(uncompressed_buf, &uncompressed_buf_len_out,
                compressed_buf, &compressed_buf_len_out,
                work_buf, work_buf_len);
        break;
    case WRAP_GZIP:
    case WRAP_GZIP_ALLOCED:
        err = uncompressSafeGzip(uncompressed_buf, &uncompressed_buf_len_out,
                compressed_buf, &compressed_buf_len_out,
                work_buf, work_buf_len, &head_out);
        break;
    }

//    if (use_gzip) {
//        err = uncompressSafeGzip(uncompressed_buf, &uncompressed_buf_len_out,
//                compressed_buf, &compressed_buf_len_out,
//                work_buf, work_buf_len, &head_out);
//    } else {
//        err = uncompressSafe(uncompressed_buf, &uncompressed_buf_len_out,
//                compressed_buf, &compressed_buf_len_out,
//                work_buf, work_buf_len);
//    }

    EXPECT_EQ(err, Z_OK);
    EXPECT_EQ(uncompressed_buf_len_out, nread);
    // Compare
    int nbad = 0;
    if(uncompressed_buf_len_out == nread) {
        for (int i = 0; i < nread; i++) {
            if (source_buf[i] != uncompressed_buf[i])
                nbad++;
        }
    }
    EXPECT_EQ(nbad, 0);


//    if (strcmp((char*)uncompressed_buf, hello)) {
//        fprintf(stderr, "bad uncompress\n");
//        exit(1);
//    } else {
//        printf("uncompress(): %s\n", (char *)uncompressed_buf);
//    }

    if (wrap_type == WRAP_GZIP) {
        EXPECT_EQ(head.time, head_out.time);
    }
    if (wrap_type == WRAP_GZIP_ALLOCED) {
        EXPECT_EQ(head.time, head_out.time);
        EXPECT_EQ(0, strcmp((char*)head_out.name, (char*)head.name));
    }

    free(work_buf);
    free(compressed_buf);
    free(uncompressed_buf);
    free(head_out.name);
    free(head_out.comment);
    free(head_out.extra);

}

TEST_F(ZlibTest, Compress) {
    zlib_test_compress(WRAP_ZLIB, Z_DEFAULT_COMPRESSION, Z_DEFAULT_STRATEGY);
}

TEST_F(ZlibTest, CompressGzip) {
    zlib_test_compress(WRAP_GZIP, Z_DEFAULT_COMPRESSION, Z_DEFAULT_STRATEGY);
}

TEST_F(ZlibTest, CompressGzipAlloced) {
    zlib_test_compress(WRAP_GZIP_ALLOCED, Z_DEFAULT_COMPRESSION, Z_DEFAULT_STRATEGY);
}

TEST_F(ZlibTest, CompressRaw) {
    zlib_test_compress(WRAP_NONE, Z_DEFAULT_COMPRESSION, Z_DEFAULT_STRATEGY);
}

TEST_F(ZlibTest, CompressNone) {
    zlib_test_compress(WRAP_ZLIB, Z_NO_COMPRESSION, Z_DEFAULT_STRATEGY);
}

TEST_F(ZlibTest, CompressBestSpeed) {
    zlib_test_compress(WRAP_ZLIB, Z_BEST_SPEED, Z_DEFAULT_STRATEGY);
}

TEST_F(ZlibTest, CompressBestCompression) {
    zlib_test_compress(WRAP_ZLIB, Z_BEST_COMPRESSION, Z_DEFAULT_STRATEGY);
}

TEST_F(ZlibTest, CompressFiltered) {
    zlib_test_compress(WRAP_ZLIB, Z_DEFAULT_COMPRESSION, Z_FILTERED);
}

TEST_F(ZlibTest, CompressHuffmanOnly) {
    zlib_test_compress(WRAP_ZLIB, Z_DEFAULT_COMPRESSION, Z_HUFFMAN_ONLY);
}

TEST_F(ZlibTest, CompressRLE) {
    zlib_test_compress(WRAP_ZLIB, Z_DEFAULT_COMPRESSION, Z_RLE);
}

TEST_F(ZlibTest, CompressFixed) {
    zlib_test_compress(WRAP_ZLIB, Z_DEFAULT_COMPRESSION, Z_FIXED);
}



void zlib_test_corpus(bool use_gzip, int level)
{
    int err;
    FILE * file;
    int source_buf_len;
    Byte * source_buf;
    uLong compressed_buf_len;
    Byte * compressed_buf;
    uLong cwork_buf_len;
    Byte * cwork_buf;
    uLong uwork_buf_len;
    Byte * uwork_buf;
    int ucomp_buf_len;
    Byte * ucomp_buf;

    gz_header head;
    memset(&head, 0, sizeof(head));
    head.text = true;
    head.time = 42;
    head.os = 3;
    head.extra = Z_NULL;
    head.name = Z_NULL;
    head.comment = Z_NULL;
    gz_header head_out;


    source_buf_len = CORPUS_MAX_SIZE_LARGE;
    source_buf = (Byte*) malloc(source_buf_len);
    err = use_gzip ?
            compressGetMaxOutputSizeGzip(source_buf_len, &head,
                    &compressed_buf_len) :
            compressGetMaxOutputSize(source_buf_len, &compressed_buf_len);
    EXPECT_EQ(err, Z_OK);
    compressed_buf = (Byte *) malloc(compressed_buf_len);
    err = compressGetMinWorkBufSize(&cwork_buf_len);
    EXPECT_EQ(err, Z_OK);
    cwork_buf = (Byte *) malloc(cwork_buf_len);
    err = uncompressGetMinWorkBufSize(&uwork_buf_len);
    EXPECT_EQ(err, Z_OK);
    uwork_buf = (Byte *) malloc(uwork_buf_len);
    ucomp_buf_len = CORPUS_MAX_SIZE_LARGE;
    ucomp_buf = (Byte*) malloc(ucomp_buf_len);

    for (int j = 0; j < NUM_CORPUS; j++) {

        file = fopen(corpus_files[j], "r");
        EXPECT_FALSE(file == NULL);
        if (file == NULL) {
            continue;
        }
        int nread = fread(source_buf, 1, source_buf_len, file);
        EXPECT_FALSE(ferror(file));
        if (ferror(file)) {
            continue;
        }
        fclose(file);
        printf("Read %d bytes from file %s.\n", nread, corpus_files[j]);

        uLong compressed_buf_len_out = compressed_buf_len;
        err = use_gzip ?
                compressSafeGzip(compressed_buf, &compressed_buf_len_out,
                        source_buf, nread,
                        cwork_buf, cwork_buf_len, level, &head) :
                compressSafe(compressed_buf, &compressed_buf_len_out,
                        source_buf, nread,
                        cwork_buf, cwork_buf_len, level);

        EXPECT_EQ(err, Z_OK);

        printf("Compressed to %lu bytes, reduction of %f percent.\n",
                compressed_buf_len_out,
                (100.0*(nread-compressed_buf_len_out))/nread);

        uLong ucomp_buf_len_out = ucomp_buf_len;
        err = use_gzip ? uncompressSafeGzip(ucomp_buf, &ucomp_buf_len_out,
                                 compressed_buf, &compressed_buf_len_out,
                                 uwork_buf, uwork_buf_len, &head_out) :
                         uncompressSafe(ucomp_buf, &ucomp_buf_len_out,
                                 compressed_buf, &compressed_buf_len_out,
                                 uwork_buf, uwork_buf_len);

        EXPECT_EQ(err, Z_OK);
        EXPECT_EQ(ucomp_buf_len_out,nread);
        // Compare
        int nbad = 0;
        if(ucomp_buf_len_out == nread) {
            for (int i = 0; i < nread; i++) {
                if (source_buf[i] != ucomp_buf[i])
                    nbad++;
            }
        }
        EXPECT_EQ(nbad, 0);

        if (use_gzip) {
            EXPECT_EQ(head.time, head_out.time);
        }

        if (err == Z_OK) {
            printf("Input:\n");
            for (int i = 0; i < (nread < 240 ? nread : 240); i++) {
                if (isprint(source_buf[i]) || source_buf[i] == 0xA
                    || source_buf[i] == 0xD) {
                    printf("%c", source_buf[i]);
                } else {
                    printf(" %#x ", source_buf[i]);
                }
            }
            printf("...\n");
            printf("Output:\n");
            for (int i = 0; i < (nread < 240 ? nread : 240); i++) {
                if (isprint(ucomp_buf[i]) || ucomp_buf[i] == 0xA
                    || source_buf[i] == 0xD) {
                    printf("%c", ucomp_buf[i]);
                } else {
                    printf(" %#x ", ucomp_buf[i]);
                }
            }
            printf("...\n");
        }
    }

    free(source_buf);
    free(compressed_buf);
    free(cwork_buf);
    free(uwork_buf);
    free(ucomp_buf);
}

// FIXME restore
TEST_F(ZlibTest, Corpus) {
    bool use_gzip = false;
    zlib_test_corpus(use_gzip, Z_DEFAULT_COMPRESSION);
}

TEST_F(ZlibTest, CorpusGzip) {
    bool use_gzip = true;
    zlib_test_corpus(use_gzip, Z_DEFAULT_COMPRESSION);
}

TEST_F(ZlibTest, CorpusLevels) {
    bool use_gzip = false;
    zlib_test_corpus(use_gzip, Z_NO_COMPRESSION);
    zlib_test_corpus(use_gzip, Z_BEST_SPEED);
    zlib_test_corpus(use_gzip, Z_BEST_COMPRESSION);
}

TEST_F(ZlibTest, Dictionary) {

    int err;

    FILE * file = fopen("corpus/cantrbry/alice29.txt", "r");
    ASSERT_FALSE(file == NULL);

    int source_buf_len = 160000;
    Byte * source_buf = (Byte *)malloc(source_buf_len);
    int nread = fread(source_buf, 1, source_buf_len, file);
    ASSERT_FALSE(ferror(file));
    fclose(file);
    printf("Read %d bytes.\n", nread);
    ASSERT_TRUE(nread <= source_buf_len);
    source_buf_len = nread;


    uLong compressed_buf_len;
    Byte * compressed_buf;
    uLong work_buf_len;
    Byte * work_buf;
    uLong compressed_buf_len_out;
//    gz_header head;
//    memset(&head, 0, sizeof(head));
//    head.text = true;
//    head.time = 42;
//    head.os = 3;
//    head.name = (Bytef*)"Alice's Adventures In Wonderland";
//    head.comment = (Bytef*)"By Lewis Carroll";
//    head.extra = (Bytef*)"THE MILLENNIUM FULCRUM EDITION 2.9";
//    head.extra_len = strlen((char*)head.extra);
//
//    gz_header head_out;
//    head_out.name = (Byte *)malloc(80);
//    head_out.name_max = 80;
//    head_out.comment = (Byte *)malloc(80);
//    head_out.comm_max = 80;
//    head_out.extra = (Byte *)malloc(80);
//    head_out.extra_max = 80;

    err = compressGetMaxOutputSize(source_buf_len, &compressed_buf_len);
    EXPECT_EQ(err, Z_OK);
    compressed_buf = (Byte *) malloc(compressed_buf_len);
    err = compressGetMinWorkBufSize(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *) malloc(work_buf_len);
    compressed_buf_len_out = compressed_buf_len;

    printf("Compressed buf size: %lu. Work buf size: %lu.\n",
            compressed_buf_len, work_buf_len);


    z_static_mem mem;
    mem.work = work_buf;
    mem.workLen = work_buf_len;
    mem.workAlloced = 0;

    z_stream stream;
    stream.zalloc = z_static_alloc;
    stream.zfree = z_static_free;
    stream.opaque = (voidpf)&mem;

    err = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
//    err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
//         DEF_WBITS + 16, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
//    err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
//            DEF_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    EXPECT_EQ(err, Z_OK);

    stream.next_out = compressed_buf;
    stream.avail_out = compressed_buf_len_out;
    stream.next_in = source_buf;
    stream.avail_in = source_buf_len;


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
    int dictLength = strlen((char*)dictionary);
    err = deflateSetDictionary(&stream, dictionary, dictLength);
    EXPECT_EQ(err, Z_OK);

//    // set gzip header, if not null
//     err = deflateSetHeader(&stream, &head);
//     EXPECT_EQ(err, Z_OK);


    // compress in one fell swoop
    err = deflate(&stream, Z_FINISH);
    EXPECT_EQ(err, Z_STREAM_END);

    compressed_buf_len_out = stream.total_out;

    Bytef * dictionary_out = (Bytef *)malloc(32768);
    uInt dict_out_len = 0;
    err = deflateGetDictionary(&stream, dictionary_out, &dict_out_len);
    EXPECT_EQ(err, Z_OK);

    err = deflateEnd(&stream);
    EXPECT_EQ(err, Z_OK);

    free(work_buf);

    printf("Compressed to size: %lu.\n",
            compressed_buf_len_out);


    uLong uncompressed_buf_len = source_buf_len;
    Byte * uncompressed_buf = (Byte *)malloc(uncompressed_buf_len);
    EXPECT_NE(uncompressed_buf, (Byte *)NULL);
    err = uncompressGetMinWorkBufSize(&work_buf_len);
    EXPECT_EQ(err, Z_OK);
    work_buf = (Byte *)malloc(work_buf_len);

    printf("Work buf size: %lu.\n",
            work_buf_len);

    //compressed_buf_len_out = compressed_buf_len;
    uLong uncompressed_buf_len_out = uncompressed_buf_len;

    z_static_mem mem_inf;
    mem_inf.work = work_buf;
    mem_inf.workLen = work_buf_len;
    mem_inf.workAlloced = 0;

    z_stream stream_inf;
    stream_inf.next_in = (z_const Bytef *)compressed_buf;
    stream_inf.avail_in = compressed_buf_len;
    stream_inf.next_out = uncompressed_buf;
    stream_inf.avail_out = uncompressed_buf_len;
    stream_inf.zalloc = z_static_alloc;
    stream_inf.zfree = z_static_free;
    stream_inf.opaque = (voidpf)&mem_inf;

    err = inflateInit(&stream_inf);
    EXPECT_EQ(err, Z_OK);

    // inflate should report dictionary needed
    err = inflate(&stream_inf, Z_NO_FLUSH);
    EXPECT_EQ(err, Z_NEED_DICT);
    if(stream_inf.msg != Z_NULL) {
        printf("msg: %s\n", stream_inf.msg);
    }

    err = inflateSetDictionary(&stream_inf, dictionary, dictLength);
    EXPECT_EQ(err, Z_OK);
    if(stream_inf.msg != Z_NULL) {
        printf("msg: %s\n", stream_inf.msg);
    }

    // inflate
    err = inflate(&stream_inf, Z_FINISH);
    uncompressed_buf_len_out = stream_inf.total_out;
    EXPECT_EQ(err, Z_STREAM_END);
    if(stream_inf.msg != Z_NULL) {
        printf("msg: %s\n", stream_inf.msg);
    }


    Bytef * dictionary_out_inf = (Bytef *)malloc(32768);
    uInt dict_out_inf_len = 0;
    err = inflateGetDictionary(&stream_inf, dictionary_out_inf, &dict_out_inf_len);
    EXPECT_EQ(err, Z_OK);


    err = inflateEnd(&stream_inf);
    EXPECT_EQ(err, Z_OK);

    EXPECT_EQ(uncompressed_buf_len_out, nread);
    // Compare
    int nbad = 0;
    if(uncompressed_buf_len_out == nread) {
        for (int i = 0; i < nread; i++) {
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
//    free(head_out.name);
//    free(head_out.comment);
//    free(head_out.extra);

    free(dictionary_out);



}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

