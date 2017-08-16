/*
 * tc_hash.h: Cryptographic hash function library.
 *
 * DEPENDS:
 * VERSION: 0.0.1 (2017-08-12)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Cas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.0.2    added "FIPS 202" algorithms (SHA3-{224,256,384,512}, SHAKE{128,256})
 * 0.0.1    initial public release (MD5, FIPS 180-4: SHA1 & SHA2-{224,256,384,512,512/224,512/256})
 *
 * TODOs:
 * - Tiger/{128,160,192}, Tiger2/{128,160,192}
 * - RIPEMD-{128,160,256,320}
 * - optimizations
 * - HMAC [FIPS 198-1]
 * - CRC-* (not cryptographic, but useful; most likely as a separate `tc_checksum`, though)
 *
 *
 *
 * A library of byte-oriented cryptographic hash functions (some non-cryptographic ones are planned).
 *
 * A single file should contain the following `#define` before including the header:
 *
 *      #define TC_HASH_IMPLEMENTATION
 *      #include "tc_hash.h"
 *
 */

/* ========== API ==========
 *
 * All of the hash algorithms follow the examples provided here for MD5; in
 * other words, consider `MD5` and `md5` as wildcards for any algorithm.
 *
 *
 * For simple use with all the data already in memory, simply call:
 *
 *      uint8_t digest[TCHASH_MD5_DIGEST_SIZE];
 *      tchash_md5(digest, data, length_in_bytes);
 *
 * However, for large files that wouldn't fit in memory, the above example may
 * not be viable. In that case, use the streaming API:
 *
 *      uint8_t digest[TCHASH_MD5_DIGEST_SIZE];
 *      TCHash_MD5 md5;
 *      tchash_md5_init(&md5);
 *      do
 *      {
 *          size_t nbytes;
 *          char buffer[4096];
 *          nybtes = fread(buffer, 1, sizeof(buffer), file);
 *          tchash_md5_process(&md5, buffer, nbytes);
 *      }
 *      while(nbytes == sizeof(buffer));
 *      tchash_md5_get(&md5, digest);
 *
 * There is no `tchash_md5_deinit()` function --- all the data lives on the
 * stack, and thus there is nothing to deinitialize. Copying the state can thus
 * be done simply by assigning structs: `TCHash_MD5 md5copy = md5;`.
 *
 * The SHAKE128 and SHAKE256 algorithms are an exception here, since they have
 * variable-length digests. They take an additional parameter, the size of the
 * digest, in the `get()` function. For this reason, there is also no equivalent
 * `DIGEST_SIZE` constant;
 *
 *
 * Note that `tchash_md5_get(md5,digest)` does *not* modify the state of `md5`.
 * This means that it can be used to fetch intermediate results, as in:
 *
 *      tchash_md5_process(&md5, bufA, bytesA);
 *      tchash_md5_get(&md5, digestA);
 *      tchash_md5_process(&md5, bufB, bytesB);
 *      tchash_md5_get(&md5, digestB);
 *      ...
 *
 *
 * Finally, some utility functions are available, e.g. for conversion between
 * hex-strings and raw byte data, and for
 *
 *
 * The following algorithms are available:
 * - MD5 (not recommended)
 * - SHA1 [FIPS 180-4] (not recommended)
 * - SHA2 [FIPS 180-4]:
 *      - SHA2-256 (a.k.a SHA-256; 32-bit)
 *      - SHA2-512 (a.k.a SHA-512; 64-bit)
 *      - SHA2-224 (a.k.a SHA-224; variant of SHA2-256)
 *      - SHA2-384 (a.k.a SHA-384; variant of SHA2-512)
 *      - SHA2-512/224 (a.k.a SHA-512/224)
 *      - SHA2-512/256 (a.k.a SHA-512/256)
 *      - SHA2-512/t IV Generation Function (can be used to derive initial
 *        values for any `t` for SHA2-512/t)
 * - SHA3 [FIPS 202]:
 *      - SHA3-224
 *      - SHA3-256
 *      - SHA3-384
 *      - SHA3-512
 * - SHAKE [FIPS 202]:
 *      - SHAKE128
 *      - SHAKE256
 * To help decide: avoid MD5 & SHA1 if at all possible (both have been broken;
 * they are included because some file formats and protocols still depend on
 * them, and because they still have *some* use as non-crypto-secure checksums).
 * SHA2-512/256 is a good choice for performance (good performance on 64-bit,
 * somewhat resistant against length extension attacks); for state-of-the-art
 * security (at the cost of speed), use an algorithm in the SHA3 family (this
 * includes the SHAKE{128,256} algorithms).
 *
 *
 * What follows is the API reference; again, using MD5 as an example.
 *
 *
 * SYNOPSIS:
 *  TCHash_MD5* tchash_md5_init(TCHash_MD5* md5);
 * PARAMETERS:
 *  - md5: MD5 state
 * RETURN VALUE:
 *  The argument passed in as `md5`.
 * DESCRIPTION:
 *  Initialize the state of the hashing algorithm with the required values.
 *
 *  SHA2-512 has an additional initializer available:
 *
 *      TCHash_SHA2_512* tchash_sha2_512_init_ivgen(TCHash_SHA2_512* sha2_512);
 *
 *  This is the same as the ordinary SHA2-512, but with a different
 *  initialization vector; it is meant for generating the IVs for SHA2-512/t.
 *  To use it in that manner, have it hash the string "SHA2-512/t" (replacing
 *  "t" with the actual bit-length); the resulting hash is the IV for such a
 *  function (after splitting it into 8 parts).
 *
 *
 * SYNOPSIS:
 *  void tchash_md5_process(TCHash_MD5* md5, const void* data, size_t dlen);
 * PARAMETERS:
 *  - md5: algorithm state
 *  - data: raw data to process
 *  - dlen: data length in bytes
 * DESCRIPTION:
 *  Process `dlen` bytes of data, updating the algorithm state. If the length is
 *  0, the call is a no-op.
 *
 *  Thus, the following are equivalent (assuming `data` is of type `char*` and
 *  of adequate length):
 *
 *      tchash_md5_process(&md5, data, 80); // variant A
 *
 *      tchash_md5_process(&md5, data     , 20); // variant B
 *      tchash_md5_process(&md5, data + 20, 5);
 *      tchash_md5_process(&md5, data + 25, 55);
 *
 *
 * SYNOPSIS:
 *  void tchash_md5_get(TCHash_MD5* md5, void* digest);
 * PARAMETERS:
 *  - md5: algorithm state
 *  - digest: buffer for the resulting digest, at least TCHASH_MD5_DIGEST_SIZE large
 * RETURN VALUE:
 *  The resulting digest (this is the same pointer that was passed in as `digest`).
 * DESCRIPTION:
 *  Do the final transformation of input, and copy said result into `digest`.
 *
 *  Note that this does *not* modify the algorithm state --- in other words,
 *  it is perfectly acceptable to call this function, and then continue
 *  processing with more data:
 *
 *      tchash_md5_process(&md5, dataA, dataA_len);
 *      tchash_md5_get(&md5, digestA);
 *      tchash_md5_process(&md5, dataB, dataB_len);
 *      tchash_md5_get(&md5, digestB);
 *      ...
 *
 *
 * SYNOPSIS:
 *  void* tchash_md5(void* digest, const void* data, size_t dlen);
 * PARAMETERS:
 *  - digest: buffer for the resulting digest, at least TCHASH_MD5_DIGEST_SIZE large
 *  - data: raw data to process
 *  - dlen: data length in bytes
 * RETURN VALUE:
 *  The resulting digest (this is the same pointer that was passed in as `digest`).
 * DESCRIPTION:
 *  Compute a hash of the data in memory.
 *
 *  This is a shorthand for `init();process(data,dlen);get(digest)`. In fact, it
 *  is exactly equivalent to the following:
 *
 *      void* my_tchash_md5(void* digest, const void* data, size_t dlen)
 *      {
 *          TCHash_MD5 md5;
 *          tchash_md5_init(&md5);
 *          tchash_md5_process(&md5, data, dlen);
 *          return tchash_md5_get(&md5, digest);
 *      }
 *
 *
 *
 * SYNOPSIS:
 *  int tchash_secure_eq(const void* a, const void* b, size_t len);
 * PARAMETERS:
 *  - a,b: data to compare
 *  - len: length of data in bytes
 * RETURN VALUE:
 *  `1` if the data compares equal, `0` otherwise.
 * DESCRIPTION:
 *  Compare data in constant time, to protect from timing attacks.
 *
 *  This function compares the data in a way that always takes the same amount
 *  of time, even if there is an early mismatch in data. That way, it takes a
 *  constant amount of time, regardless of the location of a (potential)
 *  mismatch.
 * SEE ALSO:
 *  - [Timing Attack (Wikipedia)](https://en.wikipedia.org/wiki/Timing_attack)
 *
 *
 * SYNOPSIS:
 *  size_t tchash_xstring_from_bytes(char* str, const void* data, size_t dlen);
 * PARAMETERS:
 *  - str: buffer of length at least `dlen * 2 + 1` bytes
 *  - data: data to convert
 *  - dlen: length of data in bytes
 * RETURN VALUE:
 *  Number of characters in the resulting string, excluding a terminating `\0`.
 * DESCRIPTION:
 *  Convert raw byte data into a lowercase hexadecimal string.
 *
 *  For example, this will convert the bytes `{0x0a,0x1b,0x2c}` into
 *  `"0a1b2c"`, including the terminating `\0` (so, 6+1 characters are written).
 * SEE ALSO:
 *  - `tchash_bytes_from_xstring()`
 *
 *
 * SYNOPSIS:
 *  size_t tchash_bytes_from_xstring(void* data, const char* str, int slen);
 * PARAMETERS:
 *  - data: buffer of length at least `str_length / 2 + 1` bytes
 *  - str: string to convert
 *  - slen: length of string, or `-1` if it is NUL-terminated
 * RETURN VALUE:
 *  Number of bytes in the resulting data, or `0` on error.
 * DESCRIPTION:
 *  Convert a hexadecimal string into raw byte data.
 *
 *  Both lowercase and uppercase hexadecimal characters are supported; any space
 *  is ignored in the input. If the input is errorneous, the function will
 *  return `0` and abort conversion.
 *
 *  For example, this will convert the string "0a1 b 2c" into the raw bytes
 *  `{0x0a,0x1b,0x2c}`.
 * SEE ALSO:
 *  - `tchash_xstring_from_bytes()`
 */

