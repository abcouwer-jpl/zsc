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

//void test_deflate       OF((Byte *compr, uLong comprLen));
//void test_inflate       OF((Byte *compr, uLong comprLen,
//                            Byte *uncompr, uLong uncomprLen));
//void test_large_deflate OF((Byte *compr, uLong comprLen,
//                            Byte *uncompr, uLong uncomprLen));
//void test_large_inflate OF((Byte *compr, uLong comprLen,
//                            Byte *uncompr, uLong uncomprLen));
//void test_flush         OF((Byte *compr, uLong *comprLen));
//void test_sync          OF((Byte *compr, uLong comprLen,
//                            Byte *uncompr, uLong uncomprLen));
//void test_dict_deflate  OF((Byte *compr, uLong comprLen));
//void test_dict_inflate  OF((Byte *compr, uLong comprLen,
//                            Byte *uncompr, uLong uncomprLen));
//int  main               OF((int argc, char *argv[]));


#ifdef Z_SOLO

void *myalloc OF((void *, unsigned, unsigned));
void myfree OF((void *, void *));

void *myalloc(q, n, m)
    void *q;
    unsigned n, m;
{
    (void)q;
    return calloc(n, m);
}

void myfree(void *q, void *p)
{
    (void)q;
    free(p);
}

static alloc_func zalloc = myalloc;
static free_func zfree = myfree;

#else /* !Z_SOLO */

static alloc_func zalloc = (alloc_func)0;
static free_func zfree = (free_func)0;

void test_compress      OF((Byte *compr, uLong comprLen,
                            Byte *uncompr, uLong uncomprLen));
void test_gzio          OF((const char *fname,
                            Byte *uncompr, uLong uncomprLen));




// The fixture for testing zlib.
class ZlibTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if their bodies would
  // be empty.

  ZlibTest() {
     // You can do set-up work for each test here.
//      static const char* myVersion = ZLIB_VERSION;
//
//      if (zlibVersion()[0] != myVersion[0]) {
//          fprintf(stderr, "incompatible zlib version\n");
//          exit(1);
//
//      } else if (strcmp(zlibVersion(), ZLIB_VERSION) != 0) {
//          fprintf(stderr, "warning: different zlib version\n");
//      }
//
//      printf("zlib version %s = 0x%04x, compile flags = 0x%lx\n",
//              ZLIB_VERSION, ZLIB_VERNUM, zlibCompileFlags());

      compr    = (Byte*)calloc((uInt)comprLen, 1);
      uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
      /* compr and uncompr are cleared to avoid reading uninitialized
       * data and to ensure that uncompr compresses well.
       */
      if (compr == Z_NULL || uncompr == Z_NULL) {
          printf("out of memory\n");
          exit(1);
      }
  }

  ~ZlibTest() override {
     // You can do clean-up work that doesn't throw exceptions here.
      free(compr);
      free(uncompr);
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

  // Class members declared here can be used by all tests in the test suite
  // for Foo.
  Byte *compr;
  Byte *uncompr;
  uLong comprLen = 10000*sizeof(int); /* don't overflow on MSDOS */
  uLong uncomprLen = comprLen;
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

/* ===========================================================================
 * Test compress() and uncompress()
 */
TEST_F(ZlibTest, Compress) {
//void test_compress(Byte *compr, uLong comprLen, Byte *uncompr, uLong uncomprLen)
//{
    int err;
    uLong len = (uLong)strlen(hello)+1;

    err = compress(compr, &comprLen, (const Bytef*)hello, len);
    EXPECT_EQ(err, Z_OK);
    CHECK_ERR(err, "compress");

    strcpy((char*)uncompr, "garbage");

    err = uncompress(uncompr, &uncomprLen, compr, comprLen);
    EXPECT_EQ(err, Z_OK);
    CHECK_ERR(err, "uncompress");

    EXPECT_EQ(strcmp((char*)uncompr, hello), 0);
    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad uncompress\n");
        exit(1);
    } else {
        printf("uncompress(): %s\n", (char *)uncompr);
    }
}

/* ===========================================================================
 * Test read/write of .gz files
 */
