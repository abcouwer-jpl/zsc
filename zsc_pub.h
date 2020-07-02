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
 * @file        zsc_pub.h
 * @date        2020-07-01
 * @author      Neil Abcouwer
 * @brief       Function declarations for using zlib in a safety critical way.
 *
 * These functions to zlib compression and decompression without using dynamic
 * memory allocation. Compression functions allow a maximum block length,
 * creating compressed buffers with independent sections.
 * If one block is corrupted, the remaining blocks can still be recovered.
 * The decompression functions will indicate data error in this case, but
 * the remaining data will still be written to the output buffer.
 *
 * The following parameters are common to a number of functions:
 *
 * - The level must be Z_DEFAULT_COMPRESSION, or between 0 and 9:
 *   1 gives best speed, 9 gives best compression, 0 gives no compression at all
 *   (the input data is simply copied a block at a time).  Z_DEFAULT_COMPRESSION
 *   requests a default compromise between speed and compression
 *   (currently equivalent to level 6).
 *
 * - The window_bits parameter is the base two logarithm of the window size
 *   (the size of the history buffer). It should be in the range 9..15
 *   for this version of the library. Larger values of this parameter
 *   result in better compression at the expense of memory usage.
 *   The default value is 15.
 *
 * - The mem_level parameter specifies how much memory should be allocated
 *   for the internal compression state. mem_level=1 uses minimum memory
 *   but is slow and reduces compression ratio; mem_level=9 uses maximum
 *   memory for optimal speed. The default value is 8.
 *   See zconf.h for total memory usage as a function of window_bits and mem_level.
 *
 * - The strategy parameter is used to tune the compression algorithm.
 *   Use the value Z_DEFAULT_STRATEGY for normal data, Z_FILTERED for data
 *   produced by a filter (or predictor), Z_HUFFMAN_ONLY to force Huffman
 *   encoding only (no string match), or Z_RLE to limit match distances
 *   to one (run-length encoding). Filtered data consists mostly of small
 *   values with a somewhat random distribution. In this case, the
 *   compression algorithm is tuned to compress them better. The effect of
 *   Z_FILTERED is to force more Huffman coding and less string matching;
 *   it is somewhat intermediate between Z_DEFAULT_STRATEGY and Z_HUFFMAN_ONLY.
 *   Z_RLE is designed to be almost as fast as Z_HUFFMAN_ONLY, but give better
 *   compression for PNG image data. The strategy parameter only affects
 *   the compression ratio but not the correctness of the compressed output
 *   even if it is not set appropriately. Z_FIXED prevents the use of dynamic
 *   Huffman codes, allowing for a simpler decoder for special applications.
 *
 */

#ifndef ZSC_PUB_H
#define ZSC_PUB_H

// FIXME replace with just a types file
#include "zconf.h"
#include "zlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// FIXME add macros for sizes

/**
 * @brief Get minimum size of a work buffer, default compression settings
 * Returns the size of working memory that must be provided to a compression function
 * using default settings..
 *
 * @param size_out  Minimum size required for working memory
 * @return Z_OK if there was no error
 */
int zsc_compress_get_min_work_buf_size(uLongf *size_out);

/**
 * @brief Get minimum size of a work buffer, custom compression settings
 * Returns the size of working memory that must be provided to a compression function
 * using a certain window size and memory level.
 *
 * @param window_bits   the base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 * @param mem_level     how much memory to use for internal state.
 *                      Should be in the range 1 to 9.
 * @param size_out      Minimum size required for working memory
 * @return Z_STREAM_ERROR if parameters are improper, Z_OK otherwise
 */
int zsc_compress_get_min_work_buf_size2(int window_bits,
        int mem_level,
        uLongf *size_out);

/**
 * @brief Get maximum theoretical size of compression output, default settings
 * Returns the maximum size that a compressed output could be,
 * given default memory and window settings.
 * Note this is actually larger than the input size, completely random input
 * could be incompressible, plus overhead, plus wrappers.
 *  *
 * @param source_len  Size, in bytes, of the compression input
 * @param max_block_len  Maximum size, in bytes, of an output block
 * @param level  Compression level
 * @param size_out  Minimum size required for working memory
 * @return Z_OK if there was no error
 */
