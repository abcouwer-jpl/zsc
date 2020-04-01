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

// max size of a file from a corpus
enum {
    CORPUS_MAX_SIZE_CANTRBRY = 1029744,
    CORPUS_MAX_SIZE_ARTIFICL =  100000,
    CORPUS_MAX_SIZE_LARGE    = 4638690,
    CORPUS_MAX_SIZE_CALGARY  =  768771,
    CORPUS_MAX_SIZE = CORPUS_MAX_SIZE_LARGE
};

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


void zlib_test_compress(bool test_gzip)
{
    int err;
    uLong len = (uLong)strlen(hello)+1;

    uLong compressed_buf_len;
    Byte * compressed_buf;
    uLong work_buf_len;
    Byte * work_buf;
    uLong compressed_buf_len_out;

    if (test_gzip) {
        gz_header head;
        memset(&head, 0, sizeof(head));
        head.text = true;
        head.time = 42;
        head.os = 3;
        head.extra = Z_NULL;
        head.name = Z_NULL;
        head.comment = Z_NULL;



        compressed_buf_len = compressSafeBoundDestGzip(len, &head);
        compressed_buf = (Byte *) malloc(compressed_buf_len);
        work_buf_len = compressSafeBoundWork();
        work_buf = (Byte *) malloc(work_buf_len);
        compressed_buf_len_out = compressed_buf_len;

        printf("Compressed buf size: %lu. Work buf size: %lu.\n",
                compressed_buf_len, work_buf_len);

        err = compressSafeGzip(compressed_buf, &compressed_buf_len_out,
                (const Bytef*) hello, len,
                work_buf, work_buf_len, &head);

    } else {
        compressed_buf_len = compressSafeBoundDest(len);
        compressed_buf = (Byte *) malloc(compressed_buf_len);
        work_buf_len = compressSafeBoundWork();
        work_buf = (Byte *) malloc(work_buf_len);
        compressed_buf_len_out = compressed_buf_len;

        printf("Compressed buf size: %lu. Work buf size: %lu.\n",
                compressed_buf_len, work_buf_len);

        err = compressSafe(compressed_buf, &compressed_buf_len_out,
                (const Bytef*) hello, len,
                work_buf, work_buf_len);


    }

    EXPECT_EQ(err, Z_OK);

    free(work_buf);


    printf("Compressed to size: %lu.\n",
            compressed_buf_len_out);


    uLong uncompressed_buf_len = len;
    Byte * uncompressed_buf = (Byte *)malloc(uncompressed_buf_len);
    work_buf_len = uncompressSafeBoundWork();
    work_buf = (Byte *)malloc(work_buf_len);

    printf("Work buf size: %lu.\n",
            work_buf_len);

    //compressed_buf_len_out = compressed_buf_len;
    uLong uncompressed_buf_len_out = uncompressed_buf_len;
    if (test_gzip) {
        err = uncompressSafeGzip(uncompressed_buf, &uncompressed_buf_len_out,
                compressed_buf, &compressed_buf_len_out,
                work_buf, work_buf_len);
    } else {
        err = uncompressSafe(uncompressed_buf, &uncompressed_buf_len_out,
                compressed_buf, &compressed_buf_len_out,
                work_buf, work_buf_len);
    }

    EXPECT_EQ(err, Z_OK);
    if (strcmp((char*)uncompressed_buf, hello)) {
        fprintf(stderr, "bad uncompress\n");
        exit(1);
    } else {
        printf("uncompress(): %s\n", (char *)uncompressed_buf);
    }

    free(work_buf);
    free(compressed_buf);
    free(uncompressed_buf);

}

TEST_F(ZlibTest, CompressSafe) {
    bool test_gzip = false;
    zlib_test_compress(test_gzip);

//    int err;
//    uLong len = (uLong)strlen(hello)+1;
//
//    uLong compressed_buf_len = compressSafeBoundDest(len);
//    Byte * compressed_buf = (Byte *)malloc(compressed_buf_len);
//    uLong work_buf_len = compressSafeBoundWork();
//    Byte * work_buf = (Byte *)malloc(work_buf_len);
//
//    printf("Compressed buf size: %lu. Work buf size: %lu.\n",
//            compressed_buf_len, work_buf_len);
//
//    uLong compressed_buf_len_out = compressed_buf_len;
//    err = compressSafe(compressed_buf, &compressed_buf_len_out,
//            (const Bytef*)hello, len,
//            work_buf, work_buf_len);
//
//    EXPECT_EQ(err, Z_OK);
//
//    free(work_buf);
//
//    printf("Compressed to size: %lu.\n",
//            compressed_buf_len_out);
//
//
//    uLong uncompressed_buf_len = len;
//    Byte * uncompressed_buf = (Byte *)malloc(uncompressed_buf_len);
//    work_buf_len = uncompressSafeBoundWork();
//    work_buf = (Byte *)malloc(work_buf_len);
//
//    printf("Work buf size: %lu.\n",
//            work_buf_len);
//
//    //compressed_buf_len_out = compressed_buf_len;
//    uLong uncompressed_buf_len_out = uncompressed_buf_len;
//    err = uncompressSafe(uncompressed_buf, &uncompressed_buf_len_out,
//            compressed_buf, &compressed_buf_len_out,
//            work_buf, work_buf_len);
//
//    EXPECT_EQ(err, Z_OK);
//    if (strcmp((char*)uncompressed_buf, hello)) {
//        fprintf(stderr, "bad uncompress\n");
//        exit(1);
//    } else {
//        printf("uncompress(): %s\n", (char *)uncompressed_buf);
//    }
//
//    free(work_buf);
//    free(compressed_buf);
//    free(uncompressed_buf);

}

TEST_F(ZlibTest, CompressSafeGzip) {
    bool test_gzip = true;
    zlib_test_compress(test_gzip);
}

TEST_F(ZlibTest, CompressCorpusSafe) {

    int err;
    FILE * file;
    int source_buf_len = CORPUS_MAX_SIZE_LARGE;
    Byte * source_buf = (Byte*) malloc(source_buf_len);
    int compressed_buf_len = compressSafeBoundDest(source_buf_len);
    Byte * compressed_buf = (Byte *) malloc(compressed_buf_len);
    int cwork_buf_len = compressSafeBoundWork();
    Byte * cwork_buf = (Byte *) malloc(cwork_buf_len);
    int uwork_buf_len = uncompressSafeBoundWork();
    Byte * uwork_buf = (Byte *) malloc(uwork_buf_len);
    int ucomp_buf_len = CORPUS_MAX_SIZE_LARGE;
    Byte * ucomp_buf = (Byte*) malloc(ucomp_buf_len);

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
        err = compressSafe(compressed_buf, &compressed_buf_len_out,
                source_buf, nread,
                cwork_buf, cwork_buf_len);

        EXPECT_EQ(err, Z_OK);

        printf("Compressed to %lu bytes, reduction of %f \%.\n",
                compressed_buf_len_out, (nread-compressed_buf_len_out)/nread);

        uLong ucomp_buf_len_out = ucomp_buf_len;
        err = uncompressSafe(ucomp_buf, &ucomp_buf_len_out,
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








int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