TEST_F(ZlibTest, Gzio) {
//void test_gzio(const char *fname,  /* compressed file name */
//        Byte *uncompr, uLong uncomprLen)
//{
#ifdef NO_GZCOMPRESS
    fprintf(stderr, "NO_GZCOMPRESS -- gz* functions cannot compress\n");
#else
    const char *fname = /*(argc > 1 ? argv[1] : */TESTFILE/*)*/;
    int err;
    int len = (int)strlen(hello)+1;
    gzFile file;
    z_off_t pos;
    int ret;

    file = gzopen(fname, "wb");
    ASSERT_FALSE(file == NULL);
    gzputc(file, 'h');
    ret = gzputs(file, "ello");
    ASSERT_EQ(ret,4);
    ret = gzprintf(file, ", %s!", "hello");
    ASSERT_EQ(ret,8);
    gzseek(file, 1L, SEEK_CUR); /* add one zero byte */
    gzclose(file);

    file = gzopen(fname, "rb");
    ASSERT_FALSE(file == NULL);

    strcpy((char*)uncompr, "garbage");
    ret = gzread(file, uncompr, (unsigned)uncomprLen);
    ASSERT_EQ(ret, len);
    ASSERT_EQ(strcmp((char*)uncompr, hello), 0);
    printf("gzread(): %s\n", (char*)uncompr);

    pos = gzseek(file, -8L, SEEK_CUR);
    ASSERT_EQ(pos,6);

    ASSERT_EQ(gztell(file), pos);

    ASSERT_EQ(gzgetc(file), ' ');

    ASSERT_EQ(gzungetc(' ', file), ' ');

    gzgets(file, (char*)uncompr, (int)uncomprLen);

    ASSERT_EQ(strlen((char*)uncompr), 7);
    ASSERT_EQ(strcmp((char*)uncompr, hello + 6),0);

    printf("gzgets() after gzseek: %s\n", (char*)uncompr);

    gzclose(file);
#endif
}

#endif /* Z_SOLO */

/* ===========================================================================
 * Test deflate() with small buffers
 */
TEST_F(ZlibTest, DeflateInflate) {
//void test_deflate(
//    Byte *compr,
//    uLong comprLen)
//{
    z_stream c_stream; /* compression stream */
    int err;
    uLong len = (uLong)strlen(hello)+1;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "deflateInit");

    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.next_out = compr;

    while (c_stream.total_in != len && c_stream.total_out < comprLen) {
        c_stream.avail_in = c_stream.avail_out = 1; /* force small buffers */
        err = deflate(&c_stream, Z_NO_FLUSH);
//        CHECK_ERR(err, "deflate");
        ASSERT_EQ(err, Z_OK);
    }
    /* Finish the stream, still forcing small buffers: */
    for (;;) {
        c_stream.avail_out = 1;
        err = deflate(&c_stream, Z_FINISH);
        if (err == Z_STREAM_END) break;
//        CHECK_ERR(err, "deflate");
        ASSERT_EQ(err, Z_OK);
    }

    err = deflateEnd(&c_stream);
//    CHECK_ERR(err, "deflateEnd");
    ASSERT_EQ(err, Z_OK);

//}
//
///* ===========================================================================
// * Test inflate() with small buffers
// */
//TEST_F(ZlibTest, Inflate) {
////void test_inflate(Byte *compr, uLong comprLen, Byte *uncompr, uLong uncomprLen)
//////    Byte *compr, *uncompr;
//////    uLong comprLen, uncomprLen;
////{
//    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = 0;
    d_stream.next_out = uncompr;

    err = inflateInit(&d_stream);
//    CHECK_ERR(err, "inflateInit");
    ASSERT_EQ(err, Z_OK);

    while (d_stream.total_out < uncomprLen && d_stream.total_in < comprLen) {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
//        CHECK_ERR(err, "inflate");
        ASSERT_EQ(err, Z_OK);
    }

    err = inflateEnd(&d_stream);
//    CHECK_ERR(err, "inflateEnd");
    ASSERT_EQ(err, Z_OK);

    ASSERT_EQ(strcmp((char*)uncompr, hello), 0);
//    if (strcmp((char*)uncompr, hello)) {
//        fprintf(stderr, "bad inflate\n");
//        exit(1);
//    } else {
    printf("inflate(): %s\n", (char *)uncompr);
//    }
}

/* ===========================================================================
 * Test deflate() with large buffers and dynamic change of compression level
 */
TEST_F(ZlibTest, LargeDeflateInflate) {
//void test_large_deflate(Byte *compr, uLong comprLen, Byte *uncompr, uLong uncomprLen)
////    Byte *compr, *uncompr;
////    uLong comprLen, uncomprLen;
//{
    z_stream c_stream; /* compression stream */
    int err;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_BEST_SPEED);
//    CHECK_ERR(err, "deflateInit");
    ASSERT_EQ(err, Z_OK);

    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;

    /* At this point, uncompr is still mostly zeroes, so it should compress
     * very well:
     */
    c_stream.next_in = uncompr;
    c_stream.avail_in = (uInt)uncomprLen;
    err = deflate(&c_stream, Z_NO_FLUSH);
