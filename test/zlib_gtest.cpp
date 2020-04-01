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

//      compr    = (Byte*)calloc((uInt)comprLen, 1);
//      uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
//      /* compr and uncompr are cleared to avoid reading uninitialized
//       * data and to ensure that uncompr compresses well.
//       */
//      if (compr == Z_NULL || uncompr == Z_NULL) {
//          printf("out of memory\n");
//          exit(1);
//      }
  }

  ~ZlibTest() override {
     // You can do clean-up work that doesn't throw exceptions here.
//      free(compr);
//      free(uncompr);
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
//  Byte *compr;
//  Byte *uncompr;
//  uLong comprLen = 10000*sizeof(int); /* don't overflow on MSDOS */
//  uLong uncomprLen = comprLen;
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

TEST_F(ZlibTest, CompressSafe) {
    int err;
    uLong len = (uLong)strlen(hello)+1;

    uLong compressed_buf_len = compressSafeBoundDest(len);
    Byte * compressed_buf = (Byte *)malloc(compressed_buf_len);
    uLong work_buf_len = compressSafeBoundWork();
    Byte * work_buf = (Byte *)malloc(work_buf_len);

    printf("Compressed buf size: %lu. Work buf size: %lu.\n",
            compressed_buf_len, work_buf_len);

    uLong compressed_buf_len_out = compressed_buf_len;
    err = compressSafe(compressed_buf, &compressed_buf_len_out,
            (const Bytef*)hello, len,
            work_buf, work_buf_len);

    EXPECT_EQ(err, Z_OK);

    free(work_buf);



    uLong uncompressed_buf_len = len;
    Byte * uncompressed_buf = (Byte *)malloc(uncompressed_buf_len);
    work_buf_len = uncompressSafeBoundWork();
    work_buf = (Byte *)malloc(work_buf_len);

    printf("Work buf size: %lu.\n",
            work_buf_len);

    compressed_buf_len_out = compressed_buf_len;
    uLong uncompressed_buf_len_out = uncompressed_buf_len;
    err = uncompressSafe(uncompressed_buf, &uncompressed_buf_len_out,
            compressed_buf, &compressed_buf_len_out,
            work_buf, work_buf_len);

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




int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