#ifndef TC_HASH_H_
#define TC_HASH_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int tchash_secure_eq(const void* a, const void* b, size_t len);
size_t tchash_xstring_from_bytes(char* str, const void* data, size_t dlen);
size_t tchash_bytes_from_xstring(void* data, const char* str, int slen);


#define TCHASH_MD5_BLOCK_SIZE       64
#define TCHASH_MD5_DIGEST_SIZE      16
typedef struct TCHash_MD5
{
    uint64_t total;
    uint32_t h[4];
    union { uint32_t M[TCHASH_MD5_BLOCK_SIZE / sizeof(uint32_t)]; unsigned char b[TCHASH_MD5_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_MD5;
TCHash_MD5* tchash_md5_init(TCHash_MD5* md5);
void tchash_md5_process(TCHash_MD5* md5, const void* data, size_t dlen);
void* tchash_md5_get(TCHash_MD5* md5, void* digest);
void* tchash_md5(void* digest, const void* data, size_t dlen);


#define TCHASH_SHA1_BLOCK_SIZE      64
#define TCHASH_SHA1_DIGEST_SIZE     20
typedef struct TCHash_SHA1
{
    uint64_t total;
    uint32_t h[5];
    union { uint32_t M[TCHASH_SHA1_BLOCK_SIZE / sizeof(uint32_t)]; unsigned char b[TCHASH_SHA1_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_SHA1;
TCHash_SHA1* tchash_sha1_init(TCHash_SHA1* sha1);
void tchash_sha1_process(TCHash_SHA1* sha1, const void* data, size_t dlen);
void* tchash_sha1_get(TCHash_SHA1* sha1, void* digest);
void* tchash_sha1(void* digest, const void* data, size_t dlen);


#define TCHASH_SHA2_256_BLOCK_SIZE  64
#define TCHASH_SHA2_256_DIGEST_SIZE 32
typedef struct TCHash_SHA2_256
{
    uint64_t total;
    uint32_t h[8];
    union { uint32_t M[TCHASH_SHA2_256_BLOCK_SIZE / sizeof(uint32_t)]; unsigned char b[TCHASH_SHA2_256_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_SHA2_256;
TCHash_SHA2_256* tchash_sha2_256_init(TCHash_SHA2_256* sha2_256);
void tchash_sha2_256_process(TCHash_SHA2_256* sha2_256, const void* data, size_t dlen);
void* tchash_sha2_256_get(TCHash_SHA2_256* sha2_256, void* digest);
void* tchash_sha2_256(void* digest, const void* data, size_t dlen);

#define TCHASH_SHA2_512_BLOCK_SIZE  128
#define TCHASH_SHA2_512_DIGEST_SIZE 64
typedef struct TCHash_SHA2_512
{
    union {
        uint64_t lh[2];
#ifdef __GNUC__
//        unsigned __int128 i;
#endif /* __GNUC__ */
    } total;
    uint64_t h[8];
    union { uint64_t M[TCHASH_SHA2_512_BLOCK_SIZE / sizeof(uint64_t)]; unsigned char b[TCHASH_SHA2_512_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_SHA2_512;
TCHash_SHA2_512* tchash_sha2_512_init(TCHash_SHA2_512* sha2_512);
void tchash_sha2_512_process(TCHash_SHA2_512* sha2_512, const void* data, size_t dlen);
void* tchash_sha2_512_get(TCHash_SHA2_512* sha2_512, void* digest);
void* tchash_sha2_512(void* digest, const void* data, size_t dlen);

TCHash_SHA2_512* tchash_sha2_512_init_ivgen(TCHash_SHA2_512* sha2);

#define TCHASH_SHA2_224_BLOCK_SIZE  TCHASH_SHA2_256_BLOCK_SIZE
#define TCHASH_SHA2_224_DIGEST_SIZE 28
typedef struct TCHash_SHA2_224
{
    TCHash_SHA2_256 sha2_256;
} TCHash_SHA2_224;
TCHash_SHA2_224* tchash_sha2_224_init(TCHash_SHA2_224* sha2_224);
void tchash_sha2_224_process(TCHash_SHA2_224* sha2_224, const void* data, size_t dlen);
void* tchash_sha2_224_get(TCHash_SHA2_224* sha2_224, void* digest);
void* tchash_sha2_224(void* digest, const void* data, size_t dlen);

#define TCHASH_SHA2_384_BLOCK_SIZE  TCHASH_SHA2_512_BLOCK_SIZE
#define TCHASH_SHA2_384_DIGEST_SIZE 48
typedef struct TCHash_SHA2_384
{
    TCHash_SHA2_512 sha2_512;
} TCHash_SHA2_384;
TCHash_SHA2_384* tchash_sha2_384_init(TCHash_SHA2_384* sha2_384);
void tchash_sha2_384_process(TCHash_SHA2_384* sha2_384, const void* data, size_t dlen);
void* tchash_sha2_384_get(TCHash_SHA2_384* sha2_384, void* digest);
void* tchash_sha2_384(void* digest, const void* data, size_t dlen);

#define TCHASH_SHA2_512_224_BLOCK_SIZE  TCHASH_SHA2_512_BLOCK_SIZE
#define TCHASH_SHA2_512_224_DIGEST_SIZE 28
typedef TCHash_SHA2_512 TCHash_SHA2_512_224;
TCHash_SHA2_512_224* tchash_sha2_512_224_init(TCHash_SHA2_512_224* sha2_512_224);
void tchash_sha2_512_224_process(TCHash_SHA2_512_224* sha2_512_224, const void* data, size_t dlen);
void* tchash_sha2_512_224_get(TCHash_SHA2_512_224* sha2_512_224, void* digest);
void* tchash_sha2_512_224(void* digest, const void* data, size_t dlen);

#define TCHASH_SHA2_512_256_BLOCK_SIZE  TCHASH_SHA2_512_BLOCK_SIZE
#define TCHASH_SHA2_512_256_DIGEST_SIZE 32
typedef TCHash_SHA2_512 TCHash_SHA2_512_256;

TCHash_SHA2_512_256* tchash_sha2_512_256_init(TCHash_SHA2_512_256* sha2_512_256);
void tchash_sha2_512_256_process(TCHash_SHA2_512_256* sha2_512_256, const void* data, size_t dlen);
void* tchash_sha2_512_256_get(TCHash_SHA2_512_256* sha2_512_256, void* digest);
void* tchash_sha2_512_256(void* digest, const void* data, size_t dlen);


#define TCHASH_SHA3_224_BLOCK_SIZE  ((1600-2*224)/8)
#define TCHASH_SHA3_224_DIGEST_SIZE (224/8)
typedef struct TCHash_SHA3_224
{
    /*uint64_t total;*/
    uint64_t h[25];
    union { uint64_t M[TCHASH_SHA3_224_BLOCK_SIZE / sizeof(uint64_t)]; unsigned char b[TCHASH_SHA3_224_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_SHA3_224;
TCHash_SHA3_224* tchash_sha3_224_init(TCHash_SHA3_224* sha3_224);
void tchash_sha3_224_process(TCHash_SHA3_224* sha3_224, const void* data, size_t dlen);
void* tchash_sha3_224_get(TCHash_SHA3_224* sha3_224, void* digest);
void* tchash_sha3_224(void* digest, const void* data, size_t dlen);

#define TCHASH_SHA3_256_BLOCK_SIZE  ((1600-2*256)/8)
#define TCHASH_SHA3_256_DIGEST_SIZE (256/8)
typedef struct TCHash_SHA3_256
{
    /*uint64_t total;*/
    uint64_t h[25];
    union { uint64_t M[TCHASH_SHA3_256_BLOCK_SIZE / sizeof(uint64_t)]; unsigned char b[TCHASH_SHA3_256_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_SHA3_256;
TCHash_SHA3_256* tchash_sha3_256_init(TCHash_SHA3_256* sha3_256);
void tchash_sha3_256_process(TCHash_SHA3_256* sha3_256, const void* data, size_t dlen);
void* tchash_sha3_256_get(TCHash_SHA3_256* sha3_256, void* digest);
void* tchash_sha3_256(void* digest, const void* data, size_t dlen);

#define TCHASH_SHA3_384_BLOCK_SIZE  ((1600-2*384)/8)
#define TCHASH_SHA3_384_DIGEST_SIZE (384/8)
typedef struct TCHash_SHA3_384
{
    /*uint64_t total;*/
    uint64_t h[25];
    union { uint64_t M[TCHASH_SHA3_384_BLOCK_SIZE / sizeof(uint64_t)]; unsigned char b[TCHASH_SHA3_384_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_SHA3_384;
TCHash_SHA3_384* tchash_sha3_384_init(TCHash_SHA3_384* sha3_384);
void tchash_sha3_384_process(TCHash_SHA3_384* sha3_384, const void* data, size_t dlen);
void* tchash_sha3_384_get(TCHash_SHA3_384* sha3_384, void* digest);
void* tchash_sha3_384(void* digest, const void* data, size_t dlen);

#define TCHASH_SHA3_512_BLOCK_SIZE  ((1600-2*512)/8)
#define TCHASH_SHA3_512_DIGEST_SIZE (512/8)
typedef struct TCHash_SHA3_512
{
    /*uint64_t total;*/
    uint64_t h[25];
    union { uint64_t M[TCHASH_SHA3_512_BLOCK_SIZE / sizeof(uint64_t)]; unsigned char b[TCHASH_SHA3_512_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_SHA3_512;
TCHash_SHA3_512* tchash_sha3_512_init(TCHash_SHA3_512* sha3_512);
void tchash_sha3_512_process(TCHash_SHA3_512* sha3_512, const void* data, size_t dlen);
void* tchash_sha3_512_get(TCHash_SHA3_512* sha3_512, void* digest);
void* tchash_sha3_512(void* digest, const void* data, size_t dlen);


#define TCHASH_SHAKE128_BLOCK_SIZE  ((1600-2*128)/8)
typedef struct TCHash_SHAKE128
{
    /*uint64_t total;*/
    uint64_t h[25];
    union { uint64_t M[TCHASH_SHAKE128_BLOCK_SIZE / sizeof(uint64_t)]; unsigned char b[TCHASH_SHAKE128_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_SHAKE128;
TCHash_SHAKE128* tchash_shake128_init(TCHash_SHAKE128* shake128);
void tchash_shake128_process(TCHash_SHAKE128* shake128, const void* data, size_t dlen);
void* tchash_shake128_get(TCHash_SHAKE128* shake128, void* digest, size_t digestlen);
void* tchash_shake128(void* digest, size_t digestlen, const void* data, size_t dlen);

#define TCHASH_SHAKE256_BLOCK_SIZE  ((1600-2*256)/8)
typedef struct TCHash_SHAKE256
{
    /*uint64_t total;*/
    uint64_t h[25];
    union { uint64_t M[TCHASH_SHAKE256_BLOCK_SIZE / sizeof(uint64_t)]; unsigned char b[TCHASH_SHAKE256_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_SHAKE256;
TCHash_SHAKE256* tchash_shake256_init(TCHash_SHAKE256* shake256);
void tchash_shake256_process(TCHash_SHAKE256* shake256, const void* data, size_t dlen);
void* tchash_shake256_get(TCHash_SHAKE256* shake256, void* digest, size_t digestlen);
void* tchash_shake256(void* digest, size_t digestlen, const void* data, size_t dlen);

#ifdef __cplusplus
}
#endif

#endif /* TC_HASH_H_ */



#ifdef TC_HASH_IMPLEMENTATION
#undef TC_HASH_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>

#ifndef TC__STATIC_CAST
#ifdef __cplusplus
#define TC__STATIC_CAST(T,v) static_cast<T>(v)
#else
#define TC__STATIC_CAST(T,v) ((T)(v))
#endif
#endif /* TC__STATIC_CAST */

/* no cast done to preserve undefined function warnings in C */
#ifndef TC__VOID_CAST
#ifdef __cplusplus
#define TC__VOID_CAST(T,v)  TC__STATIC_CAST(T,v)
#else
#define TC__VOID_CAST(T,v)  (v)
#endif
#endif /* TC__VOID_CAST */

#define TCHASH_I_MIN(x,y)   ((x)<(y)?(x):(y))

static uint16_t tchash_i_swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}
static uint32_t tchash_i_swap32(uint32_t x)
{
    return ((uint32_t)tchash_i_swap16(x) << 16) | tchash_i_swap16(x >> 16);
}
static uint64_t tchash_i_swap64(uint64_t x)
{
    return ((uint64_t)tchash_i_swap32(x) << 32) | tchash_i_swap32(x >> 32);
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TCHASH_I_FROM_LE16(x)   ((uint16_t)(x))
#define TCHASH_I_FROM_LE32(x)   ((uint32_t)(x))
#define TCHASH_I_FROM_LE64(x)   ((uint64_t)(x))
#define TCHASH_I_FROM_BE16(x)   tchash_i_swap16(x)
#define TCHASH_I_FROM_BE32(x)   tchash_i_swap32(x)
#define TCHASH_I_FROM_BE64(x)   tchash_i_swap64(x)
#define TCHASH_I_TO_LE16(x)     ((uint16_t)(x))
#define TCHASH_I_TO_LE32(x)     ((uint32_t)(x))
#define TCHASH_I_TO_LE64(x)     ((uint64_t)(x))
#define TCHASH_I_TO_BE16(x)     tchash_i_swap16(x)
#define TCHASH_I_TO_BE32(x)     tchash_i_swap32(x)
#define TCHASH_I_TO_BE64(x)     tchash_i_swap64(x)
static void tchash_i_from_le32arr(uint32_t* x, size_t len) {}
static void tchash_i_from_be32arr(uint32_t* x, size_t len) { while(len--) x[len] = TCHASH_I_FROM_BE32(x[len]); }
static void tchash_i_to_le32arr(uint32_t* x, size_t len) {}
static void tchash_i_to_be32arr(uint32_t* x, size_t len) { while(len--) x[len] = TCHASH_I_TO_BE32(x[len]); }
static void tchash_i_from_le64arr(uint64_t* x, size_t len) {}
static void tchash_i_from_be64arr(uint64_t* x, size_t len) { while(len--) x[len] = TCHASH_I_FROM_BE64(x[len]); }
static void tchash_i_to_le64arr(uint64_t* x, size_t len) {}
static void tchash_i_to_be64arr(uint64_t* x, size_t len) { while(len--) x[len] = TCHASH_I_TO_BE64(x[len]); }
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define TCHASH_I_FROM_LE16(x)   tchash_i_swap16(x)
#define TCHASH_I_FROM_LE32(x)   tchash_i_swap32(x)
#define TCHASH_I_FROM_LE64(x)   tchash_i_swap64(x)
#define TCHASH_I_FROM_BE16(x)   ((uint16_t)(x))
#define TCHASH_I_FROM_BE32(x)   ((uint32_t)(x))
#define TCHASH_I_FROM_BE64(x)   ((uint64_t)(x))
#define TCHASH_I_TO_LE16(x)     tchash_i_swap16(x)
#define TCHASH_I_TO_LE32(x)     tchash_i_swap32(x)
#define TCHASH_I_TO_LE64(x)     tchash_i_swap64(x)
#define TCHASH_I_TO_BE16(x)     ((uint16_t)(x))
#define TCHASH_I_TO_BE32(x)     ((uint32_t)(x))
#define TCHASH_I_TO_BE64(x)     ((uint64_t)(x))
static void tchash_i_from_le32arr(uint32_t* x, size_t len) { while(len--) x[len] = TCHASH_I_FROM_LE32(x[len]); }
static void tchash_i_from_be32arr(uint32_t* x, size_t len) {}
static void tchash_i_to_le32arr(uint32_t* x, size_t len) { while(len--) x[len] = TCHASH_I_TO_LE32(x[len]); }
static void tchash_i_to_be32arr(uint32_t* x, size_t len) {}
static void tchash_i_from_le64arr(uint64_t* x, size_t len) { while(len--) x[len] = TCHASH_I_FROM_LE64(x[len]); }
static void tchash_i_from_be64arr(uint64_t* x, size_t len) {}
static void tchash_i_to_le64arr(uint64_t* x, size_t len) { while(len--) x[len] = TCHASH_I_TO_LE64(x[len]); }
static void tchash_i_to_be64arr(uint64_t* x, size_t len) {}
#else
#error "Unknown byte order"
#endif
static void tchash_i_to_le64arr_cpy(uint64_t* d, const uint64_t* x, size_t len) { while(len--) *d++ = TCHASH_I_FROM_LE64(*x++); }

#define TCHASH_I_U32_FROM_BYTES_LE(a,b,c,d)    ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define TCHASH_I_U32_FROM_BYTES_BE(a,b,c,d)    TCHASH_I_U32_FROM_BYTES_LE(d,c,b,a)
#define TCHASH_I_U64_FROM_BYTES_LE(a,b,c,d,e,f,g,h)    ((uint64_t)(a) | ((uint64_t)(b) << 8) | ((uint64_t)(c) << 16) | ((uint64_t)(d) << 24) | ((uint64_t)(e) << 32) | ((uint64_t)(f) << 40) | ((uint64_t)(g) << 48) | ((uint64_t)(h) << 56))
#define TCHASH_I_U64_FROM_BYTES_BE(a,b,c,d,e,f,g,h)    TCHASH_I_U64_FROM_BYTES_LE(h,g,f,e,d,c,b,a)

#define TCHASH_I_U32_FROM_BYTES_le_(a,b,c,d,e,f,g,h)    TCHASH_I_U32_FROM_BYTES_LE(a,b,c,d)
#define TCHASH_I_U32_FROM_BYTES_be_(a,b,c,d,e,f,g,h)    TCHASH_I_U32_FROM_BYTES_BE(a,b,c,d)
#define TCHASH_I_U64_FROM_BYTES_le_(a,b,c,d,e,f,g,h)    TCHASH_I_U64_FROM_BYTES_LE(a,b,c,d,e,f,g,h)
#define TCHASH_I_U64_FROM_BYTES_be_(a,b,c,d,e,f,g,h)    TCHASH_I_U64_FROM_BYTES_BE(a,b,c,d,e,f,g,h)

static uint32_t tchash_i_rotl32(uint32_t x, int k)
{
    k &= 31;
    return (x << k) | (x >> (32-k));
}
static uint32_t tchash_i_rotr32(uint32_t x, int k)
{
    k &= 31;
    return (x >> k) | (x << (32-k));
}
static uint64_t tchash_i_rotl64(uint64_t x, int k)
{
    k &= 63;
    return (x << k) | (x >> (64-k));
}
static uint64_t tchash_i_rotr64(uint64_t x, int k)
{
    k &= 63;
    return (x >> k) | (x << (64-k));
}

#define TCHASH_I_PROCESS_BODY_(LHASH,UHASH,ENDIAN,SIZE,ADDTOTAL)               \
    const unsigned char* udata = TC__VOID_CAST(const unsigned char*,data);     \
    ADDTOTAL                                                                   \
    for(;;)                                                                    \
    {                                                                          \
        /* fill buffer */                                                      \
        unsigned char clen = sizeof(LHASH->buf.b) - LHASH->blen;               \
        if(clen > dlen) clen = dlen;                                           \
        memcpy(LHASH->buf.b + LHASH->blen, udata, clen);                       \
        LHASH->blen += clen;                                                   \
        udata += clen;                                                         \
        dlen -= clen;                                                          \
        if(LHASH->blen < sizeof(LHASH->buf.b))                                 \
            break;                                                             \
        tchash_i_from_##ENDIAN##SIZE##arr(LHASH->buf.M, sizeof(LHASH->buf.M) / sizeof(*LHASH->buf.M));\
                                                                               \
        /* process chunk */                                                    \
        tchash_i_##LHASH##_process_block(LHASH->h, LHASH->buf.M);              \
        LHASH->blen = 0;                                                       \
    }

#define TCHASH_I_GET_BODY_(LHASH,UHASH,ENDIAN,SIZE,SETLENGTH,NUMM,CPYSIZE)     \
    uint##SIZE##_t h[sizeof(LHASH->h) / sizeof(*LHASH->h)];                    \
    memcpy(h, LHASH->h, sizeof(h));                                            \
    uint##SIZE##_t M[sizeof(LHASH->buf.M) / sizeof(*LHASH->buf.M)];            \
                                                                               \
    unsigned char blen = LHASH->blen;                                          \
                                                                               \
    unsigned char i = blen / sizeof(*M);                                       \
    memcpy(M, LHASH->buf.M, i * sizeof(*M));                                   \
    tchash_i_from_##ENDIAN##SIZE##arr(M, i);                                   \
                                                                               \
    unsigned char* uptr = &LHASH->buf.b[i * sizeof(*M)];                       \
    switch(blen & (sizeof(*M) - 1))                                            \
    {                                                                          \
    case 7: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], uptr[3], uptr[4], uptr[5], uptr[6], 0x80); break;\
    case 6: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], uptr[3], uptr[4], uptr[5], 0x80, 0x00); break;\
    case 5: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], uptr[3], uptr[4], 0x80, 0x00, 0x00); break;\
    case 4: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], uptr[3], 0x80, 0x00, 0x00, 0x00); break;\
    case 3: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], 0x80, 0x00, 0x00, 0x00, 0x00); break;\
    case 2: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], 0x80, 0x00, 0x00, 0x00, 0x00, 0x00); break;\
    case 1: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); break;\
    case 0: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); break;\
    }                                                                          \
    if(LHASH->blen >= sizeof(LHASH->buf.b) - sizeof(LHASH->total)) /* not enough space to put length at the end */\
    {                                                                          \
        memset(&M[i], 0, (sizeof(M) / sizeof(*M) - i) * sizeof(*M));           \
        i = 0;                                                                 \
        tchash_i_##LHASH##_process_block(h, M);                                \
    }                                                                          \
    memset(&M[i], 0, (sizeof(M) / sizeof(*M) - 2 - i) * sizeof(*M));           \
    SETLENGTH                                                                  \
    tchash_i_##LHASH##_process_block(h, M);                                    \
                                                                               \
    tchash_i_to_##ENDIAN##SIZE##arr(h, (NUMM));                                \
    memcpy(digest, h, (CPYSIZE));                                              \
    return digest;

#define TCHASH_I_SIMPLE_BODY_(LHASH,UHASH)                                     \
    TCHash_##UHASH LHASH;                                                      \
    tchash_##LHASH##_init(&LHASH);                                             \
    tchash_##LHASH##_process(&LHASH, data, dlen);                              \
    return tchash_##LHASH##_get(&LHASH, digest);

int tchash_secure_eq(const void* a, const void* b, size_t len)
{
    const unsigned char* ua = TC__VOID_CAST(const unsigned char*,a);
    const unsigned char* ub = TC__VOID_CAST(const unsigned char*,b);

    unsigned char res = 0;
    while(len--)
        res |= *ua++ ^ *ub++;
    return !!res;
}
size_t tchash_xstring_from_bytes(char* str, const void* data, size_t dlen)
{
    static const char XVals[16] = "0123456789abcdef";

    const unsigned char* udata = TC__VOID_CAST(const unsigned char*,data);
    size_t clen = dlen;
    while(clen--)
    {
        *str++ = XVals[*udata >> 4];
        *str++ = XVals[*udata & 15];
        udata++;
    }
    *str = 0;
    return 2 * dlen;
}
size_t tchash_bytes_from_xstring(void* data, const char* str, int slen)
{
    unsigned char* udata = TC__VOID_CAST(unsigned char*,data);
    if(slen < 0) slen = strlen(str);
    size_t d = 0;
    int half = 0;
    int i;
    for(i = 0; i < slen; i++)
    {
        char c = str[i];
        unsigned char h;
        if('0' <= c && c <= '9')
            h = c - '0';
        else if('A' <= c && c <= 'F')
            h = c - 'A' + 10;
        else if('a' <= c && c <= 'f')
            h = c - 'a' + 10;
        else if(c == ' ' || c == '\t' || c == '\v' || c == '\f')
            continue;
        else
            return 0; /* error */

        half = (half + 1) & 1;
        if(half)
            udata[d] = h << 4;
        else
            udata[d++] |= h;
    }
    return d;
}

TCHash_MD5* tchash_md5_init(TCHash_MD5* md5)
{
    static const uint32_t InitH[] = { UINT32_C(0x67452301), UINT32_C(0xefcdab89), UINT32_C(0x98badcfe), UINT32_C(0x10325476) };
    if(!md5) return NULL;

    md5->total = 0;
    memcpy(md5->h, InitH, sizeof(InitH));
    md5->blen = 0;

    return md5;
}
static void tchash_i_md5_process_block(uint32_t h[4], const uint32_t M[16])
{
    static const uint32_t s[] = {
        7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
        5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
        4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
        6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,
    };
    static const uint32_t K[] = {
        UINT32_C(0xd76aa478), UINT32_C(0xe8c7b756), UINT32_C(0x242070db), UINT32_C(0xc1bdceee),
        UINT32_C(0xf57c0faf), UINT32_C(0x4787c62a), UINT32_C(0xa8304613), UINT32_C(0xfd469501),
        UINT32_C(0x698098d8), UINT32_C(0x8b44f7af), UINT32_C(0xffff5bb1), UINT32_C(0x895cd7be),
        UINT32_C(0x6b901122), UINT32_C(0xfd987193), UINT32_C(0xa679438e), UINT32_C(0x49b40821),
        UINT32_C(0xf61e2562), UINT32_C(0xc040b340), UINT32_C(0x265e5a51), UINT32_C(0xe9b6c7aa),
        UINT32_C(0xd62f105d), UINT32_C(0x02441453), UINT32_C(0xd8a1e681), UINT32_C(0xe7d3fbc8),
        UINT32_C(0x21e1cde6), UINT32_C(0xc33707d6), UINT32_C(0xf4d50d87), UINT32_C(0x455a14ed),
        UINT32_C(0xa9e3e905), UINT32_C(0xfcefa3f8), UINT32_C(0x676f02d9), UINT32_C(0x8d2a4c8a),
        UINT32_C(0xfffa3942), UINT32_C(0x8771f681), UINT32_C(0x6d9d6122), UINT32_C(0xfde5380c),
        UINT32_C(0xa4beea44), UINT32_C(0x4bdecfa9), UINT32_C(0xf6bb4b60), UINT32_C(0xbebfbc70),
        UINT32_C(0x289b7ec6), UINT32_C(0xeaa127fa), UINT32_C(0xd4ef3085), UINT32_C(0x04881d05),
        UINT32_C(0xd9d4d039), UINT32_C(0xe6db99e5), UINT32_C(0x1fa27cf8), UINT32_C(0xc4ac5665),
        UINT32_C(0xf4292244), UINT32_C(0x432aff97), UINT32_C(0xab9423a7), UINT32_C(0xfc93a039),
        UINT32_C(0x655b59c3), UINT32_C(0x8f0ccc92), UINT32_C(0xffeff47d), UINT32_C(0x85845dd1),
        UINT32_C(0x6fa87e4f), UINT32_C(0xfe2ce6e0), UINT32_C(0xa3014314), UINT32_C(0x4e0811a1),
        UINT32_C(0xf7537e82), UINT32_C(0xbd3af235), UINT32_C(0x2ad7d2bb), UINT32_C(0xeb86d391),
    };

    uint32_t A = h[0];
    uint32_t B = h[1];
    uint32_t C = h[2];
    uint32_t D = h[3];

    int i;
    for(i = 0; i < 64; i++)
    {
        uint32_t F;
        uint8_t g;
        if(i < 16)
        {
            F = (B & C) | (~B & D);
            g = i;
        }
        else if(i < 32)
        {
            F = (D & B) | (~D & C);
            g = (5*i + 1) & 15;
        }
        else if(i < 48)
        {
            F = B ^ C ^ D;
            g = (3*i + 5) & 15;
        }
        else
        {
            F = C ^ (B | ~D);
            g = (7*i) & 15;
        }
        F += A + K[i] + M[g];
        A = D;
        D = C;
        C = B;
        B += tchash_i_rotl32(F, s[i]);
    }
    h[0] += A;
    h[1] += B;
    h[2] += C;
    h[3] += D;
}
void tchash_md5_process(TCHash_MD5* md5, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(md5,MD5,le,32,{md5->total += dlen;});
}
void* tchash_md5_get(TCHash_MD5* md5, void* digest)
{
    TCHASH_I_GET_BODY_(md5,MD5,le,32,{M[14] = md5->total << 3; M[15] = md5->total >> 29;},sizeof(h)/sizeof(*h),sizeof(h));
}
void* tchash_md5(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(md5,MD5) }



TCHash_SHA1* tchash_sha1_init(TCHash_SHA1* sha1)
{
    static const uint32_t InitH[] = { UINT32_C(0x67452301), UINT32_C(0xEFCDAB89), UINT32_C(0x98BADCFE), UINT32_C(0x10325476), UINT32_C(0xC3D2E1F0) };
    if(!sha1) return NULL;

    sha1->total = 0;
    memcpy(sha1->h, InitH, sizeof(InitH));
    sha1->blen = 0;

    return sha1;
}
static void tchash_i_sha1_process_block(uint32_t h[5], const uint32_t M[16])
{
    uint32_t w[80];
    memcpy(w, M, 16 * sizeof(*M));

    int i;
    for(i = 16; i < 80; i++)
        w[i] = tchash_i_rotl32(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);

    uint32_t a = h[0];
    uint32_t b = h[1];
    uint32_t c = h[2];
    uint32_t d = h[3];
    uint32_t e = h[4];

    for(i = 0; i < 80; i++)
    {
        uint32_t f, k;
        if(i < 20)
        {
            f = (b & c) | (~b & d);
            k = UINT32_C(0x5A827999);
        }
        else if(i < 40)
        {
            f = b ^ c ^ d;
            k = UINT32_C(0x6ED9EBA1);
        }
        else if(i < 60)
        {
            f = (b & c) |  (b & d) | (c & d);
            k = UINT32_C(0x8F1BBCDC);
        }
        else
        {
            f = b ^ c ^ d;
            k = UINT32_C(0xCA62C1D6);
        }
        uint32_t temp = tchash_i_rotl32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = tchash_i_rotl32(b, 30);
        b = a;
        a = temp;
    }

    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}
void tchash_sha1_process(TCHash_SHA1* sha1, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(sha1,SHA1,be,32,{sha1->total += dlen;});
}
void* tchash_sha1_get(TCHash_SHA1* sha1, void* digest)
{
    TCHASH_I_GET_BODY_(sha1,SHA1,be,32,{M[14] = sha1->total >> 29; M[15] = sha1->total << 3;},sizeof(h)/sizeof(*h),sizeof(h))
}
void* tchash_sha1(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha1,SHA1) }



TCHash_SHA2_256* tchash_sha2_256_init(TCHash_SHA2_256* sha2)
{
    static const uint32_t InitH[] = { UINT32_C(0x6a09e667), UINT32_C(0xbb67ae85), UINT32_C(0x3c6ef372), UINT32_C(0xa54ff53a), UINT32_C(0x510e527f), UINT32_C(0x9b05688c), UINT32_C(0x1f83d9ab), UINT32_C(0x5be0cd19) };
    if(!sha2) return NULL;

    sha2->total = 0;
    memcpy(sha2->h, InitH, sizeof(InitH));
    sha2->blen = 0;

    return sha2;
}
static void tchash_i_sha2_256_process_block(uint32_t H[8], const uint32_t M[16])
{
    static const uint32_t k[] = {
        UINT32_C(0x428a2f98), UINT32_C(0x71374491), UINT32_C(0xb5c0fbcf), UINT32_C(0xe9b5dba5), UINT32_C(0x3956c25b), UINT32_C(0x59f111f1), UINT32_C(0x923f82a4), UINT32_C(0xab1c5ed5),
        UINT32_C(0xd807aa98), UINT32_C(0x12835b01), UINT32_C(0x243185be), UINT32_C(0x550c7dc3), UINT32_C(0x72be5d74), UINT32_C(0x80deb1fe), UINT32_C(0x9bdc06a7), UINT32_C(0xc19bf174),
        UINT32_C(0xe49b69c1), UINT32_C(0xefbe4786), UINT32_C(0x0fc19dc6), UINT32_C(0x240ca1cc), UINT32_C(0x2de92c6f), UINT32_C(0x4a7484aa), UINT32_C(0x5cb0a9dc), UINT32_C(0x76f988da),
        UINT32_C(0x983e5152), UINT32_C(0xa831c66d), UINT32_C(0xb00327c8), UINT32_C(0xbf597fc7), UINT32_C(0xc6e00bf3), UINT32_C(0xd5a79147), UINT32_C(0x06ca6351), UINT32_C(0x14292967),
        UINT32_C(0x27b70a85), UINT32_C(0x2e1b2138), UINT32_C(0x4d2c6dfc), UINT32_C(0x53380d13), UINT32_C(0x650a7354), UINT32_C(0x766a0abb), UINT32_C(0x81c2c92e), UINT32_C(0x92722c85),
        UINT32_C(0xa2bfe8a1), UINT32_C(0xa81a664b), UINT32_C(0xc24b8b70), UINT32_C(0xc76c51a3), UINT32_C(0xd192e819), UINT32_C(0xd6990624), UINT32_C(0xf40e3585), UINT32_C(0x106aa070),
        UINT32_C(0x19a4c116), UINT32_C(0x1e376c08), UINT32_C(0x2748774c), UINT32_C(0x34b0bcb5), UINT32_C(0x391c0cb3), UINT32_C(0x4ed8aa4a), UINT32_C(0x5b9cca4f), UINT32_C(0x682e6ff3),
        UINT32_C(0x748f82ee), UINT32_C(0x78a5636f), UINT32_C(0x84c87814), UINT32_C(0x8cc70208), UINT32_C(0x90befffa), UINT32_C(0xa4506ceb), UINT32_C(0xbef9a3f7), UINT32_C(0xc67178f2)
    };

    uint32_t w[64];
    memcpy(w, M, 16 * sizeof(*M));

    int i;
    for(i = 16; i < 64; i++)
    {
        uint32_t s0 = tchash_i_rotr32(w[i-15], 7) ^ tchash_i_rotr32(w[i-15], 18) ^ (w[i-15] >> 3);
        uint32_t s1 = tchash_i_rotr32(w[i-2], 17) ^ tchash_i_rotr32(w[i-2], 19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    uint32_t a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];

    for(i = 0; i < 64; i++)
    {
        uint32_t S1 = tchash_i_rotr32(e, 6) ^ tchash_i_rotr32(e, 11) ^ tchash_i_rotr32(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t temp1 = h + S1 + ch + k[i] + w[i];
        uint32_t S0 = tchash_i_rotr32(a, 2) ^ tchash_i_rotr32(a, 13) ^ tchash_i_rotr32(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    H[0] += a;
    H[1] += b;
    H[2] += c;
    H[3] += d;
    H[4] += e;
    H[5] += f;
    H[6] += g;
    H[7] += h;
}
void tchash_sha2_256_process(TCHash_SHA2_256* sha2_256, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(sha2_256,SHA2_256,be,32,{sha2_256->total += dlen;});
}
void* tchash_sha2_256_get(TCHash_SHA2_256* sha2_256, void* digest)
{
    TCHASH_I_GET_BODY_(sha2_256,SHA2_256,be,32,{M[14] = sha2_256->total >> 29; M[15] = sha2_256->total << 3;},sizeof(h)/sizeof(*h),sizeof(h))
}
void* tchash_sha2_256(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha2_256,SHA2_256) }

TCHash_SHA2_512* tchash_sha2_512_init(TCHash_SHA2_512* sha2)
{
    static const uint64_t InitH[] = {
            UINT64_C(0x6a09e667f3bcc908), UINT64_C(0xbb67ae8584caa73b), UINT64_C(0x3c6ef372fe94f82b), UINT64_C(0xa54ff53a5f1d36f1),
            UINT64_C(0x510e527fade682d1), UINT64_C(0x9b05688c2b3e6c1f), UINT64_C(0x1f83d9abfb41bd6b), UINT64_C(0x5be0cd19137e2179) };
    if(!sha2) return NULL;

    sha2->total.lh[0] = sha2->total.lh[1] = 0;
    memcpy(sha2->h, InitH, sizeof(InitH));
    sha2->blen = 0;

    return sha2;
}
static void tchash_i_sha2_512_process_block(uint64_t H[8], const uint64_t M[16])
{
    static const uint64_t k[] = {
        UINT64_C(0x428a2f98d728ae22), UINT64_C(0x7137449123ef65cd), UINT64_C(0xb5c0fbcfec4d3b2f), UINT64_C(0xe9b5dba58189dbbc), UINT64_C(0x3956c25bf348b538),
        UINT64_C(0x59f111f1b605d019), UINT64_C(0x923f82a4af194f9b), UINT64_C(0xab1c5ed5da6d8118), UINT64_C(0xd807aa98a3030242), UINT64_C(0x12835b0145706fbe),
        UINT64_C(0x243185be4ee4b28c), UINT64_C(0x550c7dc3d5ffb4e2), UINT64_C(0x72be5d74f27b896f), UINT64_C(0x80deb1fe3b1696b1), UINT64_C(0x9bdc06a725c71235),
        UINT64_C(0xc19bf174cf692694), UINT64_C(0xe49b69c19ef14ad2), UINT64_C(0xefbe4786384f25e3), UINT64_C(0x0fc19dc68b8cd5b5), UINT64_C(0x240ca1cc77ac9c65),
        UINT64_C(0x2de92c6f592b0275), UINT64_C(0x4a7484aa6ea6e483), UINT64_C(0x5cb0a9dcbd41fbd4), UINT64_C(0x76f988da831153b5), UINT64_C(0x983e5152ee66dfab),
        UINT64_C(0xa831c66d2db43210), UINT64_C(0xb00327c898fb213f), UINT64_C(0xbf597fc7beef0ee4), UINT64_C(0xc6e00bf33da88fc2), UINT64_C(0xd5a79147930aa725),
        UINT64_C(0x06ca6351e003826f), UINT64_C(0x142929670a0e6e70), UINT64_C(0x27b70a8546d22ffc), UINT64_C(0x2e1b21385c26c926), UINT64_C(0x4d2c6dfc5ac42aed),
        UINT64_C(0x53380d139d95b3df), UINT64_C(0x650a73548baf63de), UINT64_C(0x766a0abb3c77b2a8), UINT64_C(0x81c2c92e47edaee6), UINT64_C(0x92722c851482353b),
        UINT64_C(0xa2bfe8a14cf10364), UINT64_C(0xa81a664bbc423001), UINT64_C(0xc24b8b70d0f89791), UINT64_C(0xc76c51a30654be30), UINT64_C(0xd192e819d6ef5218),
        UINT64_C(0xd69906245565a910), UINT64_C(0xf40e35855771202a), UINT64_C(0x106aa07032bbd1b8), UINT64_C(0x19a4c116b8d2d0c8), UINT64_C(0x1e376c085141ab53),
        UINT64_C(0x2748774cdf8eeb99), UINT64_C(0x34b0bcb5e19b48a8), UINT64_C(0x391c0cb3c5c95a63), UINT64_C(0x4ed8aa4ae3418acb), UINT64_C(0x5b9cca4f7763e373),
        UINT64_C(0x682e6ff3d6b2b8a3), UINT64_C(0x748f82ee5defb2fc), UINT64_C(0x78a5636f43172f60), UINT64_C(0x84c87814a1f0ab72), UINT64_C(0x8cc702081a6439ec),
        UINT64_C(0x90befffa23631e28), UINT64_C(0xa4506cebde82bde9), UINT64_C(0xbef9a3f7b2c67915), UINT64_C(0xc67178f2e372532b), UINT64_C(0xca273eceea26619c),
        UINT64_C(0xd186b8c721c0c207), UINT64_C(0xeada7dd6cde0eb1e), UINT64_C(0xf57d4f7fee6ed178), UINT64_C(0x06f067aa72176fba), UINT64_C(0x0a637dc5a2c898a6),
        UINT64_C(0x113f9804bef90dae), UINT64_C(0x1b710b35131c471b), UINT64_C(0x28db77f523047d84), UINT64_C(0x32caab7b40c72493), UINT64_C(0x3c9ebe0a15c9bebc),
        UINT64_C(0x431d67c49c100d4c), UINT64_C(0x4cc5d4becb3e42b6), UINT64_C(0x597f299cfc657e2a), UINT64_C(0x5fcb6fab3ad6faec), UINT64_C(0x6c44198c4a475817)
    };

    uint64_t w[80];
    memcpy(w, M, 16 * sizeof(*M));

    int i;
    for(i = 16; i < 80; i++)
    {
        uint64_t s0 = tchash_i_rotr64(w[i-15], 1) ^ tchash_i_rotr64(w[i-15], 8) ^ (w[i-15] >> 7);
        uint64_t s1 = tchash_i_rotr64(w[i-2], 19) ^ tchash_i_rotr64(w[i-2], 61) ^ (w[i-2] >> 6);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    uint64_t a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];

    for(i = 0; i < 80; i++)
    {
        uint64_t S1 = tchash_i_rotr64(e, 14) ^ tchash_i_rotr64(e, 18) ^ tchash_i_rotr64(e, 41);
        uint64_t ch = (e & f) ^ (~e & g);
        uint64_t temp1 = h + S1 + ch + k[i] + w[i];
        uint64_t S0 = tchash_i_rotr64(a, 28) ^ tchash_i_rotr64(a, 34) ^ tchash_i_rotr64(a, 39);
        uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint64_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    H[0] += a;
    H[1] += b;
    H[2] += c;
    H[3] += d;
    H[4] += e;
    H[5] += f;
    H[6] += g;
    H[7] += h;
}
void tchash_sha2_512_process(TCHash_SHA2_512* sha2_512, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(sha2_512,SHA2_512,be,64,{uint64_t prevl = sha2_512->total.lh[0]; sha2_512->total.lh[0] += dlen; if(sha2_512->total.lh[0] < prevl) sha2_512->total.lh[1]++;});
}
void* tchash_sha2_512_get(TCHash_SHA2_512* sha2_512, void* digest)
{
    TCHASH_I_GET_BODY_(sha2_512,SHA2_512,be,64,{M[14] = (sha2_512->total.lh[1] << 3) | (sha2_512->total.lh[0] >> (64-3)); M[15] = sha2_512->total.lh[0] << 3;},sizeof(h)/sizeof(*h),sizeof(h))
}
void* tchash_sha2_512(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha2_512,SHA2_512) }

TCHash_SHA2_512* tchash_sha2_512_init_ivgen(TCHash_SHA2_512* sha2)
{
    if(!tchash_sha2_512_init(sha2)) return NULL;
    int i;
    for(i = 0; i < sizeof(sha2->h) / sizeof(*sha2->h); i++)
        sha2->h[i] ^= UINT64_C(0xa5a5a5a5a5a5a5a5);
    return sha2;
}

TCHash_SHA2_224* tchash_sha2_224_init(TCHash_SHA2_224* sha2)
{
    static const uint32_t InitH[] = { UINT32_C(0xc1059ed8), UINT32_C(0x367cd507), UINT32_C(0x3070dd17), UINT32_C(0xf70e5939), UINT32_C(0xffc00b31), UINT32_C(0x68581511), UINT32_C(0x64f98fa7), UINT32_C(0xbefa4fa4) };
    if(!sha2) return NULL;

    sha2->sha2_256.total = 0;
    memcpy(sha2->sha2_256.h, InitH, sizeof(InitH));
    sha2->sha2_256.blen = 0;

    return sha2;
}
void tchash_sha2_224_process(TCHash_SHA2_224* sha2_224, const void* data, size_t dlen)
{
    tchash_sha2_256_process(&sha2_224->sha2_256, data, dlen);
}
void* tchash_sha2_224_get(TCHash_SHA2_224* sha2_224, void* digest)
{
    TCHash_SHA2_256* sha2_256 = &sha2_224->sha2_256;
    TCHASH_I_GET_BODY_(sha2_256,SHA2_256,be,32,{M[14] = sha2_256->total >> 29; M[15] = sha2_256->total << 3;},sizeof(h)/sizeof(*h) - 1,TCHASH_SHA2_224_DIGEST_SIZE)
}
void* tchash_sha2_224(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha2_224,SHA2_224) }

TCHash_SHA2_384* tchash_sha2_384_init(TCHash_SHA2_384* sha2)
{
    static const uint64_t InitH[] = {
        UINT64_C(0xcbbb9d5dc1059ed8), UINT64_C(0x629a292a367cd507), UINT64_C(0x9159015a3070dd17), UINT64_C(0x152fecd8f70e5939),
        UINT64_C(0x67332667ffc00b31), UINT64_C(0x8eb44a8768581511), UINT64_C(0xdb0c2e0d64f98fa7), UINT64_C(0x47b5481dbefa4fa4) };
    if(!sha2) return NULL;

    sha2->sha2_512.total.lh[0] = sha2->sha2_512.total.lh[1] = 0;
    memcpy(sha2->sha2_512.h, InitH, sizeof(InitH));
    sha2->sha2_512.blen = 0;

    return sha2;
}
void tchash_sha2_384_process(TCHash_SHA2_384* sha2_384, const void* data, size_t dlen)
{
    tchash_sha2_512_process(&sha2_384->sha2_512, data, dlen);
}
void* tchash_sha2_384_get(TCHash_SHA2_384* sha2_384, void* digest)
{
    TCHash_SHA2_512* sha2_512 = &sha2_384->sha2_512;
    TCHASH_I_GET_BODY_(sha2_512,SHA2_512,be,64,{M[14] = (sha2_512->total.lh[1] << 3) | (sha2_512->total.lh[0] >> (64-3)); M[15] = sha2_512->total.lh[0] << 3;},sizeof(h)/sizeof(*h) - 2,TCHASH_SHA2_384_DIGEST_SIZE)
}
void* tchash_sha2_384(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha2_384,SHA2_384) }

TCHash_SHA2_512* tchash_sha2_512_224_init(TCHash_SHA2_512* sha2)
{
    static const uint64_t InitH[] = {
        UINT64_C(0x8c3d37c819544da2), UINT64_C(0x73e1996689dcd4d6), UINT64_C(0x1dfab7ae32ff9c82), UINT64_C(0x679dd514582f9fcf),
        UINT64_C(0x0f6d2b697bd44da8), UINT64_C(0x77e36f7304c48942), UINT64_C(0x3f9d85a86a1d36c8), UINT64_C(0x1112e6ad91d692a1) };
    if(!sha2) return NULL;

    sha2->total.lh[0] = sha2->total.lh[1] = 0;
    memcpy(sha2->h, InitH, sizeof(InitH));
    sha2->blen = 0;

    return sha2;
}
void tchash_sha2_512_224_process(TCHash_SHA2_512* sha2_512, const void* data, size_t dlen)
{
    tchash_sha2_512_process(sha2_512, data, dlen);
}
void* tchash_sha2_512_224_get(TCHash_SHA2_512* sha2_512, void* digest)
{
    TCHASH_I_GET_BODY_(sha2_512,SHA2_512,be,64,{M[14] = (sha2_512->total.lh[1] << 3) | (sha2_512->total.lh[0] >> (64-3)); M[15] = sha2_512->total.lh[0] << 3;},sizeof(h)/sizeof(*h) - 4,TCHASH_SHA2_512_224_DIGEST_SIZE)
}
void* tchash_sha2_512_224(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha2_512_224,SHA2_512_224) }

TCHash_SHA2_512* tchash_sha2_512_256_init(TCHash_SHA2_512* sha2)
{
    static const uint64_t InitH[] = {
        UINT64_C(0x22312194fc2bf72c), UINT64_C(0x9f555fa3c84c64c2), UINT64_C(0x2393b86b6f53b151), UINT64_C(0x963877195940eabd),
        UINT64_C(0x96283ee2a88effe3), UINT64_C(0xbe5e1e2553863992), UINT64_C(0x2b0199fc2c85b8aa), UINT64_C(0x0eb72ddc81c52ca2) };
    if(!sha2) return NULL;

    sha2->total.lh[0] = sha2->total.lh[1] = 0;
    memcpy(sha2->h, InitH, sizeof(InitH));
    sha2->blen = 0;

    return sha2;
}
void tchash_sha2_512_256_process(TCHash_SHA2_512* sha2_512, const void* data, size_t dlen)
{
    tchash_sha2_512_process(sha2_512, data, dlen);
}
void* tchash_sha2_512_256_get(TCHash_SHA2_512* sha2_512, void* digest)
{
    TCHASH_I_GET_BODY_(sha2_512,SHA2_512,be,64,{M[14] = (sha2_512->total.lh[1] << 3) | (sha2_512->total.lh[0] >> (64-3)); M[15] = sha2_512->total.lh[0] << 3;},sizeof(h)/sizeof(*h) - 4,TCHASH_SHA2_512_256_DIGEST_SIZE)
}
void* tchash_sha2_512_256(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha2_512_256,SHA2_512_256) }



#define TCHASH_I_KECCAK1600_L   6
/* was used to compute the tables */
/* 5: rc(t)     */
/*static unsigned char tchash_i_keccak_p1600_rc_bit(int t)
{
    t %= 255;
    unsigned char R = 1;
    int i;
    for(i = 0; i < t; i++)
    {
        unsigned char b = R >> 7;
        R <<= 1;
        R ^= b | (b << 4) | (b << 5) | (b << 6);
    }
    return R & 1;
}
static uint64_t tchash_i_keccak_p1600_rc(int ir, int l)
{
    uint64_t RC = 0;
    int j;
    for(j = 0; j <= l; j++)
        RC |= (uint64_t)tchash_i_keccak_p1600_rc_bit(j + 7 * ir) << ((1 << j) - 1);
    return RC;
}*/
#define A_(A,x,y)  (A)[(y)*5+(x)]
// http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf
static void tchash_i_keccak_p1600(uint64_t A[25], int nrounds)
{
    static const uint64_t RC[12 + 2 * TCHASH_I_KECCAK1600_L] = {
        UINT64_C(0x0000000000000001),UINT64_C(0x0000000000008082),UINT64_C(0x800000000000808A),UINT64_C(0x8000000080008000),
        UINT64_C(0x000000000000808B),UINT64_C(0x0000000080000001),UINT64_C(0x8000000080008081),UINT64_C(0x8000000000008009),
        UINT64_C(0x000000000000008A),UINT64_C(0x0000000000000088),UINT64_C(0x0000000080008009),UINT64_C(0x000000008000000A),
        UINT64_C(0x000000008000808B),UINT64_C(0x800000000000008B),UINT64_C(0x8000000000008089),UINT64_C(0x8000000000008003),
        UINT64_C(0x8000000000008002),UINT64_C(0x8000000000000080),UINT64_C(0x000000000000800A),UINT64_C(0x800000008000000A),
        UINT64_C(0x8000000080008081),UINT64_C(0x8000000000008080),UINT64_C(0x0000000080000001),UINT64_C(0x8000000080008008),
    };
    if(nrounds < 0) nrounds = 12 + 2 * TCHASH_I_KECCAK1600_L;

    uint64_t C[5];
    uint64_t tmpA[25];
    uint64_t* newA[2];
    int curA = 0;

    newA[!curA] = A;
    newA[curA] = tmpA;

    int ir;
    for(ir = 12 + 2 * TCHASH_I_KECCAK1600_L - nrounds; ir < 12 + 2 * TCHASH_I_KECCAK1600_L; ir++)
    {
        int x, y, t;

        /* 1: theta(A)  */
        for(x = 0; x < 5; x++)
            C[x] = A_(newA[!curA],x,0) ^ A_(newA[!curA],x,1) ^ A_(newA[!curA],x,2) ^ A_(newA[!curA],x,3) ^ A_(newA[!curA],x,4);
        for(x = 0; x < 5; x++)
        {
            uint64_t Dx = C[(x+5-1)%5] ^ tchash_i_rotl64(C[(x+1)%5], 1);
            for(y = 0; y < 5; y++)
                A_(newA[curA],x,y) = A_(newA[!curA],x,y) ^ Dx;
        }
        curA = !curA;

        /* 2: rho(A)    */
        newA[curA][0] = newA[!curA][0];
        x = 1; y = 0;
        for(t = 0; t < 24; t++)
        {
            A_(newA[curA],x,y) = tchash_i_rotl64(A_(newA[!curA],x,y), (t+1)*(t+2)/2);
            int nx = y;
            y = (2*x+3*y) % 5;
            x = nx;
        }
        curA = !curA;

        /* 3: pi(A)     */
        for(x = 0; x < 5; x++)
            for(y = 0; y < 5; y++)
                //A_(newA[curA],(2*y+3*x)%5,x) = A_(newA[!curA],x,y);//WP
                A_(newA[curA],x,y) = A_(newA[!curA],(x+3*y)%5,x);
        curA = !curA;

        /* 4: chi(A)    */
        for(x = 0; x < 5; x++)
            for(y = 0; y < 5; y++)
                A_(newA[curA],x,y) = A_(newA[!curA],x,y) ^ (~A_(newA[!curA],(x+1)%5,y) & A_(newA[!curA],(x+2)%5,y));
        curA = !curA;

        /* 6: iota(a,ri)*/
        newA[!curA][0] ^= RC[ir];
        /* do *not* flip curA here */
    }
    if(newA[!curA] != A)
        memcpy(A, newA[!curA], sizeof(tmpA));
}
#undef A_
static void tchash_i_keccak1600_process_block(uint64_t h[25], uint64_t* M, size_t bsize, int nrounds)
{
    size_t i;
    for(i = 0; i < bsize / sizeof(*M); i++)
        h[i] ^= M[i];
    tchash_i_keccak_p1600(h, nrounds);
}

#define TCHASH_I_GETKECCAK_BODY_(LHASH,UHASH,DIGESTSIZE,PADBITS,NPADBITS)      \
    TCHash_##UHASH sha3 = *LHASH;                                              \
                                                                               \
    unsigned char i = sha3.blen;                                               \
    /* S=(01); P=(1 0* 1) */                                                   \
    sha3.buf.b[i++] = (PADBITS) | (1 << (NPADBITS)); /* (?) (1 0* ... */       \
    memset(&sha3.buf.b[i], 0x00, sizeof(sha3.buf.b) - i);                      \
    sha3.buf.b[sizeof(sha3.buf.b) - 1] |= 0x80; /* ... 1) */                   \
                                                                               \
    tchash_i_from_le64arr(sha3.buf.M, sizeof(sha3.buf.M) / sizeof(*sha3.buf.M));\
    tchash_i_##LHASH##_process_block(sha3.h, sha3.buf.M);                      \
                                                                               \
    tchash_i_to_le64arr(sha3.h, (DIGESTSIZE + (sizeof(*sha3.h) - 1)) / sizeof(*sha3.h));\
    memcpy(digest, sha3.h, DIGESTSIZE);                                        \
    return digest;
#define TCHASH_I_GETKECCAK_BODY_VARYING_(LHASH,UHASH,DIGESTSIZE,PADBITS,NPADBITS)\
    TCHash_##UHASH sha3 = *LHASH;                                              \
    uint64_t outh[sizeof(sha3.h) / sizeof(*sha3.h)];                           \
                                                                               \
    unsigned char i = sha3.blen;                                               \
    /* S=(01); P=(1 0* 1) */                                                   \
    sha3.buf.b[i++] = (PADBITS) | (1 << (NPADBITS)); /* (?) (1 0* ... */       \
    memset(&sha3.buf.b[i], 0x00, sizeof(sha3.buf.b) - i);                      \
    sha3.buf.b[sizeof(sha3.buf.b) - 1] |= 0x80; /* ... 1) */                   \
                                                                               \
    tchash_i_from_le64arr(sha3.buf.M, sizeof(sha3.buf.M) / sizeof(*sha3.buf.M));\
    tchash_i_##LHASH##_process_block(sha3.h, sha3.buf.M);                      \
                                                                               \
    size_t remaining = (DIGESTSIZE);                                           \
    char* cdigest = digest;                                                    \
    for(;;)                                                                    \
    {                                                                          \
        size_t clen = remaining < sizeof(sha3.buf.b) ? remaining : sizeof(sha3.buf.b);\
        tchash_i_to_le64arr_cpy(outh, sha3.h, (clen + (sizeof(*sha3.h) - 1)) / sizeof(*sha3.h));\
        memcpy(cdigest, outh, clen);                                           \
        cdigest += clen;                                                       \
        remaining -= clen;                                                     \
        if(!remaining) break;                                                  \
        tchash_i_keccak_p1600(sha3.h, -1);                                     \
    }                                                                          \
    return digest;
#define TCHASH_I_GETKECCAK_BODY_SHA3_(LHASH,UHASH)   TCHASH_I_GETKECCAK_BODY_(LHASH,UHASH,TCHASH_##UHASH##_DIGEST_SIZE,0x02,2)
#define TCHASH_I_GETKECCAK_BODY_RAWSHAKE_(LHASH,UHASH,DIGESTSIZE,PADBITS,NPADBITS)  TCHASH_I_GETKECCAK_BODY_VARYING_(LHASH,UHASH,DIGESTSIZE,0x03 | ((PADBITS) << (NPADBITS)),2 + (NPADBITS))
#define TCHASH_I_GETKECCAK_BODY_SHAKE_(LHASH,UHASH,DIGESTSIZE)  TCHASH_I_GETKECCAK_BODY_RAWSHAKE_(LHASH,UHASH,DIGESTSIZE,0x03,2)

TCHash_SHA3_224* tchash_sha3_224_init(TCHash_SHA3_224* sha3_224)
{
    if(!sha3_224) return NULL;

    /*sha3_224->total = 0;*/
    memset(sha3_224->h, 0, sizeof(sha3_224->h));
    sha3_224->blen = 0;

    return sha3_224;
}
static void tchash_i_sha3_224_process_block(uint64_t h[25], uint64_t M[9]) { tchash_i_keccak1600_process_block(h, M, TCHASH_SHA3_224_BLOCK_SIZE, -1); }
void tchash_sha3_224_process(TCHash_SHA3_224* sha3_224, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(sha3_224,SHA3_224,le,64,/*{sha3_224->total+=dlen;}*/)
}
void* tchash_sha3_224_get(TCHash_SHA3_224* sha3_224, void* digest)
{
    TCHASH_I_GETKECCAK_BODY_SHA3_(sha3_224,SHA3_224)
}
void* tchash_sha3_224(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha3_224,SHA3_224) }

TCHash_SHA3_256* tchash_sha3_256_init(TCHash_SHA3_256* sha3_256)
{
    if(!sha3_256) return NULL;

    /*sha3_256->total = 0;*/
    memset(sha3_256->h, 0, sizeof(sha3_256->h));
    sha3_256->blen = 0;

    return sha3_256;
}
static void tchash_i_sha3_256_process_block(uint64_t h[25], uint64_t M[9]) { tchash_i_keccak1600_process_block(h, M, TCHASH_SHA3_256_BLOCK_SIZE, -1); }
void tchash_sha3_256_process(TCHash_SHA3_256* sha3_256, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(sha3_256,SHA3_256,le,64,/*{sha3_256->total+=dlen;}*/)
}
void* tchash_sha3_256_get(TCHash_SHA3_256* sha3_256, void* digest)
{
    TCHASH_I_GETKECCAK_BODY_SHA3_(sha3_256,SHA3_256)
}
void* tchash_sha3_256(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha3_256,SHA3_256) }

TCHash_SHA3_384* tchash_sha3_384_init(TCHash_SHA3_384* sha3_384)
{
    if(!sha3_384) return NULL;

    /*sha3_384->total = 0;*/
    memset(sha3_384->h, 0, sizeof(sha3_384->h));
    sha3_384->blen = 0;

    return sha3_384;
}
static void tchash_i_sha3_384_process_block(uint64_t h[25], uint64_t M[9]) { tchash_i_keccak1600_process_block(h, M, TCHASH_SHA3_384_BLOCK_SIZE, -1); }
void tchash_sha3_384_process(TCHash_SHA3_384* sha3_384, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(sha3_384,SHA3_384,le,64,/*{sha3_384->total+=dlen;}*/)
}
void* tchash_sha3_384_get(TCHash_SHA3_384* sha3_384, void* digest)
{
    TCHASH_I_GETKECCAK_BODY_SHA3_(sha3_384,SHA3_384)
}
void* tchash_sha3_384(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha3_384,SHA3_384) }

TCHash_SHA3_512* tchash_sha3_512_init(TCHash_SHA3_512* sha3_512)
{
    if(!sha3_512) return NULL;

    /*sha3_512->total = 0;*/
    memset(sha3_512->h, 0, sizeof(sha3_512->h));
    sha3_512->blen = 0;

    return sha3_512;
}
static void tchash_i_sha3_512_process_block(uint64_t h[25], uint64_t M[9]) { tchash_i_keccak1600_process_block(h, M, TCHASH_SHA3_512_BLOCK_SIZE, -1); }
void tchash_sha3_512_process(TCHash_SHA3_512* sha3_512, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(sha3_512,SHA3_512,le,64,/*{sha3_512->total+=dlen;}*/)
}
void* tchash_sha3_512_get(TCHash_SHA3_512* sha3_512, void* digest)
{
    TCHASH_I_GETKECCAK_BODY_SHA3_(sha3_512,SHA3_512)
}
void* tchash_sha3_512(void* digest, const void* data, size_t dlen) { TCHASH_I_SIMPLE_BODY_(sha3_512,SHA3_512) }



TCHash_SHAKE128* tchash_shake128_init(TCHash_SHAKE128* shake128)
{
    if(!shake128) return NULL;

    /*shake128->total = 0;*/
    memset(shake128->h, 0, sizeof(shake128->h));
    shake128->blen = 0;

    return shake128;
}
static void tchash_i_shake128_process_block(uint64_t h[25], uint64_t M[9]) { tchash_i_keccak1600_process_block(h, M, TCHASH_SHAKE128_BLOCK_SIZE, -1); }
void tchash_shake128_process(TCHash_SHAKE128* shake128, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(shake128,SHAKE128,le,64,/*{shake128->total+=dlen;}*/)
}
void* tchash_shake128_get(TCHash_SHAKE128* shake128, void* digest, size_t digestlen)
{
    TCHASH_I_GETKECCAK_BODY_SHAKE_(shake128,SHAKE128,digestlen)
}
void* tchash_shake128(void* digest, size_t digestlen, const void* data, size_t dlen)
{
    TCHash_SHAKE128 shake128;
    tchash_shake128_init(&shake128);
    tchash_shake128_process(&shake128, data, dlen);
    return tchash_shake128_get(&shake128, digest, digestlen);
}

TCHash_SHAKE256* tchash_shake256_init(TCHash_SHAKE256* shake256)
{
    if(!shake256) return NULL;

    /*shake256->total = 0;*/
    memset(shake256->h, 0, sizeof(shake256->h));
    shake256->blen = 0;

    return shake256;
}
static void tchash_i_shake256_process_block(uint64_t h[25], uint64_t M[9]) { tchash_i_keccak1600_process_block(h, M, TCHASH_SHAKE256_BLOCK_SIZE, -1); }
void tchash_shake256_process(TCHash_SHAKE256* shake256, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(shake256,SHAKE256,le,64,/*{shake256->total+=dlen;}*/)
}
void* tchash_shake256_get(TCHash_SHAKE256* shake256, void* digest, size_t digestlen)
{
    TCHASH_I_GETKECCAK_BODY_SHAKE_(shake256,SHAKE256,digestlen)
}
void* tchash_shake256(void* digest, size_t digestlen, const void* data, size_t dlen)
{
    TCHash_SHAKE256 shake256;
    tchash_shake256_init(&shake256);
    tchash_shake256_process(&shake256, data, dlen);
    return tchash_shake256_get(&shake256, digest, digestlen);
}

#endif /* TC_HASH_IMPLEMENTATION */