//    CHECK_ERR(err, "deflate");
    ASSERT_EQ(err, Z_OK);
    ASSERT_EQ(c_stream.avail_in, 0);
//    if (c_stream.avail_in != 0) {
//        fprintf(stderr, "deflate not greedy\n");
//        exit(1);
//    }

    /* Feed in already compressed data and switch to no compression: */
    deflateParams(&c_stream, Z_NO_COMPRESSION, Z_DEFAULT_STRATEGY);
    c_stream.next_in = compr;
    c_stream.avail_in = (uInt)comprLen/2;
    err = deflate(&c_stream, Z_NO_FLUSH);
//    CHECK_ERR(err, "deflate");
    ASSERT_EQ(err, Z_OK);

    /* Switch back to compressing mode: */
    deflateParams(&c_stream, Z_BEST_COMPRESSION, Z_FILTERED);
    c_stream.next_in = uncompr;
    c_stream.avail_in = (uInt)uncomprLen;
    err = deflate(&c_stream, Z_NO_FLUSH);
//    CHECK_ERR(err, "deflate");
    ASSERT_EQ(err, Z_OK);

    err = deflate(&c_stream, Z_FINISH);
    ASSERT_EQ(err, Z_STREAM_END);
//    if (err != Z_STREAM_END) {
//        fprintf(stderr, "deflate should report Z_STREAM_END\n");
//        exit(1);
//    }
    err = deflateEnd(&c_stream);
//    CHECK_ERR(err, "deflateEnd");
    ASSERT_EQ(err, Z_OK);

//}
//
///* ===========================================================================
// * Test inflate() with large buffers
// */
//TEST_F(ZlibTest, LargeInflate) {
////void test_large_inflate(Byte *compr, uLong comprLen, Byte *uncompr, uLong uncomprLen)
//////    Byte *compr, *uncompr;
//////    uLong comprLen, uncomprLen;
////{
//    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)comprLen;

    err = inflateInit(&d_stream);
//    CHECK_ERR(err, "inflateInit");
    ASSERT_EQ(err, Z_OK);

    for (;;) {
        d_stream.next_out = uncompr;            /* discard the output */
        d_stream.avail_out = (uInt)uncomprLen;
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        ASSERT_EQ(err, Z_OK);
//        CHECK_ERR(err, "large inflate");
    }

    err = inflateEnd(&d_stream);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "inflateEnd");

    ASSERT_EQ(d_stream.total_out, 2*uncomprLen + comprLen/2);
//    if (d_stream.total_out != 2*uncomprLen + comprLen/2) {
//        fprintf(stderr, "bad large inflate: %ld\n", d_stream.total_out);
//        exit(1);
//    } else {
        printf("large_inflate(): OK\n");
//    }
}

/* ===========================================================================
 * Test deflate() with full flush
 */
TEST_F(ZlibTest, DeflateFlushInflateSync) {
//void test_flush(Byte *compr, uLong *comprLen)
////    Byte *compr;
////    uLong *comprLen;
//{
    z_stream c_stream; /* compression stream */
    int err;
    uInt len = (uInt)strlen(hello)+1;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "deflateInit");

    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.next_out = compr;
    c_stream.avail_in = 3;
    c_stream.avail_out = (uInt)comprLen;
    err = deflate(&c_stream, Z_FULL_FLUSH);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "deflate");

    compr[3]++; /* force an error in first compressed block */
    c_stream.avail_in = len - 3;

    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        ASSERT_EQ(err, Z_OK);
//        CHECK_ERR(err, "deflate");
    }
    err = deflateEnd(&c_stream);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "deflateEnd");

    comprLen = c_stream.total_out;
//}
//
///* ===========================================================================
// * Test inflateSync()
// */
//TEST_F(ZlibTest, Sync) {
////void test_sync(Byte *compr, uLong comprLen, Byte *uncompr, uLong uncomprLen)
//////    Byte *compr, *uncompr;
//////    uLong comprLen, uncomprLen;
////{
//    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = 2; /* just read the zlib header */

    err = inflateInit(&d_stream);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "inflateInit");

    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    err = inflate(&d_stream, Z_NO_FLUSH);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "inflate");

    d_stream.avail_in = (uInt)comprLen-2;   /* read all compressed data */
    err = inflateSync(&d_stream);           /* but skip the damaged part */
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "inflateSync");

    err = inflate(&d_stream, Z_FINISH);
    ASSERT_EQ(err, Z_DATA_ERROR);
//    if (err != Z_DATA_ERROR) {
//        fprintf(stderr, "inflate should report DATA_ERROR\n");
//        /* Because of incorrect adler32 */
//        exit(1);
//    }
    err = inflateEnd(&d_stream);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "inflateEnd");

    printf("after inflateSync(): hel%s\n", (char *)uncompr);
}