int zsc_compress_get_max_output_size(uLong source_len,
        uLong max_block_len,
        int level,
        uLongf *size_out);

/**
 * @brief Get maximum size of compression output with a gzip wrapper, default
 * Returns the maximum size that a compressed output could be,
 * given default memory and window settings, and a gzip wrapper.
 * Note this is actually larger than the input size, completely random input
 * could be incompressible, plus overhead, plus wrappers.
 *  *
 * @param source_len  Size, in bytes, of the compression input
 * @param max_block_len  Maximum size, in bytes, of an output block
 * @param level  Compression level
 * @param gz_header Pointer to gzip header
 * @param size_out  Minimum size required for working memory
 * @return Z_OK if there was no error
 */
int zsc_compress_get_max_output_size_gzip(uLong source_len,
        uLong max_block_len,
        int level,
        gz_headerp gz_header,
        uLongf *size_out);

/**
 * @brief Get maximum theoretical size of compression output, custom settings
 * Returns the maximum size that a compressed output could be,
 * given custom memory and window settings.
 * Note this is actually larger than the input size, completely random input
 * could be incompressible, plus overhead, plus wrappers.
 *  *
 * @param source_len    Size, in bytes, of the compression input
 * @param max_block_len Maximum size, in bytes, of an output block
 * @param level         Compression level
 * @param window_bits   the base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 * @param mem_level     how much memory to use for internal state.
 *                      Should be in the range 1 to 9.
 * @param size_out  `   Minimum size required for working memory
 * @return Z_OK if there was no error
 */
int zsc_compress_get_max_output_size2(uLong source_len,
        uLong max_block_len,
        int level,
        int window_bits,
        int mem_level,
        uLongf *size_out);

/**
 * @brief Get maximum size of compression output with a gzip wrapper, custom settings
 * Returns the maximum size that a compressed output could be,
 * given custom memory and window settings, and a gzip wrapper.
 * Note this is actually larger than the input size, completely random input
 * could be incompressible, plus overhead, plus wrappers.
 *  *
 * @param source_len     Size, in bytes, of the compression input
 * @param max_block_len Maximum size, in bytes, of an output block
 * @param level         Compression level
 * @param window_bits   the base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 * @param mem_level     how much memory to use for internal state.
 *                      Should be in the range 1 to 9.
 * @param size_out      Minimum size required for working memory
 * @param gz_header     Pointer to gzip header
 * @return Z_OK if there was no error
 */
int zsc_compress_get_max_output_size_gzip2(uLong source_len,
        uLong max_block_len,
        int level,
        int window_bits,
        int mem_level,
        gz_headerp gz_head,
        uLongf *size_out);

/**
 * @brief Compress a buffer.
 * @param dest          Output buffer
 * @param dest_len      The length of output buffer, in bytes
 *                      If smaller than size given by get_max_output_size(),
 *                      compression may fail.
 *                      After call, gets the size of compressed output.
 * @param source        Input buffer
 * @param source_len    Length of input buffer, in bytes
 * @param max_block_len Maximum length of a compressed output.
 *                      If input is larger than this length, only max_block_len bytes
 *                      will be compressed at a time, and each of these sections
 *                      can be uncompressed independently.
 *                      Making this size too small will significantly degrade
 *                      compression performance.
 * @param work          Working memory
 * @param work_len      Length of work buffer. If less than size given by
 *                      get_min_work_buf_size(), compression will fail.
 * @param level         Compression level
 * @return Z_OK if compression succeeded, an error code otherwise.
 */
int zsc_compress(Bytef *dest,
        uLongf *dest_len,
        const Bytef *source,
        uLong source_len,
        uLong max_block_len,
        Bytef *work,
        uLong work_len,
        int level);

/**
 * @brief Compress a buffer with a gzip header
 * @param dest          Output buffer
 * @param dest_len      Length of output buffer, in bytes
 *                      If smaller than size given by get_max_output_size(),
 *                      compression may fail.
 *                      After call, gets the size of compressed output.
 * @param source        Input buffer
 * @param source_len    Length of input buffer, in bytes
 * @param max_block_len Maximum length of a compressed output.
 *                      If input is larger than this length, only max_block_len bytes
 *                      will be compressed at a time, and each of these sections
 *                      can be uncompressed independently.
 *                      Making this size too small will significantly degrade
 *                      compression performance.
 * @param work          Working memory
 * @param work_len      Length of work buffer. If less than size given by
 *                      get_min_work_buf_size(), compression will fail.
 * @param level         Compression level
 * @param gz_headerp    Pointer to a GZip header
 * @return Z_OK if compression succeeded, an error code otherwise.
 */