/* ===========================================================================
 * Test deflate() with preset dictionary
 */
TEST_F(ZlibTest, DictDeflateInflate) {
//void test_dict_deflate(Byte *compr, uLong comprLen)
////    Byte *compr;
////    uLong comprLen;
//{
    z_stream c_stream; /* compression stream */
    int err;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_BEST_COMPRESSION);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "deflateInit");

    err = deflateSetDictionary(&c_stream,
                (const Bytef*)dictionary, (int)sizeof(dictionary));
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "deflateSetDictionary");

    dictId = c_stream.adler;
    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;

    c_stream.next_in = (z_const unsigned char *)hello;
    c_stream.avail_in = (uInt)strlen(hello)+1;

    err = deflate(&c_stream, Z_FINISH);
    ASSERT_EQ(err, Z_STREAM_END);
//    if (err != Z_STREAM_END) {
//        fprintf(stderr, "deflate should report Z_STREAM_END\n");
//        exit(1);
//    }
    err = deflateEnd(&c_stream);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "deflateEnd");
//}
//
///* ===========================================================================
// * Test inflate() with a preset dictionary
// */
//TEST_F(ZlibTest, DictInflate) {
////void test_dict_inflate(Byte *compr, uLong comprLen,
////        Byte *uncompr, uLong uncomprLen)
//////    Byte *compr, *uncompr;
//////    uLong comprLen, uncomprLen;
////{
//    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)comprLen;

    err = inflateInit(&d_stream);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "inflateInit");

    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    for (;;) {
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        if (err == Z_NEED_DICT) {
            ASSERT_EQ(d_stream.adler, dictId);
//            if (d_stream.adler != dictId) {
//                fprintf(stderr, "unexpected dictionary");
//                exit(1);
//            }
            err = inflateSetDictionary(&d_stream, (const Bytef*)dictionary,
                                       (int)sizeof(dictionary));
        }
        ASSERT_EQ(err, Z_OK);
//        CHECK_ERR(err, "inflate with dict");
    }

    err = inflateEnd(&d_stream);
    ASSERT_EQ(err, Z_OK);
//    CHECK_ERR(err, "inflateEnd");

    ASSERT_EQ(strcmp((char*)uncompr, hello), 0);
//    if (strcmp((char*)uncompr, hello)) {
//        fprintf(stderr, "bad inflate with dict\n");
//        exit(1);
//    } else {
        printf("inflate with dictionary: %s\n", (char *)uncompr);
//    }
}

/* ===========================================================================
 * Usage:  example [output.gz  [input.gz]]
 */
#if 0
int main(int argc, char *argv[])
//    int argc;
//    char *argv[];
{
    Byte *compr, *uncompr;
    uLong comprLen = 10000*sizeof(int); /* don't overflow on MSDOS */
    uLong uncomprLen = comprLen;
    static const char* myVersion = ZLIB_VERSION;

    if (zlibVersion()[0] != myVersion[0]) {
        fprintf(stderr, "incompatible zlib version\n");
        exit(1);

    } else if (strcmp(zlibVersion(), ZLIB_VERSION) != 0) {
        fprintf(stderr, "warning: different zlib version\n");
    }

    printf("zlib version %s = 0x%04x, compile flags = 0x%lx\n",
            ZLIB_VERSION, ZLIB_VERNUM, zlibCompileFlags());

    compr    = (Byte*)calloc((uInt)comprLen, 1);
    uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
    /* compr and uncompr are cleared to avoid reading uninitialized
     * data and to ensure that uncompr compresses well.
     */
    if (compr == Z_NULL || uncompr == Z_NULL) {
        printf("out of memory\n");
        exit(1);
    }

#ifdef Z_SOLO
    (void)argc;
    (void)argv;
#else
    test_compress(compr, comprLen, uncompr, uncomprLen);

    test_gzio((argc > 1 ? argv[1] : TESTFILE),
              uncompr, uncomprLen);
#endif

    test_deflate(compr, comprLen);
    test_inflate(compr, comprLen, uncompr, uncomprLen);

    test_large_deflate(compr, comprLen, uncompr, uncomprLen);
    test_large_inflate(compr, comprLen, uncompr, uncomprLen);

    test_flush(compr, &comprLen);
    test_sync(compr, comprLen, uncompr, uncomprLen);
    comprLen = uncomprLen;

    test_dict_deflate(compr, comprLen);
    test_dict_inflate(compr, comprLen, uncompr, uncomprLen);

    free(compr);
    free(uncompr);

    return 0;
}
#endif


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