int zsc_compress_gzip(Bytef *dest,
        uLongf *dest_len,
        const Bytef *source,
        uLong source_len,
        uLong max_block_len,
        Bytef *work,
        uLong work_len,
        int level,
        gz_headerp gz_header);

/**
 * @brief Compress a buffer with custom settings
 * @param dest          Output buffer
 * @param dest_len      Length of output buffer, in bytes
 *                      If smaller than size given by get_max_output_size(),
 *                      compression may fail.
 *                      After call, gets the size of compressed output.
 * @param source        Input buffer
 * @param source_len    Length of input buffer, in bytes
 * @param max_block_len Maximum length of a compressed output.
 *                      If input is larger than this length, only max_block_len bytes
 *                      will be compressed at a time, and each of these sections
 *                      can be uncompressed independently.
 *                      Making this size too small will significantly degrade
 *                      compression performance.
 * @param work          Working memory
 * @param work_len      Length of work buffer. If less than size given by
 *                      get_min_work_buf_size(), compression will fail.
 * @param level         Compression level
 * @param window_bits   the base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 * @param mem_level     how much memory to use for internal state.
 *                      Should be in the range 1 to 9.
 * @param strategy      Compression strategy
 * @return Z_OK if compression succeeded, an error code otherwise.
 */
int zsc_compress2(Bytef *dest,
        uLongf *dest_len,
        const Bytef *source,
        uLong source_len,
        uLong max_block_len,
        Bytef *work,
        uLong work_len,
        int level,
        int window_bits,
        int mem_level,
        int strategy);

/**
 * @brief Compress a buffer with custom settings and a gzip header
 * @param dest          Output buffer
 * @param dest_len      Length of output buffer, in bytes
 *                      If smaller than size given by get_max_output_size(),
 *                      compression may fail.
 *                      After call, gets the size of compressed output.
 * @param source        Input buffer
 * @param source_len    Length of input buffer, in bytes
 * @param max_block_len Maximum length of a compressed output.
 *                      If input is larger than this length, only max_block_len bytes
 *                      will be compressed at a time, and each of these sections
 *                      can be uncompressed independently.
 *                      Making this size too small will significantly degrade
 *                      compression performance.
 * @param work          Working memory
 * @param work_len      Length of work buffer. If less than size given by
 *                      get_min_work_buf_size(), compression will fail.
 * @param level         Compression level
 * @param window_bits   the base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 * @param mem_level     how much memory to use for internal state.
 *                      Should be in the range 1 to 9.
 * @param strategy      Compression strategy
 * @param gz_headerp    Pointer to a GZip header
 * @return Z_OK if compression succeeded, an error code otherwise.
 */
int zsc_compress_gzip2(Bytef *dest,
        uLongf *dest_len,
        const Bytef *source,
        uLong source_len,
        uLong max_block_len,
        Bytef *work,
        uLong work_len,
        int level,
        int window_bits,
        int mem_level,
        int strategy,
        gz_headerp gz_header);

/**
 * @brief Get minimum size of a work buffer, default decompression settings
 * Returns the size of working memory that must be provided to a decompression
 * function using default settings.
 *
 * @param size_out  Minimum size required for working memory
 * @return Z_OK if there was no error
 */
int zsc_uncompress_get_min_work_buf_size(uLongf *size_out);

/**
 * @brief Get minimum size of a work buffer, custom window size
 * Returns the size of working memory that must be provided to a decompression
 * function using default settings.
 *
 * @param window_bits   the base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 * @param size_out      Minimum size required for working memory
 * @return  Z_STREAM_ERROR if parameters are improper, Z_OK otherwise
 */
int zsc_uncompress_get_min_work_buf_size2(int window_bits,
        uLongf *size_out);

/**
 * @brief Decompress a buffer.
 * If some blocks were found to have a data error, but there were no other issues,
 * The function will return Z_DATA_ERROR and fill the output buffer with the
 * blocks that did not have errors.
 *
 * @param dest          Output buffer
 * @param dest_len      Length of output buffer, in bytes
 *                      If smaller than the size of the compressed data,
 *                      compression will fail.
 *                      It is up to the client to decide how appropriately size
 *                      this buffer.
 *                      After call, gets the size of decompressed output.
 * @param source        Input compressed buffer
 * @param source_len    Length of input buffer, in bytes
 *                      After call, gets number of bytes actually processed.
 * @param work          Working memory
 * @param work_len      Length of work buffer. If less than size given by
 *                      get_min_work_buf_size(), decompression will fail.
 * @return Z_OK if decompression succeeded, an error code otherwise.
 */
int zsc_uncompress(Bytef *dest,
        uLongf *dest_len,
        const Bytef *source,
        uLong *source_len,
        Bytef *work,
        uLong work_len);

/**
 * @brief Decompress a buffer with a gzip header.
 * @param dest          Output buffer
 * @param dest_len      Length of output buffer, in bytes
 *                      If smaller than the size of the compressed data,
 *                      compression will fail.
 *                      It is up to the client to decide how appropriately size
 *                      this buffer.
 *                      After call, gets the size of decompressed output.
 * @param source        Input compressed buffer
 * @param source_len    Length of input buffer, in bytes
 *                      After call, gets number of bytes actually processed.
 * @param work          Working memory
 * @param work_len      Length of work buffer. If less than size given by
 *                      get_min_work_buf_size(), decompression will fail.
 * @param gz_head       Pointer to where the gzip wrapper will be saved.
 * @return Z_OK if decompression succeeded, an error code otherwise.
 */
int zsc_uncompress_safe_gzip(Bytef *dest,
        uLongf *dest_len,
        const Bytef *source,
        uLong *source_len,
        Bytef *work,
        uLong work_len,
        gz_headerp gz_head);

/**
 * @brief Decompress a buffer with a custom window size.
 * @param dest          Output buffer
 * @param dest_len      Length of output buffer, in bytes
 *                      If smaller than the size of the compressed data,
 *                      compression will fail.
 *                      It is up to the client to decide how appropriately size
 *                      this buffer.
 *                      After call, gets the size of decompressed output.
 * @param source        Input compressed buffer
 * @param source_len    Length of input buffer, in bytes
 *                      After call, gets number of bytes actually processed.
 * @param work          Working memory
 * @param work_len      Length of work buffer. If less than size given by
 *                      get_min_work_buf_size(), decompression will fail.
 * @param window_bits   the base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 * @return Z_OK if decompression succeeded, an error code otherwise.
 */
int zsc_uncompress_safe2(Bytef *dest,
        uLongf *dest_len,
        const Bytef *source,
        uLong *source_len,
        Bytef *work,
        uLong work_len,
        int window_bits);

/**
 * @brief Decompress a buffer with a GZIP wrapper and a custom window size.
 * @param dest          Output buffer
 * @param dest_len      Length of output buffer, in bytes
 *                      If smaller than the size of the compressed data,
 *                      compression will fail.
 *                      It is up to the client to decide how appropriately size
 *                      this buffer.
 *                      After call, gets the size of decompressed output.
 * @param source        Input compressed buffer
 * @param source_len    Length of input buffer, in bytes.
 *                      After call, gets number of bytes actually processed.
 * @param work          Working memory
 * @param work_len      Length of work buffer. If less than size given by
 *                      get_min_work_buf_size(), decompression will fail.
 * @param window_bits   the base two logarithm of the window size.
 *                      Should be in the range 9 to 15.
 * @param gz_head       Pointer to where the gzip wrapper will be saved.
 * @return Z_OK if decompression succeeded, an error code otherwise.
 */
int zsc_uncompress_safe_gzip2(Bytef *dest,
        uLongf *dest_len,
        const Bytef *source,
        uLong *source_len,
        Bytef *work,
        uLong work_len,
        int window_bits,
        gz_headerp gz_head);

#ifdef __cplusplus
}
#endif

#endif /* ZSC_PUB_H_ */
