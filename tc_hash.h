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
 * 0.0.3    changed `tchash_xstring_from_bytes()` to accept an `uppercase` parameter
 *          fixed a theoretical bug with uninitialized data in some cases (by sheer dumb luck, the bug did not affect any existing implementations)
 *          added Tiger and Tiger2 hashes (Tiger{,2}/{192,160,128})
 *          added Base64 string conversion
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
 * Some algorithms (such as SHAKE128 and SHAKE256) are an exception here, since
 * they have variable-length digests. They take an additional parameter, the
 * size of the digest, in the `get()` function. For this reason, there is also
 * no equivalent `DIGEST_SIZE` constant. If such a constant *does* exist (such
 * as for the Tiger and Tiger2 hashes), it signifies the *maximum* digest size
 * for a specific algorithm.
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
 * - Tiger
 *      - Tiger/192
 *      - Tiger/160
 *      - Tiger/128
 *      - Tiger2/192
 *      - Tiger2/160
 *      - Tiger2/128
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
 *  size_t tchash_xstring_from_bytes(char* str, const void* data, size_t dlen, int uppercase);
 * PARAMETERS:
 *  - str: buffer of length at least `dlen * 2 + 1` bytes
 *  - data: data to convert
 *  - dlen: length of data in bytes
 *  - uppercase: whether the resulting string should be uppercase
 * RETURN VALUE:
 *  Number of characters in the resulting string, excluding a terminating `\0`.
 * DESCRIPTION:
 *  Convert raw byte data into a hexadecimal string.
 *
 *  For example, this will convert the bytes `{0x0a,0x1b,0x2c}` into `"0a1b2c"`,
 *  including the terminating `'\0'` (so, 6+1 characters are written).
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
 *  Both lowercase and uppercase hexadecimal characters are supported;
 *  whitespace is ignored in the input. If the input is errorneous, the function
 *  will return `0` and abort conversion.
 *
 *  For example, this will convert the string `"0a1 b 2c"` into the raw bytes
 *  `{0x0a,0x1b,0x2c}`.
 * SEE ALSO:
 *  - `tchash_xstring_from_bytes()`
 *
 *
 * SYNOPSIS:
 *  size_t tchash_base64_from_bytes(char* str, const void* data, size_t dlen, int c62, int c63, int cpad);
 * PARAMETERS:
 *  - str: buffer of length at least `dlen * 4 + 1` bytes
 *  - data: data to convert
 *  - dlen: length of data in bytes
 *  - c62,c63: character to use for the values `62` and `63`, respectively, or
 *             `-1` to use the defaults ('+' and '/')
 *  - cpad: character to use for the padding, `-1` to use the default ('='), or
 *          `0` to disable
 * RETURN VALUE:
 *  Number of characters in the result string, exclusing the terminating `\0`.
 * DESCRIPTION:
 *  Convert raw byte data into a Base64 string.
 *
 *  For example, this will convert the bytes `{0x4d,0x61}` into `"TWQ="`,
 *  including the terminating `'\0'` (so, 4+1 characters are written).
 *
 *  The character set is as follows (in order for values from 0 to 63; the
 *  padding character is '='):
 *  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/", where
 *  the last two characters are customizable.
 * SEE ALSO:
 *  - `tchash_bytes_from_base64()`
 *
 *
 * SYNOPSIS:
 *  size_t tchash_bytes_from_base64(void* data, const char* str, int slen, int c62, int c63, int cpad);
 * PARAMETERS:
 *  - data: buffer of length at least `str_length * 3 / 4` bytes
 *  - str: string to convert
 *  - slen: length of string, or `-1` if it is NUL-terminated
 *  - c62,c63: character to use for the values `62` and `63`, respectively, or
 *             `-1` to use the defaults ('+' and '/')
 *  - cpad: character to use for the padding, `-1` to use the default ('='), or
 *          `0` to disable
 * RETURN VALUE:
 *  Number of bytes in the resulting data, or `0` on error.
 * DESCRIPTION:
 *  Convert a Base64 string into raw byte data.
 *
 *  If the input is errorneous, the function will return `0` and abort
 *  conversion.
 *
 *  For example, this will convert the string `"TWQ="` into bytes `{0x4d,0x61}`.
 * SEE ALSO:
 *  - `tchash_base64_from_bytes()`
 */

#ifndef TC_HASH_H_
#define TC_HASH_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int tchash_secure_eq(const void* a, const void* b, size_t len);
size_t tchash_xstring_from_bytes(char* str, const void* data, size_t dlen, int uppercase);
size_t tchash_bytes_from_xstring(void* data, const char* str, int slen);
size_t tchash_base64_from_bytes(char* str, const void* data, size_t dlen, int c62, int c63, int cpad);
size_t tchash_bytes_from_base64(void* data, const char* str, int slen, int c62, int c63, int cpad);


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


#define TCHASH_TIGER_BLOCK_SIZE     (512/8)
#define TCHASH_TIGER192_DIGEST_SIZE (192/8)
#define TCHASH_TIGER160_DIGEST_SIZE (160/8)
#define TCHASH_TIGER128_DIGEST_SIZE (128/8)
typedef struct TCHash_Tiger
{
    uint64_t total;
    uint64_t h[3];
    union { uint64_t M[TCHASH_TIGER_BLOCK_SIZE / sizeof(uint64_t)]; unsigned char b[TCHASH_TIGER_BLOCK_SIZE]; } buf;
    unsigned char blen;
} TCHash_Tiger;
TCHash_Tiger* tchash_tiger_init(TCHash_Tiger* tiger);
#define tchash_tiger192_init    tchash_tiger_init
#define tchash_tiger160_init    tchash_tiger_init
#define tchash_tiger128_init    tchash_tiger_init
void tchash_tiger_process(TCHash_Tiger* tiger, const void* data, size_t dlen);
#define tchash_tiger192_process     tchash_tiger_process
#define tchash_tiger160_process     tchash_tiger_process
#define tchash_tiger128_process     tchash_tiger_process
void* tchash_tiger_get(TCHash_Tiger* tiger, void* digest, size_t digestlen);
void* tchash_tiger192_get(TCHash_Tiger* tiger, void* digest);
void* tchash_tiger160_get(TCHash_Tiger* tiger, void* digest);
void* tchash_tiger128_get(TCHash_Tiger* tiger, void* digest);
void* tchash_tiger(void* digest, size_t digestlen, const void* data, size_t dlen);
void* tchash_tiger192(void* digest, const void* data, size_t dlen);
void* tchash_tiger160(void* digest, const void* data, size_t dlen);
void* tchash_tiger128(void* digest, const void* data, size_t dlen);

#define TCHASH_TIGER2_192_DIGEST_SIZE TCHASH_TIGER192_DIGEST_SIZE
#define TCHASH_TIGER2_160_DIGEST_SIZE TCHASH_TIGER160_DIGEST_SIZE
#define TCHASH_TIGER2_128_DIGEST_SIZE TCHASH_TIGER128_DIGEST_SIZE
typedef TCHash_Tiger TCHash_Tiger2;
#define tchash_tiger2_init      tchash_tiger_init
#define tchash_tiger2_192_init  tchash_tiger2_init
#define tchash_tiger2_160_init  tchash_tiger2_init
#define tchash_tiger2_128_init  tchash_tiger2_init
#define tchash_tiger2_process       tchash_tiger_process
#define tchash_tiger2_192_process   tchash_tiger2_process
#define tchash_tiger2_160_process   tchash_tiger2_process
#define tchash_tiger2_128_process   tchash_tiger2_process
void* tchash_tiger2_get(TCHash_Tiger* tiger, void* digest, size_t digestlen);
void* tchash_tiger2_192_get(TCHash_Tiger* tiger, void* digest);
void* tchash_tiger2_160_get(TCHash_Tiger* tiger, void* digest);
void* tchash_tiger2_128_get(TCHash_Tiger* tiger, void* digest);
void* tchash_tiger2(void* digest, size_t digestlen, const void* data, size_t dlen);
void* tchash_tiger2_192(void* digest, const void* data, size_t dlen);
void* tchash_tiger2_160(void* digest, const void* data, size_t dlen);
void* tchash_tiger2_128(void* digest, const void* data, size_t dlen);

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

#ifndef TC__ASSERT
#include <assert.h>
#define TC__ASSERT(x)   assert(x)
#endif /* TC__ASSERT */

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

#define TCHASH_I_GETPAD_BODY_(LHASH,UHASH,ENDIAN,SIZE,SETLENGTH,NUMM,CPYSIZE,PAD)\
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
    case 7: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], uptr[3], uptr[4], uptr[5], uptr[6], (PAD)); break;\
    case 6: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], uptr[3], uptr[4], uptr[5], (PAD), 0x00); break;\
    case 5: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], uptr[3], uptr[4], (PAD), 0x00, 0x00); break;\
    case 4: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], uptr[3], (PAD), 0x00, 0x00, 0x00); break;\
    case 3: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], uptr[2], (PAD), 0x00, 0x00, 0x00, 0x00); break;\
    case 2: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], uptr[1], (PAD), 0x00, 0x00, 0x00, 0x00, 0x00); break;\
    case 1: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_(uptr[0], (PAD), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); break;\
    case 0: M[i++] = TCHASH_I_U##SIZE##_FROM_BYTES_##ENDIAN##_((PAD), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); break;\
    }                                                                          \
    if(LHASH->blen >= sizeof(LHASH->buf.b) - sizeof(LHASH->total)) /* not enough space to put length at the end */\
    {                                                                          \
        memset(&M[i], 0, (sizeof(M) / sizeof(*M) - i) * sizeof(*M));           \
        i = 0;                                                                 \
        tchash_i_##LHASH##_process_block(h, M);                                \
    }                                                                          \
    memset(&M[i], 0, (sizeof(M) / sizeof(*M) - i) * sizeof(*M) - sizeof(LHASH->total));\
    SETLENGTH                                                                  \
    tchash_i_##LHASH##_process_block(h, M);                                    \
                                                                               \
    tchash_i_to_##ENDIAN##SIZE##arr(h, (NUMM));                                \
    memcpy(digest, h, (CPYSIZE));                                              \
    return digest;
#define TCHASH_I_GET_BODY_(LHASH,UHASH,ENDIAN,SIZE,SETLENGTH,NUMM,CPYSIZE)  TCHASH_I_GETPAD_BODY_(LHASH,UHASH,ENDIAN,SIZE,SETLENGTH,NUMM,CPYSIZE,0x80)

#define TCHASH_I_SIMPLE_BODY_(LHASH,UHASH)                                     \
    TCHash_##UHASH LHASH;                                                      \
    tchash_##LHASH##_init(&LHASH);                                             \
    tchash_##LHASH##_process(&LHASH, data, dlen);                              \
    return tchash_##LHASH##_get(&LHASH, digest);
#define TCHASH_I_SIMPLELEN_BODY_(LHASH,UHASH)                                  \
    TCHash_##UHASH LHASH;                                                      \
    tchash_##LHASH##_init(&LHASH);                                             \
    tchash_##LHASH##_process(&LHASH, data, dlen);                              \
    return tchash_##LHASH##_get(&LHASH, digest, digestlen);

int tchash_secure_eq(const void* a, const void* b, size_t len)
{
    const unsigned char* ua = TC__VOID_CAST(const unsigned char*,a);
    const unsigned char* ub = TC__VOID_CAST(const unsigned char*,b);

    unsigned char res = 0;
    while(len--)
        res |= *ua++ ^ *ub++;
    return !!res;
}
size_t tchash_xstring_from_bytes(char* str, const void* data, size_t dlen, int uppercase)
{
    static const char XVals[2][16] = { "0123456789abcdef", "0123456789ABCDEF" };
    uppercase = !!uppercase; /* convert to 0 or 1 */

    const unsigned char* udata = TC__VOID_CAST(const unsigned char*,data);
    size_t clen = dlen;
    while(clen--)
    {
        *str++ = XVals[uppercase][*udata >> 4];
        *str++ = XVals[uppercase][*udata & 15];
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
#define TCHASH_I_BASE64_DEF62   '+'
#define TCHASH_I_BASE64_DEF63   '/'
#define TCHASH_I_BASE64_DEFPAD  '='

static char tchash_i_to_base64char(unsigned char b, int c62, int c63)
{
    static const char BaseChars[62] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    return b == 63 ? c63 : b == 62 ? c62 : BaseChars[b];
}
static int tchash_i_from_base64char(unsigned char* padding, char b, int c62, int c63, int cpad)
{
    if(cpad && b == cpad)
    {
        ++*padding;
        return 0;
    }
    if(*padding && b != cpad)
        return -2; /* if we had padding, we need to still have it */
    return 'A' <= b && b <= 'Z' ? b - 'A'
         : 'a' <= b && b <= 'z' ? b - 'a' + 26
         : '0' <= b && b <= '9' ? b - '0' + 26 * 2
         : b == c62 ? 62
         : b == c63 ? 63
         : -1;
}
size_t tchash_base64_from_bytes(char* str, const void* data, size_t dlen, int c62, int c63, int cpad)
{
    if(c62 <= 0) c62 = TCHASH_I_BASE64_DEF62;
    if(c63 <= 0) c63 = TCHASH_I_BASE64_DEF63;
    if(cpad < 0) cpad = TCHASH_I_BASE64_DEFPAD;

    const unsigned char* udata = TC__VOID_CAST(const unsigned char*,data);
    char* ptr = str;

    size_t i;
    int j;
    for(i = 0; i < dlen; i += 3)
    {
        unsigned char d[3];
        if(i + 2 < dlen)
        {
            d[2] = udata[i+2];
            d[1] = udata[i+1];
        }
        else if(i + 1 < dlen)
        {
            d[2] = 0;
            d[1] = udata[i+1];
        }
        else
            d[2] = d[1] = 0;
        d[0] = udata[i+0];

        unsigned char b[4];
        b[0] = d[0] >> 2;
        b[1] = ((d[0] << 4) & 0x3F) | (d[1] >> 4);
        b[2] = ((d[1] << 2) & 0x3F) | (d[2] >> 6);
        b[3] = d[2] & 63;

        size_t rem = dlen - i;
        for(j = 0; j < 4 - (3 - TCHASH_I_MIN(rem,3)); j++)
            ptr[j] = tchash_i_to_base64char(b[j], c62, c63);
        ptr += j;
        if(cpad)
        {
            memset(ptr, cpad, 4 - j);
            ptr += 4 - j;
        }
    }

    *ptr = 0;
    return ptr - str;
}
size_t tchash_bytes_from_base64(void* data, const char* str, int slen, int c62, int c63, int cpad)
{
    if(c62 <= 0) c62 = TCHASH_I_BASE64_DEF62;
    if(c63 <= 0) c63 = TCHASH_I_BASE64_DEF63;
    if(cpad < 0) cpad = TCHASH_I_BASE64_DEFPAD;
    if(slen < 0) slen = strlen(str);

    unsigned char* udata = TC__VOID_CAST(unsigned char*,data);
    unsigned char* ptr = udata;

    int i;
    for(i = 0; i < slen; i += 4)
    {
        unsigned char padding = 0;

        int v;
        unsigned char b[4];
        if((v = tchash_i_from_base64char(&padding, str[i+0], c62, c63, cpad)) < 0) return 0;
        b[0] = v;
        if(i + 1 < slen)
        {
            if((v = tchash_i_from_base64char(&padding, str[i+1], c62, c63, cpad)) < 0) return 0;
            b[1] = v;
            if(i + 2 < slen)
            {
                if((v = tchash_i_from_base64char(&padding, str[i+2], c62, c63, cpad)) < 0) return 0;
                b[2] = v;
                if(i + 3 < slen)
                {
                    if((v = tchash_i_from_base64char(&padding, str[i+3], c62, c63, cpad)) < 0) return 0;
                    b[3] = v;
                }
                else
                {
                    if(!cpad) padding += 1;
                    b[3] = 0;
                }
            }
            else
            {
                if(!cpad) padding += 2;
                b[2] = b[3] = 0;
            }
        }
        else
        {
            if(!cpad) padding += 3;
            b[1] = b[2] = b[3] = 0;
        }

        //if(padding == 4) continue;
        if(padding == 3) return 0; /* it must be 1, 2, or 4 */
        if(padding <= 2) *ptr++ = (b[0] << 2) | (b[1] >> 4);
        if(padding <= 1) *ptr++ = (b[1] << 4) | (b[2] >> 2);
        if(padding <= 0) *ptr++ = (b[2] << 6) | b[3];
    }
    return ptr - udata;
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



TCHash_Tiger* tchash_tiger_init(TCHash_Tiger* tiger)
{
    // http://www.cs.technion.ac.il/~biham/Reports/Tiger/
    static const uint64_t InitH[] = { UINT64_C(0x0123456789ABCDEF), UINT64_C(0xFEDCBA9876543210), UINT64_C(0xF096A5B4C3B2E187) };
    if(!tiger) return NULL;

    tiger->total = 0;
    memcpy(tiger->h, InitH, sizeof(InitH));
    tiger->blen = 0;

    return tiger;
}
// http://www.cs.technion.ac.il/%7Ebiham/Reports/Tiger/tiger/node3.html
#define SAVE_ABC_() do { aa = a; bb = b; cc = c; } while(0)
#define ROUND_(a,b,c,x,mul) do {                                               \
        c ^= x;                                                                \
        a -= t1[(c >> (0*8))&0xFF] ^ t2[(c >> (2*8))&0xFF] ^ t3[(c >> (4*8))&0xFF] ^ t4[(c >> (6*8))&0xFF];\
        b += t4[(c >> (1*8))&0xFF] ^ t3[(c >> (3*8))&0xFF] ^ t2[(c >> (5*8))&0xFF] ^ t1[(c >> (7*8))&0xFF];\
        b *= mul;                                                              \
    } while(0)
#define PASS_(a,b,c,mul) do {                                                  \
        ROUND_(a,b,c,x0,mul);                                                  \
        ROUND_(b,c,a,x1,mul);                                                  \
        ROUND_(c,a,b,x2,mul);                                                  \
        ROUND_(a,b,c,x3,mul);                                                  \
        ROUND_(b,c,a,x4,mul);                                                  \
        ROUND_(c,a,b,x5,mul);                                                  \
        ROUND_(a,b,c,x6,mul);                                                  \
        ROUND_(b,c,a,x7,mul);                                                  \
    } while(0)
#define KEY_SCHEDULE_() do {                                                   \
        x0 -= x7 ^ UINT64_C(0xA5A5A5A5A5A5A5A5);                               \
        x1 ^= x0;                                                              \
        x2 += x1;                                                              \
        x3 -= x2 ^ ((~x1)<<19);                                                \
        x4 ^= x3;                                                              \
        x5 += x4;                                                              \
        x6 -= x5 ^ ((~x4)>>23);                                                \
        x7 ^= x6;                                                              \
        x0 += x7;                                                              \
        x1 -= x0 ^ ((~x7)<<19);                                                \
        x2 ^= x1;                                                              \
        x3 += x2;                                                              \
        x4 -= x3 ^ ((~x2)>>23);                                                \
        x5 ^= x4;                                                              \
        x6 += x5;                                                              \
        x7 -= x6 ^ UINT64_C(0x0123456789ABCDEF);                               \
    } while(0)
#define FEEDFORWARD_() do { a ^= aa; b -= bb; c += cc; } while(0)
static void tchash_i_tiger_process_block(uint64_t h[3], const uint64_t M[8])
{
    // yikes ... 8kB of data!
    static const uint64_t t1[] = {
        UINT64_C(0x02AAB17CF7E90C5E),UINT64_C(0xAC424B03E243A8EC),UINT64_C(0x72CD5BE30DD5FCD3),UINT64_C(0x6D019B93F6F97F3A),UINT64_C(0xCD9978FFD21F9193),UINT64_C(0x7573A1C9708029E2),UINT64_C(0xB164326B922A83C3),UINT64_C(0x46883EEE04915870),
        UINT64_C(0xEAACE3057103ECE6),UINT64_C(0xC54169B808A3535C),UINT64_C(0x4CE754918DDEC47C),UINT64_C(0x0AA2F4DFDC0DF40C),UINT64_C(0x10B76F18A74DBEFA),UINT64_C(0xC6CCB6235AD1AB6A),UINT64_C(0x13726121572FE2FF),UINT64_C(0x1A488C6F199D921E),
        UINT64_C(0x4BC9F9F4DA0007CA),UINT64_C(0x26F5E6F6E85241C7),UINT64_C(0x859079DBEA5947B6),UINT64_C(0x4F1885C5C99E8C92),UINT64_C(0xD78E761EA96F864B),UINT64_C(0x8E36428C52B5C17D),UINT64_C(0x69CF6827373063C1),UINT64_C(0xB607C93D9BB4C56E),
        UINT64_C(0x7D820E760E76B5EA),UINT64_C(0x645C9CC6F07FDC42),UINT64_C(0xBF38A078243342E0),UINT64_C(0x5F6B343C9D2E7D04),UINT64_C(0xF2C28AEB600B0EC6),UINT64_C(0x6C0ED85F7254BCAC),UINT64_C(0x71592281A4DB4FE5),UINT64_C(0x1967FA69CE0FED9F),
        UINT64_C(0xFD5293F8B96545DB),UINT64_C(0xC879E9D7F2A7600B),UINT64_C(0x860248920193194E),UINT64_C(0xA4F9533B2D9CC0B3),UINT64_C(0x9053836C15957613),UINT64_C(0xDB6DCF8AFC357BF1),UINT64_C(0x18BEEA7A7A370F57),UINT64_C(0x037117CA50B99066),
        UINT64_C(0x6AB30A9774424A35),UINT64_C(0xF4E92F02E325249B),UINT64_C(0x7739DB07061CCAE1),UINT64_C(0xD8F3B49CECA42A05),UINT64_C(0xBD56BE3F51382F73),UINT64_C(0x45FAED5843B0BB28),UINT64_C(0x1C813D5C11BF1F83),UINT64_C(0x8AF0E4B6D75FA169),
        UINT64_C(0x33EE18A487AD9999),UINT64_C(0x3C26E8EAB1C94410),UINT64_C(0xB510102BC0A822F9),UINT64_C(0x141EEF310CE6123B),UINT64_C(0xFC65B90059DDB154),UINT64_C(0xE0158640C5E0E607),UINT64_C(0x884E079826C3A3CF),UINT64_C(0x930D0D9523C535FD),
        UINT64_C(0x35638D754E9A2B00),UINT64_C(0x4085FCCF40469DD5),UINT64_C(0xC4B17AD28BE23A4C),UINT64_C(0xCAB2F0FC6A3E6A2E),UINT64_C(0x2860971A6B943FCD),UINT64_C(0x3DDE6EE212E30446),UINT64_C(0x6222F32AE01765AE),UINT64_C(0x5D550BB5478308FE),
        UINT64_C(0xA9EFA98DA0EDA22A),UINT64_C(0xC351A71686C40DA7),UINT64_C(0x1105586D9C867C84),UINT64_C(0xDCFFEE85FDA22853),UINT64_C(0xCCFBD0262C5EEF76),UINT64_C(0xBAF294CB8990D201),UINT64_C(0xE69464F52AFAD975),UINT64_C(0x94B013AFDF133E14),
        UINT64_C(0x06A7D1A32823C958),UINT64_C(0x6F95FE5130F61119),UINT64_C(0xD92AB34E462C06C0),UINT64_C(0xED7BDE33887C71D2),UINT64_C(0x79746D6E6518393E),UINT64_C(0x5BA419385D713329),UINT64_C(0x7C1BA6B948A97564),UINT64_C(0x31987C197BFDAC67),
        UINT64_C(0xDE6C23C44B053D02),UINT64_C(0x581C49FED002D64D),UINT64_C(0xDD474D6338261571),UINT64_C(0xAA4546C3E473D062),UINT64_C(0x928FCE349455F860),UINT64_C(0x48161BBACAAB94D9),UINT64_C(0x63912430770E6F68),UINT64_C(0x6EC8A5E602C6641C),
        UINT64_C(0x87282515337DDD2B),UINT64_C(0x2CDA6B42034B701B),UINT64_C(0xB03D37C181CB096D),UINT64_C(0xE108438266C71C6F),UINT64_C(0x2B3180C7EB51B255),UINT64_C(0xDF92B82F96C08BBC),UINT64_C(0x5C68C8C0A632F3BA),UINT64_C(0x5504CC861C3D0556),
        UINT64_C(0xABBFA4E55FB26B8F),UINT64_C(0x41848B0AB3BACEB4),UINT64_C(0xB334A273AA445D32),UINT64_C(0xBCA696F0A85AD881),UINT64_C(0x24F6EC65B528D56C),UINT64_C(0x0CE1512E90F4524A),UINT64_C(0x4E9DD79D5506D35A),UINT64_C(0x258905FAC6CE9779),
        UINT64_C(0x2019295B3E109B33),UINT64_C(0xF8A9478B73A054CC),UINT64_C(0x2924F2F934417EB0),UINT64_C(0x3993357D536D1BC4),UINT64_C(0x38A81AC21DB6FF8B),UINT64_C(0x47C4FBF17D6016BF),UINT64_C(0x1E0FAADD7667E3F5),UINT64_C(0x7ABCFF62938BEB96),
        UINT64_C(0xA78DAD948FC179C9),UINT64_C(0x8F1F98B72911E50D),UINT64_C(0x61E48EAE27121A91),UINT64_C(0x4D62F7AD31859808),UINT64_C(0xECEBA345EF5CEAEB),UINT64_C(0xF5CEB25EBC9684CE),UINT64_C(0xF633E20CB7F76221),UINT64_C(0xA32CDF06AB8293E4),
        UINT64_C(0x985A202CA5EE2CA4),UINT64_C(0xCF0B8447CC8A8FB1),UINT64_C(0x9F765244979859A3),UINT64_C(0xA8D516B1A1240017),UINT64_C(0x0BD7BA3EBB5DC726),UINT64_C(0xE54BCA55B86ADB39),UINT64_C(0x1D7A3AFD6C478063),UINT64_C(0x519EC608E7669EDD),
        UINT64_C(0x0E5715A2D149AA23),UINT64_C(0x177D4571848FF194),UINT64_C(0xEEB55F3241014C22),UINT64_C(0x0F5E5CA13A6E2EC2),UINT64_C(0x8029927B75F5C361),UINT64_C(0xAD139FABC3D6E436),UINT64_C(0x0D5DF1A94CCF402F),UINT64_C(0x3E8BD948BEA5DFC8),
        UINT64_C(0xA5A0D357BD3FF77E),UINT64_C(0xA2D12E251F74F645),UINT64_C(0x66FD9E525E81A082),UINT64_C(0x2E0C90CE7F687A49),UINT64_C(0xC2E8BCBEBA973BC5),UINT64_C(0x000001BCE509745F),UINT64_C(0x423777BBE6DAB3D6),UINT64_C(0xD1661C7EAEF06EB5),
        UINT64_C(0xA1781F354DAACFD8),UINT64_C(0x2D11284A2B16AFFC),UINT64_C(0xF1FC4F67FA891D1F),UINT64_C(0x73ECC25DCB920ADA),UINT64_C(0xAE610C22C2A12651),UINT64_C(0x96E0A810D356B78A),UINT64_C(0x5A9A381F2FE7870F),UINT64_C(0xD5AD62EDE94E5530),
        UINT64_C(0xD225E5E8368D1427),UINT64_C(0x65977B70C7AF4631),UINT64_C(0x99F889B2DE39D74F),UINT64_C(0x233F30BF54E1D143),UINT64_C(0x9A9675D3D9A63C97),UINT64_C(0x5470554FF334F9A8),UINT64_C(0x166ACB744A4F5688),UINT64_C(0x70C74CAAB2E4AEAD),
        UINT64_C(0xF0D091646F294D12),UINT64_C(0x57B82A89684031D1),UINT64_C(0xEFD95A5A61BE0B6B),UINT64_C(0x2FBD12E969F2F29A),UINT64_C(0x9BD37013FEFF9FE8),UINT64_C(0x3F9B0404D6085A06),UINT64_C(0x4940C1F3166CFE15),UINT64_C(0x09542C4DCDF3DEFB),
        UINT64_C(0xB4C5218385CD5CE3),UINT64_C(0xC935B7DC4462A641),UINT64_C(0x3417F8A68ED3B63F),UINT64_C(0xB80959295B215B40),UINT64_C(0xF99CDAEF3B8C8572),UINT64_C(0x018C0614F8FCB95D),UINT64_C(0x1B14ACCD1A3ACDF3),UINT64_C(0x84D471F200BB732D),
        UINT64_C(0xC1A3110E95E8DA16),UINT64_C(0x430A7220BF1A82B8),UINT64_C(0xB77E090D39DF210E),UINT64_C(0x5EF4BD9F3CD05E9D),UINT64_C(0x9D4FF6DA7E57A444),UINT64_C(0xDA1D60E183D4A5F8),UINT64_C(0xB287C38417998E47),UINT64_C(0xFE3EDC121BB31886),
        UINT64_C(0xC7FE3CCC980CCBEF),UINT64_C(0xE46FB590189BFD03),UINT64_C(0x3732FD469A4C57DC),UINT64_C(0x7EF700A07CF1AD65),UINT64_C(0x59C64468A31D8859),UINT64_C(0x762FB0B4D45B61F6),UINT64_C(0x155BAED099047718),UINT64_C(0x68755E4C3D50BAA6),
        UINT64_C(0xE9214E7F22D8B4DF),UINT64_C(0x2ADDBF532EAC95F4),UINT64_C(0x32AE3909B4BD0109),UINT64_C(0x834DF537B08E3450),UINT64_C(0xFA209DA84220728D),UINT64_C(0x9E691D9B9EFE23F7),UINT64_C(0x0446D288C4AE8D7F),UINT64_C(0x7B4CC524E169785B),
        UINT64_C(0x21D87F0135CA1385),UINT64_C(0xCEBB400F137B8AA5),UINT64_C(0x272E2B66580796BE),UINT64_C(0x3612264125C2B0DE),UINT64_C(0x057702BDAD1EFBB2),UINT64_C(0xD4BABB8EACF84BE9),UINT64_C(0x91583139641BC67B),UINT64_C(0x8BDC2DE08036E024),
        UINT64_C(0x603C8156F49F68ED),UINT64_C(0xF7D236F7DBEF5111),UINT64_C(0x9727C4598AD21E80),UINT64_C(0xA08A0896670A5FD7),UINT64_C(0xCB4A8F4309EBA9CB),UINT64_C(0x81AF564B0F7036A1),UINT64_C(0xC0B99AA778199ABD),UINT64_C(0x959F1EC83FC8E952),
        UINT64_C(0x8C505077794A81B9),UINT64_C(0x3ACAAF8F056338F0),UINT64_C(0x07B43F50627A6778),UINT64_C(0x4A44AB49F5ECCC77),UINT64_C(0x3BC3D6E4B679EE98),UINT64_C(0x9CC0D4D1CF14108C),UINT64_C(0x4406C00B206BC8A0),UINT64_C(0x82A18854C8D72D89),
        UINT64_C(0x67E366B35C3C432C),UINT64_C(0xB923DD61102B37F2),UINT64_C(0x56AB2779D884271D),UINT64_C(0xBE83E1B0FF1525AF),UINT64_C(0xFB7C65D4217E49A9),UINT64_C(0x6BDBE0E76D48E7D4),UINT64_C(0x08DF828745D9179E),UINT64_C(0x22EA6A9ADD53BD34),
        UINT64_C(0xE36E141C5622200A),UINT64_C(0x7F805D1B8CB750EE),UINT64_C(0xAFE5C7A59F58E837),UINT64_C(0xE27F996A4FB1C23C),UINT64_C(0xD3867DFB0775F0D0),UINT64_C(0xD0E673DE6E88891A),UINT64_C(0x123AEB9EAFB86C25),UINT64_C(0x30F1D5D5C145B895),
        UINT64_C(0xBB434A2DEE7269E7),UINT64_C(0x78CB67ECF931FA38),UINT64_C(0xF33B0372323BBF9C),UINT64_C(0x52D66336FB279C74),UINT64_C(0x505F33AC0AFB4EAA),UINT64_C(0xE8A5CD99A2CCE187),UINT64_C(0x534974801E2D30BB),UINT64_C(0x8D2D5711D5876D90),
        UINT64_C(0x1F1A412891BC038E),UINT64_C(0xD6E2E71D82E56648),UINT64_C(0x74036C3A497732B7),UINT64_C(0x89B67ED96361F5AB),UINT64_C(0xFFED95D8F1EA02A2),UINT64_C(0xE72B3BD61464D43D),UINT64_C(0xA6300F170BDC4820),UINT64_C(0xEBC18760ED78A77A),
    };
    static const uint64_t t2[] = {
        UINT64_C(0xE6A6BE5A05A12138),UINT64_C(0xB5A122A5B4F87C98),UINT64_C(0x563C6089140B6990),UINT64_C(0x4C46CB2E391F5DD5),UINT64_C(0xD932ADDBC9B79434),UINT64_C(0x08EA70E42015AFF5),UINT64_C(0xD765A6673E478CF1),UINT64_C(0xC4FB757EAB278D99),
        UINT64_C(0xDF11C6862D6E0692),UINT64_C(0xDDEB84F10D7F3B16),UINT64_C(0x6F2EF604A665EA04),UINT64_C(0x4A8E0F0FF0E0DFB3),UINT64_C(0xA5EDEEF83DBCBA51),UINT64_C(0xFC4F0A2A0EA4371E),UINT64_C(0xE83E1DA85CB38429),UINT64_C(0xDC8FF882BA1B1CE2),
        UINT64_C(0xCD45505E8353E80D),UINT64_C(0x18D19A00D4DB0717),UINT64_C(0x34A0CFEDA5F38101),UINT64_C(0x0BE77E518887CAF2),UINT64_C(0x1E341438B3C45136),UINT64_C(0xE05797F49089CCF9),UINT64_C(0xFFD23F9DF2591D14),UINT64_C(0x543DDA228595C5CD),
        UINT64_C(0x661F81FD99052A33),UINT64_C(0x8736E641DB0F7B76),UINT64_C(0x15227725418E5307),UINT64_C(0xE25F7F46162EB2FA),UINT64_C(0x48A8B2126C13D9FE),UINT64_C(0xAFDC541792E76EEA),UINT64_C(0x03D912BFC6D1898F),UINT64_C(0x31B1AAFA1B83F51B),
        UINT64_C(0xF1AC2796E42AB7D9),UINT64_C(0x40A3A7D7FCD2EBAC),UINT64_C(0x1056136D0AFBBCC5),UINT64_C(0x7889E1DD9A6D0C85),UINT64_C(0xD33525782A7974AA),UINT64_C(0xA7E25D09078AC09B),UINT64_C(0xBD4138B3EAC6EDD0),UINT64_C(0x920ABFBE71EB9E70),
        UINT64_C(0xA2A5D0F54FC2625C),UINT64_C(0xC054E36B0B1290A3),UINT64_C(0xF6DD59FF62FE932B),UINT64_C(0x3537354511A8AC7D),UINT64_C(0xCA845E9172FADCD4),UINT64_C(0x84F82B60329D20DC),UINT64_C(0x79C62CE1CD672F18),UINT64_C(0x8B09A2ADD124642C),
        UINT64_C(0xD0C1E96A19D9E726),UINT64_C(0x5A786A9B4BA9500C),UINT64_C(0x0E020336634C43F3),UINT64_C(0xC17B474AEB66D822),UINT64_C(0x6A731AE3EC9BAAC2),UINT64_C(0x8226667AE0840258),UINT64_C(0x67D4567691CAECA5),UINT64_C(0x1D94155C4875ADB5),
        UINT64_C(0x6D00FD985B813FDF),UINT64_C(0x51286EFCB774CD06),UINT64_C(0x5E8834471FA744AF),UINT64_C(0xF72CA0AEE761AE2E),UINT64_C(0xBE40E4CDAEE8E09A),UINT64_C(0xE9970BBB5118F665),UINT64_C(0x726E4BEB33DF1964),UINT64_C(0x703B000729199762),
        UINT64_C(0x4631D816F5EF30A7),UINT64_C(0xB880B5B51504A6BE),UINT64_C(0x641793C37ED84B6C),UINT64_C(0x7B21ED77F6E97D96),UINT64_C(0x776306312EF96B73),UINT64_C(0xAE528948E86FF3F4),UINT64_C(0x53DBD7F286A3F8F8),UINT64_C(0x16CADCE74CFC1063),
        UINT64_C(0x005C19BDFA52C6DD),UINT64_C(0x68868F5D64D46AD3),UINT64_C(0x3A9D512CCF1E186A),UINT64_C(0x367E62C2385660AE),UINT64_C(0xE359E7EA77DCB1D7),UINT64_C(0x526C0773749ABE6E),UINT64_C(0x735AE5F9D09F734B),UINT64_C(0x493FC7CC8A558BA8),
        UINT64_C(0xB0B9C1533041AB45),UINT64_C(0x321958BA470A59BD),UINT64_C(0x852DB00B5F46C393),UINT64_C(0x91209B2BD336B0E5),UINT64_C(0x6E604F7D659EF19F),UINT64_C(0xB99A8AE2782CCB24),UINT64_C(0xCCF52AB6C814C4C7),UINT64_C(0x4727D9AFBE11727B),
        UINT64_C(0x7E950D0C0121B34D),UINT64_C(0x756F435670AD471F),UINT64_C(0xF5ADD442615A6849),UINT64_C(0x4E87E09980B9957A),UINT64_C(0x2ACFA1DF50AEE355),UINT64_C(0xD898263AFD2FD556),UINT64_C(0xC8F4924DD80C8FD6),UINT64_C(0xCF99CA3D754A173A),
        UINT64_C(0xFE477BACAF91BF3C),UINT64_C(0xED5371F6D690C12D),UINT64_C(0x831A5C285E687094),UINT64_C(0xC5D3C90A3708A0A4),UINT64_C(0x0F7F903717D06580),UINT64_C(0x19F9BB13B8FDF27F),UINT64_C(0xB1BD6F1B4D502843),UINT64_C(0x1C761BA38FFF4012),
        UINT64_C(0x0D1530C4E2E21F3B),UINT64_C(0x8943CE69A7372C8A),UINT64_C(0xE5184E11FEB5CE66),UINT64_C(0x618BDB80BD736621),UINT64_C(0x7D29BAD68B574D0B),UINT64_C(0x81BB613E25E6FE5B),UINT64_C(0x071C9C10BC07913F),UINT64_C(0xC7BEEB7909AC2D97),
        UINT64_C(0xC3E58D353BC5D757),UINT64_C(0xEB017892F38F61E8),UINT64_C(0xD4EFFB9C9B1CC21A),UINT64_C(0x99727D26F494F7AB),UINT64_C(0xA3E063A2956B3E03),UINT64_C(0x9D4A8B9A4AA09C30),UINT64_C(0x3F6AB7D500090FB4),UINT64_C(0x9CC0F2A057268AC0),
        UINT64_C(0x3DEE9D2DEDBF42D1),UINT64_C(0x330F49C87960A972),UINT64_C(0xC6B2720287421B41),UINT64_C(0x0AC59EC07C00369C),UINT64_C(0xEF4EAC49CB353425),UINT64_C(0xF450244EEF0129D8),UINT64_C(0x8ACC46E5CAF4DEB6),UINT64_C(0x2FFEAB63989263F7),
        UINT64_C(0x8F7CB9FE5D7A4578),UINT64_C(0x5BD8F7644E634635),UINT64_C(0x427A7315BF2DC900),UINT64_C(0x17D0C4AA2125261C),UINT64_C(0x3992486C93518E50),UINT64_C(0xB4CBFEE0A2D7D4C3),UINT64_C(0x7C75D6202C5DDD8D),UINT64_C(0xDBC295D8E35B6C61),
        UINT64_C(0x60B369D302032B19),UINT64_C(0xCE42685FDCE44132),UINT64_C(0x06F3DDB9DDF65610),UINT64_C(0x8EA4D21DB5E148F0),UINT64_C(0x20B0FCE62FCD496F),UINT64_C(0x2C1B912358B0EE31),UINT64_C(0xB28317B818F5A308),UINT64_C(0xA89C1E189CA6D2CF),
        UINT64_C(0x0C6B18576AAADBC8),UINT64_C(0xB65DEAA91299FAE3),UINT64_C(0xFB2B794B7F1027E7),UINT64_C(0x04E4317F443B5BEB),UINT64_C(0x4B852D325939D0A6),UINT64_C(0xD5AE6BEEFB207FFC),UINT64_C(0x309682B281C7D374),UINT64_C(0xBAE309A194C3B475),
        UINT64_C(0x8CC3F97B13B49F05),UINT64_C(0x98A9422FF8293967),UINT64_C(0x244B16B01076FF7C),UINT64_C(0xF8BF571C663D67EE),UINT64_C(0x1F0D6758EEE30DA1),UINT64_C(0xC9B611D97ADEB9B7),UINT64_C(0xB7AFD5887B6C57A2),UINT64_C(0x6290AE846B984FE1),
        UINT64_C(0x94DF4CDEACC1A5FD),UINT64_C(0x058A5BD1C5483AFF),UINT64_C(0x63166CC142BA3C37),UINT64_C(0x8DB8526EB2F76F40),UINT64_C(0xE10880036F0D6D4E),UINT64_C(0x9E0523C9971D311D),UINT64_C(0x45EC2824CC7CD691),UINT64_C(0x575B8359E62382C9),
        UINT64_C(0xFA9E400DC4889995),UINT64_C(0xD1823ECB45721568),UINT64_C(0xDAFD983B8206082F),UINT64_C(0xAA7D29082386A8CB),UINT64_C(0x269FCD4403B87588),UINT64_C(0x1B91F5F728BDD1E0),UINT64_C(0xE4669F39040201F6),UINT64_C(0x7A1D7C218CF04ADE),
        UINT64_C(0x65623C29D79CE5CE),UINT64_C(0x2368449096C00BB1),UINT64_C(0xAB9BF1879DA503BA),UINT64_C(0xBC23ECB1A458058E),UINT64_C(0x9A58DF01BB401ECC),UINT64_C(0xA070E868A85F143D),UINT64_C(0x4FF188307DF2239E),UINT64_C(0x14D565B41A641183),
        UINT64_C(0xEE13337452701602),UINT64_C(0x950E3DCF3F285E09),UINT64_C(0x59930254B9C80953),UINT64_C(0x3BF299408930DA6D),UINT64_C(0xA955943F53691387),UINT64_C(0xA15EDECAA9CB8784),UINT64_C(0x29142127352BE9A0),UINT64_C(0x76F0371FFF4E7AFB),
        UINT64_C(0x0239F450274F2228),UINT64_C(0xBB073AF01D5E868B),UINT64_C(0xBFC80571C10E96C1),UINT64_C(0xD267088568222E23),UINT64_C(0x9671A3D48E80B5B0),UINT64_C(0x55B5D38AE193BB81),UINT64_C(0x693AE2D0A18B04B8),UINT64_C(0x5C48B4ECADD5335F),
        UINT64_C(0xFD743B194916A1CA),UINT64_C(0x2577018134BE98C4),UINT64_C(0xE77987E83C54A4AD),UINT64_C(0x28E11014DA33E1B9),UINT64_C(0x270CC59E226AA213),UINT64_C(0x71495F756D1A5F60),UINT64_C(0x9BE853FB60AFEF77),UINT64_C(0xADC786A7F7443DBF),
        UINT64_C(0x0904456173B29A82),UINT64_C(0x58BC7A66C232BD5E),UINT64_C(0xF306558C673AC8B2),UINT64_C(0x41F639C6B6C9772A),UINT64_C(0x216DEFE99FDA35DA),UINT64_C(0x11640CC71C7BE615),UINT64_C(0x93C43694565C5527),UINT64_C(0xEA038E6246777839),
        UINT64_C(0xF9ABF3CE5A3E2469),UINT64_C(0x741E768D0FD312D2),UINT64_C(0x0144B883CED652C6),UINT64_C(0xC20B5A5BA33F8552),UINT64_C(0x1AE69633C3435A9D),UINT64_C(0x97A28CA4088CFDEC),UINT64_C(0x8824A43C1E96F420),UINT64_C(0x37612FA66EEEA746),
        UINT64_C(0x6B4CB165F9CF0E5A),UINT64_C(0x43AA1C06A0ABFB4A),UINT64_C(0x7F4DC26FF162796B),UINT64_C(0x6CBACC8E54ED9B0F),UINT64_C(0xA6B7FFEFD2BB253E),UINT64_C(0x2E25BC95B0A29D4F),UINT64_C(0x86D6A58BDEF1388C),UINT64_C(0xDED74AC576B6F054),
        UINT64_C(0x8030BDBC2B45805D),UINT64_C(0x3C81AF70E94D9289),UINT64_C(0x3EFF6DDA9E3100DB),UINT64_C(0xB38DC39FDFCC8847),UINT64_C(0x123885528D17B87E),UINT64_C(0xF2DA0ED240B1B642),UINT64_C(0x44CEFADCD54BF9A9),UINT64_C(0x1312200E433C7EE6),
        UINT64_C(0x9FFCC84F3A78C748),UINT64_C(0xF0CD1F72248576BB),UINT64_C(0xEC6974053638CFE4),UINT64_C(0x2BA7B67C0CEC4E4C),UINT64_C(0xAC2F4DF3E5CE32ED),UINT64_C(0xCB33D14326EA4C11),UINT64_C(0xA4E9044CC77E58BC),UINT64_C(0x5F513293D934FCEF),
        UINT64_C(0x5DC9645506E55444),UINT64_C(0x50DE418F317DE40A),UINT64_C(0x388CB31A69DDE259),UINT64_C(0x2DB4A83455820A86),UINT64_C(0x9010A91E84711AE9),UINT64_C(0x4DF7F0B7B1498371),UINT64_C(0xD62A2EABC0977179),UINT64_C(0x22FAC097AA8D5C0E),
    };
    static const uint64_t t3[] = {
        UINT64_C(0xF49FCC2FF1DAF39B),UINT64_C(0x487FD5C66FF29281),UINT64_C(0xE8A30667FCDCA83F),UINT64_C(0x2C9B4BE3D2FCCE63),UINT64_C(0xDA3FF74B93FBBBC2),UINT64_C(0x2FA165D2FE70BA66),UINT64_C(0xA103E279970E93D4),UINT64_C(0xBECDEC77B0E45E71),
        UINT64_C(0xCFB41E723985E497),UINT64_C(0xB70AAA025EF75017),UINT64_C(0xD42309F03840B8E0),UINT64_C(0x8EFC1AD035898579),UINT64_C(0x96C6920BE2B2ABC5),UINT64_C(0x66AF4163375A9172),UINT64_C(0x2174ABDCCA7127FB),UINT64_C(0xB33CCEA64A72FF41),
        UINT64_C(0xF04A4933083066A5),UINT64_C(0x8D970ACDD7289AF5),UINT64_C(0x8F96E8E031C8C25E),UINT64_C(0xF3FEC02276875D47),UINT64_C(0xEC7BF310056190DD),UINT64_C(0xF5ADB0AEBB0F1491),UINT64_C(0x9B50F8850FD58892),UINT64_C(0x4975488358B74DE8),
        UINT64_C(0xA3354FF691531C61),UINT64_C(0x0702BBE481D2C6EE),UINT64_C(0x89FB24057DEDED98),UINT64_C(0xAC3075138596E902),UINT64_C(0x1D2D3580172772ED),UINT64_C(0xEB738FC28E6BC30D),UINT64_C(0x5854EF8F63044326),UINT64_C(0x9E5C52325ADD3BBE),
        UINT64_C(0x90AA53CF325C4623),UINT64_C(0xC1D24D51349DD067),UINT64_C(0x2051CFEEA69EA624),UINT64_C(0x13220F0A862E7E4F),UINT64_C(0xCE39399404E04864),UINT64_C(0xD9C42CA47086FCB7),UINT64_C(0x685AD2238A03E7CC),UINT64_C(0x066484B2AB2FF1DB),
        UINT64_C(0xFE9D5D70EFBF79EC),UINT64_C(0x5B13B9DD9C481854),UINT64_C(0x15F0D475ED1509AD),UINT64_C(0x0BEBCD060EC79851),UINT64_C(0xD58C6791183AB7F8),UINT64_C(0xD1187C5052F3EEE4),UINT64_C(0xC95D1192E54E82FF),UINT64_C(0x86EEA14CB9AC6CA2),
        UINT64_C(0x3485BEB153677D5D),UINT64_C(0xDD191D781F8C492A),UINT64_C(0xF60866BAA784EBF9),UINT64_C(0x518F643BA2D08C74),UINT64_C(0x8852E956E1087C22),UINT64_C(0xA768CB8DC410AE8D),UINT64_C(0x38047726BFEC8E1A),UINT64_C(0xA67738B4CD3B45AA),
        UINT64_C(0xAD16691CEC0DDE19),UINT64_C(0xC6D4319380462E07),UINT64_C(0xC5A5876D0BA61938),UINT64_C(0x16B9FA1FA58FD840),UINT64_C(0x188AB1173CA74F18),UINT64_C(0xABDA2F98C99C021F),UINT64_C(0x3E0580AB134AE816),UINT64_C(0x5F3B05B773645ABB),
        UINT64_C(0x2501A2BE5575F2F6),UINT64_C(0x1B2F74004E7E8BA9),UINT64_C(0x1CD7580371E8D953),UINT64_C(0x7F6ED89562764E30),UINT64_C(0xB15926FF596F003D),UINT64_C(0x9F65293DA8C5D6B9),UINT64_C(0x6ECEF04DD690F84C),UINT64_C(0x4782275FFF33AF88),
        UINT64_C(0xE41433083F820801),UINT64_C(0xFD0DFE409A1AF9B5),UINT64_C(0x4325A3342CDB396B),UINT64_C(0x8AE77E62B301B252),UINT64_C(0xC36F9E9F6655615A),UINT64_C(0x85455A2D92D32C09),UINT64_C(0xF2C7DEA949477485),UINT64_C(0x63CFB4C133A39EBA),
        UINT64_C(0x83B040CC6EBC5462),UINT64_C(0x3B9454C8FDB326B0),UINT64_C(0x56F56A9E87FFD78C),UINT64_C(0x2DC2940D99F42BC6),UINT64_C(0x98F7DF096B096E2D),UINT64_C(0x19A6E01E3AD852BF),UINT64_C(0x42A99CCBDBD4B40B),UINT64_C(0xA59998AF45E9C559),
        UINT64_C(0x366295E807D93186),UINT64_C(0x6B48181BFAA1F773),UINT64_C(0x1FEC57E2157A0A1D),UINT64_C(0x4667446AF6201AD5),UINT64_C(0xE615EBCACFB0F075),UINT64_C(0xB8F31F4F68290778),UINT64_C(0x22713ED6CE22D11E),UINT64_C(0x3057C1A72EC3C93B),
        UINT64_C(0xCB46ACC37C3F1F2F),UINT64_C(0xDBB893FD02AAF50E),UINT64_C(0x331FD92E600B9FCF),UINT64_C(0xA498F96148EA3AD6),UINT64_C(0xA8D8426E8B6A83EA),UINT64_C(0xA089B274B7735CDC),UINT64_C(0x87F6B3731E524A11),UINT64_C(0x118808E5CBC96749),
        UINT64_C(0x9906E4C7B19BD394),UINT64_C(0xAFED7F7E9B24A20C),UINT64_C(0x6509EADEEB3644A7),UINT64_C(0x6C1EF1D3E8EF0EDE),UINT64_C(0xB9C97D43E9798FB4),UINT64_C(0xA2F2D784740C28A3),UINT64_C(0x7B8496476197566F),UINT64_C(0x7A5BE3E6B65F069D),
        UINT64_C(0xF96330ED78BE6F10),UINT64_C(0xEEE60DE77A076A15),UINT64_C(0x2B4BEE4AA08B9BD0),UINT64_C(0x6A56A63EC7B8894E),UINT64_C(0x02121359BA34FEF4),UINT64_C(0x4CBF99F8283703FC),UINT64_C(0x398071350CAF30C8),UINT64_C(0xD0A77A89F017687A),
        UINT64_C(0xF1C1A9EB9E423569),UINT64_C(0x8C7976282DEE8199),UINT64_C(0x5D1737A5DD1F7ABD),UINT64_C(0x4F53433C09A9FA80),UINT64_C(0xFA8B0C53DF7CA1D9),UINT64_C(0x3FD9DCBC886CCB77),UINT64_C(0xC040917CA91B4720),UINT64_C(0x7DD00142F9D1DCDF),
        UINT64_C(0x8476FC1D4F387B58),UINT64_C(0x23F8E7C5F3316503),UINT64_C(0x032A2244E7E37339),UINT64_C(0x5C87A5D750F5A74B),UINT64_C(0x082B4CC43698992E),UINT64_C(0xDF917BECB858F63C),UINT64_C(0x3270B8FC5BF86DDA),UINT64_C(0x10AE72BB29B5DD76),
        UINT64_C(0x576AC94E7700362B),UINT64_C(0x1AD112DAC61EFB8F),UINT64_C(0x691BC30EC5FAA427),UINT64_C(0xFF246311CC327143),UINT64_C(0x3142368E30E53206),UINT64_C(0x71380E31E02CA396),UINT64_C(0x958D5C960AAD76F1),UINT64_C(0xF8D6F430C16DA536),
        UINT64_C(0xC8FFD13F1BE7E1D2),UINT64_C(0x7578AE66004DDBE1),UINT64_C(0x05833F01067BE646),UINT64_C(0xBB34B5AD3BFE586D),UINT64_C(0x095F34C9A12B97F0),UINT64_C(0x247AB64525D60CA8),UINT64_C(0xDCDBC6F3017477D1),UINT64_C(0x4A2E14D4DECAD24D),
        UINT64_C(0xBDB5E6D9BE0A1EEB),UINT64_C(0x2A7E70F7794301AB),UINT64_C(0xDEF42D8A270540FD),UINT64_C(0x01078EC0A34C22C1),UINT64_C(0xE5DE511AF4C16387),UINT64_C(0x7EBB3A52BD9A330A),UINT64_C(0x77697857AA7D6435),UINT64_C(0x004E831603AE4C32),
        UINT64_C(0xE7A21020AD78E312),UINT64_C(0x9D41A70C6AB420F2),UINT64_C(0x28E06C18EA1141E6),UINT64_C(0xD2B28CBD984F6B28),UINT64_C(0x26B75F6C446E9D83),UINT64_C(0xBA47568C4D418D7F),UINT64_C(0xD80BADBFE6183D8E),UINT64_C(0x0E206D7F5F166044),
        UINT64_C(0xE258A43911CBCA3E),UINT64_C(0x723A1746B21DC0BC),UINT64_C(0xC7CAA854F5D7CDD3),UINT64_C(0x7CAC32883D261D9C),UINT64_C(0x7690C26423BA942C),UINT64_C(0x17E55524478042B8),UINT64_C(0xE0BE477656A2389F),UINT64_C(0x4D289B5E67AB2DA0),
        UINT64_C(0x44862B9C8FBBFD31),UINT64_C(0xB47CC8049D141365),UINT64_C(0x822C1B362B91C793),UINT64_C(0x4EB14655FB13DFD8),UINT64_C(0x1ECBBA0714E2A97B),UINT64_C(0x6143459D5CDE5F14),UINT64_C(0x53A8FBF1D5F0AC89),UINT64_C(0x97EA04D81C5E5B00),
        UINT64_C(0x622181A8D4FDB3F3),UINT64_C(0xE9BCD341572A1208),UINT64_C(0x1411258643CCE58A),UINT64_C(0x9144C5FEA4C6E0A4),UINT64_C(0x0D33D06565CF620F),UINT64_C(0x54A48D489F219CA1),UINT64_C(0xC43E5EAC6D63C821),UINT64_C(0xA9728B3A72770DAF),
        UINT64_C(0xD7934E7B20DF87EF),UINT64_C(0xE35503B61A3E86E5),UINT64_C(0xCAE321FBC819D504),UINT64_C(0x129A50B3AC60BFA6),UINT64_C(0xCD5E68EA7E9FB6C3),UINT64_C(0xB01C90199483B1C7),UINT64_C(0x3DE93CD5C295376C),UINT64_C(0xAED52EDF2AB9AD13),
        UINT64_C(0x2E60F512C0A07884),UINT64_C(0xBC3D86A3E36210C9),UINT64_C(0x35269D9B163951CE),UINT64_C(0x0C7D6E2AD0CDB5FA),UINT64_C(0x59E86297D87F5733),UINT64_C(0x298EF221898DB0E7),UINT64_C(0x55000029D1A5AA7E),UINT64_C(0x8BC08AE1B5061B45),
        UINT64_C(0xC2C31C2B6C92703A),UINT64_C(0x94CC596BAF25EF42),UINT64_C(0x0A1D73DB22540456),UINT64_C(0x04B6A0F9D9C4179A),UINT64_C(0xEFFDAFA2AE3D3C60),UINT64_C(0xF7C8075BB49496C4),UINT64_C(0x9CC5C7141D1CD4E3),UINT64_C(0x78BD1638218E5534),
        UINT64_C(0xB2F11568F850246A),UINT64_C(0xEDFABCFA9502BC29),UINT64_C(0x796CE5F2DA23051B),UINT64_C(0xAAE128B0DC93537C),UINT64_C(0x3A493DA0EE4B29AE),UINT64_C(0xB5DF6B2C416895D7),UINT64_C(0xFCABBD25122D7F37),UINT64_C(0x70810B58105DC4B1),
        UINT64_C(0xE10FDD37F7882A90),UINT64_C(0x524DCAB5518A3F5C),UINT64_C(0x3C9E85878451255B),UINT64_C(0x4029828119BD34E2),UINT64_C(0x74A05B6F5D3CECCB),UINT64_C(0xB610021542E13ECA),UINT64_C(0x0FF979D12F59E2AC),UINT64_C(0x6037DA27E4F9CC50),
        UINT64_C(0x5E92975A0DF1847D),UINT64_C(0xD66DE190D3E623FE),UINT64_C(0x5032D6B87B568048),UINT64_C(0x9A36B7CE8235216E),UINT64_C(0x80272A7A24F64B4A),UINT64_C(0x93EFED8B8C6916F7),UINT64_C(0x37DDBFF44CCE1555),UINT64_C(0x4B95DB5D4B99BD25),
        UINT64_C(0x92D3FDA169812FC0),UINT64_C(0xFB1A4A9A90660BB6),UINT64_C(0x730C196946A4B9B2),UINT64_C(0x81E289AA7F49DA68),UINT64_C(0x64669A0F83B1A05F),UINT64_C(0x27B3FF7D9644F48B),UINT64_C(0xCC6B615C8DB675B3),UINT64_C(0x674F20B9BCEBBE95),
        UINT64_C(0x6F31238275655982),UINT64_C(0x5AE488713E45CF05),UINT64_C(0xBF619F9954C21157),UINT64_C(0xEABAC46040A8EAE9),UINT64_C(0x454C6FE9F2C0C1CD),UINT64_C(0x419CF6496412691C),UINT64_C(0xD3DC3BEF265B0F70),UINT64_C(0x6D0E60F5C3578A9E),
    };
    static const uint64_t t4[] = {
        UINT64_C(0x5B0E608526323C55),UINT64_C(0x1A46C1A9FA1B59F5),UINT64_C(0xA9E245A17C4C8FFA),UINT64_C(0x65CA5159DB2955D7),UINT64_C(0x05DB0A76CE35AFC2),UINT64_C(0x81EAC77EA9113D45),UINT64_C(0x528EF88AB6AC0A0D),UINT64_C(0xA09EA253597BE3FF),
        UINT64_C(0x430DDFB3AC48CD56),UINT64_C(0xC4B3A67AF45CE46F),UINT64_C(0x4ECECFD8FBE2D05E),UINT64_C(0x3EF56F10B39935F0),UINT64_C(0x0B22D6829CD619C6),UINT64_C(0x17FD460A74DF2069),UINT64_C(0x6CF8CC8E8510ED40),UINT64_C(0xD6C824BF3A6ECAA7),
        UINT64_C(0x61243D581A817049),UINT64_C(0x048BACB6BBC163A2),UINT64_C(0xD9A38AC27D44CC32),UINT64_C(0x7FDDFF5BAAF410AB),UINT64_C(0xAD6D495AA804824B),UINT64_C(0xE1A6A74F2D8C9F94),UINT64_C(0xD4F7851235DEE8E3),UINT64_C(0xFD4B7F886540D893),
        UINT64_C(0x247C20042AA4BFDA),UINT64_C(0x096EA1C517D1327C),UINT64_C(0xD56966B4361A6685),UINT64_C(0x277DA5C31221057D),UINT64_C(0x94D59893A43ACFF7),UINT64_C(0x64F0C51CCDC02281),UINT64_C(0x3D33BCC4FF6189DB),UINT64_C(0xE005CB184CE66AF1),
        UINT64_C(0xFF5CCD1D1DB99BEA),UINT64_C(0xB0B854A7FE42980F),UINT64_C(0x7BD46A6A718D4B9F),UINT64_C(0xD10FA8CC22A5FD8C),UINT64_C(0xD31484952BE4BD31),UINT64_C(0xC7FA975FCB243847),UINT64_C(0x4886ED1E5846C407),UINT64_C(0x28CDDB791EB70B04),
        UINT64_C(0xC2B00BE2F573417F),UINT64_C(0x5C9590452180F877),UINT64_C(0x7A6BDDFFF370EB00),UINT64_C(0xCE509E38D6D9D6A4),UINT64_C(0xEBEB0F00647FA702),UINT64_C(0x1DCC06CF76606F06),UINT64_C(0xE4D9F28BA286FF0A),UINT64_C(0xD85A305DC918C262),
        UINT64_C(0x475B1D8732225F54),UINT64_C(0x2D4FB51668CCB5FE),UINT64_C(0xA679B9D9D72BBA20),UINT64_C(0x53841C0D912D43A5),UINT64_C(0x3B7EAA48BF12A4E8),UINT64_C(0x781E0E47F22F1DDF),UINT64_C(0xEFF20CE60AB50973),UINT64_C(0x20D261D19DFFB742),
        UINT64_C(0x16A12B03062A2E39),UINT64_C(0x1960EB2239650495),UINT64_C(0x251C16FED50EB8B8),UINT64_C(0x9AC0C330F826016E),UINT64_C(0xED152665953E7671),UINT64_C(0x02D63194A6369570),UINT64_C(0x5074F08394B1C987),UINT64_C(0x70BA598C90B25CE1),
        UINT64_C(0x794A15810B9742F6),UINT64_C(0x0D5925E9FCAF8C6C),UINT64_C(0x3067716CD868744E),UINT64_C(0x910AB077E8D7731B),UINT64_C(0x6A61BBDB5AC42F61),UINT64_C(0x93513EFBF0851567),UINT64_C(0xF494724B9E83E9D5),UINT64_C(0xE887E1985C09648D),
        UINT64_C(0x34B1D3C675370CFD),UINT64_C(0xDC35E433BC0D255D),UINT64_C(0xD0AAB84234131BE0),UINT64_C(0x08042A50B48B7EAF),UINT64_C(0x9997C4EE44A3AB35),UINT64_C(0x829A7B49201799D0),UINT64_C(0x263B8307B7C54441),UINT64_C(0x752F95F4FD6A6CA6),
        UINT64_C(0x927217402C08C6E5),UINT64_C(0x2A8AB754A795D9EE),UINT64_C(0xA442F7552F72943D),UINT64_C(0x2C31334E19781208),UINT64_C(0x4FA98D7CEAEE6291),UINT64_C(0x55C3862F665DB309),UINT64_C(0xBD0610175D53B1F3),UINT64_C(0x46FE6CB840413F27),
        UINT64_C(0x3FE03792DF0CFA59),UINT64_C(0xCFE700372EB85E8F),UINT64_C(0xA7BE29E7ADBCE118),UINT64_C(0xE544EE5CDE8431DD),UINT64_C(0x8A781B1B41F1873E),UINT64_C(0xA5C94C78A0D2F0E7),UINT64_C(0x39412E2877B60728),UINT64_C(0xA1265EF3AFC9A62C),
        UINT64_C(0xBCC2770C6A2506C5),UINT64_C(0x3AB66DD5DCE1CE12),UINT64_C(0xE65499D04A675B37),UINT64_C(0x7D8F523481BFD216),UINT64_C(0x0F6F64FCEC15F389),UINT64_C(0x74EFBE618B5B13C8),UINT64_C(0xACDC82B714273E1D),UINT64_C(0xDD40BFE003199D17),
        UINT64_C(0x37E99257E7E061F8),UINT64_C(0xFA52626904775AAA),UINT64_C(0x8BBBF63A463D56F9),UINT64_C(0xF0013F1543A26E64),UINT64_C(0xA8307E9F879EC898),UINT64_C(0xCC4C27A4150177CC),UINT64_C(0x1B432F2CCA1D3348),UINT64_C(0xDE1D1F8F9F6FA013),
        UINT64_C(0x606602A047A7DDD6),UINT64_C(0xD237AB64CC1CB2C7),UINT64_C(0x9B938E7225FCD1D3),UINT64_C(0xEC4E03708E0FF476),UINT64_C(0xFEB2FBDA3D03C12D),UINT64_C(0xAE0BCED2EE43889A),UINT64_C(0x22CB8923EBFB4F43),UINT64_C(0x69360D013CF7396D),
        UINT64_C(0x855E3602D2D4E022),UINT64_C(0x073805BAD01F784C),UINT64_C(0x33E17A133852F546),UINT64_C(0xDF4874058AC7B638),UINT64_C(0xBA92B29C678AA14A),UINT64_C(0x0CE89FC76CFAADCD),UINT64_C(0x5F9D4E0908339E34),UINT64_C(0xF1AFE9291F5923B9),
        UINT64_C(0x6E3480F60F4A265F),UINT64_C(0xEEBF3A2AB29B841C),UINT64_C(0xE21938A88F91B4AD),UINT64_C(0x57DFEFF845C6D3C3),UINT64_C(0x2F006B0BF62CAAF2),UINT64_C(0x62F479EF6F75EE78),UINT64_C(0x11A55AD41C8916A9),UINT64_C(0xF229D29084FED453),
        UINT64_C(0x42F1C27B16B000E6),UINT64_C(0x2B1F76749823C074),UINT64_C(0x4B76ECA3C2745360),UINT64_C(0x8C98F463B91691BD),UINT64_C(0x14BCC93CF1ADE66A),UINT64_C(0x8885213E6D458397),UINT64_C(0x8E177DF0274D4711),UINT64_C(0xB49B73B5503F2951),
        UINT64_C(0x10168168C3F96B6B),UINT64_C(0x0E3D963B63CAB0AE),UINT64_C(0x8DFC4B5655A1DB14),UINT64_C(0xF789F1356E14DE5C),UINT64_C(0x683E68AF4E51DAC1),UINT64_C(0xC9A84F9D8D4B0FD9),UINT64_C(0x3691E03F52A0F9D1),UINT64_C(0x5ED86E46E1878E80),
        UINT64_C(0x3C711A0E99D07150),UINT64_C(0x5A0865B20C4E9310),UINT64_C(0x56FBFC1FE4F0682E),UINT64_C(0xEA8D5DE3105EDF9B),UINT64_C(0x71ABFDB12379187A),UINT64_C(0x2EB99DE1BEE77B9C),UINT64_C(0x21ECC0EA33CF4523),UINT64_C(0x59A4D7521805C7A1),
        UINT64_C(0x3896F5EB56AE7C72),UINT64_C(0xAA638F3DB18F75DC),UINT64_C(0x9F39358DABE9808E),UINT64_C(0xB7DEFA91C00B72AC),UINT64_C(0x6B5541FD62492D92),UINT64_C(0x6DC6DEE8F92E4D5B),UINT64_C(0x353F57ABC4BEEA7E),UINT64_C(0x735769D6DA5690CE),
        UINT64_C(0x0A234AA642391484),UINT64_C(0xF6F9508028F80D9D),UINT64_C(0xB8E319A27AB3F215),UINT64_C(0x31AD9C1151341A4D),UINT64_C(0x773C22A57BEF5805),UINT64_C(0x45C7561A07968633),UINT64_C(0xF913DA9E249DBE36),UINT64_C(0xDA652D9B78A64C68),
        UINT64_C(0x4C27A97F3BC334EF),UINT64_C(0x76621220E66B17F4),UINT64_C(0x967743899ACD7D0B),UINT64_C(0xF3EE5BCAE0ED6782),UINT64_C(0x409F753600C879FC),UINT64_C(0x06D09A39B5926DB6),UINT64_C(0x6F83AEB0317AC588),UINT64_C(0x01E6CA4A86381F21),
        UINT64_C(0x66FF3462D19F3025),UINT64_C(0x72207C24DDFD3BFB),UINT64_C(0x4AF6B6D3E2ECE2EB),UINT64_C(0x9C994DBEC7EA08DE),UINT64_C(0x49ACE597B09A8BC4),UINT64_C(0xB38C4766CF0797BA),UINT64_C(0x131B9373C57C2A75),UINT64_C(0xB1822CCE61931E58),
        UINT64_C(0x9D7555B909BA1C0C),UINT64_C(0x127FAFDD937D11D2),UINT64_C(0x29DA3BADC66D92E4),UINT64_C(0xA2C1D57154C2ECBC),UINT64_C(0x58C5134D82F6FE24),UINT64_C(0x1C3AE3515B62274F),UINT64_C(0xE907C82E01CB8126),UINT64_C(0xF8ED091913E37FCB),
        UINT64_C(0x3249D8F9C80046C9),UINT64_C(0x80CF9BEDE388FB63),UINT64_C(0x1881539A116CF19E),UINT64_C(0x5103F3F76BD52457),UINT64_C(0x15B7E6F5AE47F7A8),UINT64_C(0xDBD7C6DED47E9CCF),UINT64_C(0x44E55C410228BB1A),UINT64_C(0xB647D4255EDB4E99),
        UINT64_C(0x5D11882BB8AAFC30),UINT64_C(0xF5098BBB29D3212A),UINT64_C(0x8FB5EA14E90296B3),UINT64_C(0x677B942157DD025A),UINT64_C(0xFB58E7C0A390ACB5),UINT64_C(0x89D3674C83BD4A01),UINT64_C(0x9E2DA4DF4BF3B93B),UINT64_C(0xFCC41E328CAB4829),
        UINT64_C(0x03F38C96BA582C52),UINT64_C(0xCAD1BDBD7FD85DB2),UINT64_C(0xBBB442C16082AE83),UINT64_C(0xB95FE86BA5DA9AB0),UINT64_C(0xB22E04673771A93F),UINT64_C(0x845358C9493152D8),UINT64_C(0xBE2A488697B4541E),UINT64_C(0x95A2DC2DD38E6966),
        UINT64_C(0xC02C11AC923C852B),UINT64_C(0x2388B1990DF2A87B),UINT64_C(0x7C8008FA1B4F37BE),UINT64_C(0x1F70D0C84D54E503),UINT64_C(0x5490ADEC7ECE57D4),UINT64_C(0x002B3C27D9063A3A),UINT64_C(0x7EAEA3848030A2BF),UINT64_C(0xC602326DED2003C0),
        UINT64_C(0x83A7287D69A94086),UINT64_C(0xC57A5FCB30F57A8A),UINT64_C(0xB56844E479EBE779),UINT64_C(0xA373B40F05DCBCE9),UINT64_C(0xD71A786E88570EE2),UINT64_C(0x879CBACDBDE8F6A0),UINT64_C(0x976AD1BCC164A32F),UINT64_C(0xAB21E25E9666D78B),
        UINT64_C(0x901063AAE5E5C33C),UINT64_C(0x9818B34448698D90),UINT64_C(0xE36487AE3E1E8ABB),UINT64_C(0xAFBDF931893BDCB4),UINT64_C(0x6345A0DC5FBBD519),UINT64_C(0x8628FE269B9465CA),UINT64_C(0x1E5D01603F9C51EC),UINT64_C(0x4DE44006A15049B7),
        UINT64_C(0xBF6C70E5F776CBB1),UINT64_C(0x411218F2EF552BED),UINT64_C(0xCB0C0708705A36A3),UINT64_C(0xE74D14754F986044),UINT64_C(0xCD56D9430EA8280E),UINT64_C(0xC12591D7535F5065),UINT64_C(0xC83223F1720AEF96),UINT64_C(0xC3A0396F7363A51F),
    };

    uint64_t a = h[0], b = h[1], c = h[2];
    uint64_t aa, bb, cc;

    uint64_t x0 = M[0], x1 = M[1], x2 = M[2], x3 = M[3], x4 = M[4], x5 = M[5], x6 = M[6], x7 = M[7];

    SAVE_ABC_();
    PASS_(a,b,c,5);
    KEY_SCHEDULE_();
    PASS_(c,a,b,7);
    KEY_SCHEDULE_();
    PASS_(b,c,a,9);
    FEEDFORWARD_();

    h[0] = a; h[1] = b; h[2] = c;
}
#undef SAVE_ABC_
#undef ROUND_
#undef PASS_
#undef FEEDFORWARD_
void tchash_tiger_process(TCHash_Tiger* tiger, const void* data, size_t dlen)
{
    TCHASH_I_PROCESS_BODY_(tiger,Tiger,le,64,{tiger->total += dlen;});
}
void* tchash_tiger_get(TCHash_Tiger* tiger, void* digest, size_t digestlen)
{
    TCHASH_I_GETPAD_BODY_(tiger,Tiger,le,64,{M[7] = tiger->total << 3;},(digestlen + (sizeof(h) - 1))/sizeof(*h),digestlen,0x01);
}
void* tchash_tiger192_get(TCHash_Tiger* tiger, void* digest) { return tchash_tiger_get(tiger, digest, TCHASH_TIGER192_DIGEST_SIZE); }
void* tchash_tiger160_get(TCHash_Tiger* tiger, void* digest) { return tchash_tiger_get(tiger, digest, TCHASH_TIGER160_DIGEST_SIZE); }
void* tchash_tiger128_get(TCHash_Tiger* tiger, void* digest) { return tchash_tiger_get(tiger, digest, TCHASH_TIGER128_DIGEST_SIZE); }
void* tchash_tiger(void* digest, size_t digestlen, const void* data, size_t dlen) { TC__ASSERT(digestlen <= TCHASH_TIGER192_DIGEST_SIZE); TCHASH_I_SIMPLELEN_BODY_(tiger,Tiger) }
void* tchash_tiger192(void* digest, const void* data, size_t dlen) { return tchash_tiger(digest, TCHASH_TIGER192_DIGEST_SIZE, data, dlen); }
void* tchash_tiger160(void* digest, const void* data, size_t dlen) { return tchash_tiger(digest, TCHASH_TIGER160_DIGEST_SIZE, data, dlen); }
void* tchash_tiger128(void* digest, const void* data, size_t dlen) { return tchash_tiger(digest, TCHASH_TIGER128_DIGEST_SIZE, data, dlen); }

void* tchash_tiger2_get(TCHash_Tiger* tiger, void* digest, size_t digestlen)
{
    TCHASH_I_GETPAD_BODY_(tiger,Tiger,le,64,{M[7] = tiger->total << 3;},(digestlen + (sizeof(h) - 1))/sizeof(*h),digestlen,0x80);
}
void* tchash_tiger2_192_get(TCHash_Tiger* tiger, void* digest) { return tchash_tiger2_get(tiger, digest, TCHASH_TIGER2_192_DIGEST_SIZE); }
void* tchash_tiger2_160_get(TCHash_Tiger* tiger, void* digest) { return tchash_tiger2_get(tiger, digest, TCHASH_TIGER2_160_DIGEST_SIZE); }
void* tchash_tiger2_128_get(TCHash_Tiger* tiger, void* digest) { return tchash_tiger2_get(tiger, digest, TCHASH_TIGER2_128_DIGEST_SIZE); }
void* tchash_tiger2(void* digest, size_t digestlen, const void* data, size_t dlen) { TC__ASSERT(digestlen <= TCHASH_TIGER192_DIGEST_SIZE); TCHASH_I_SIMPLELEN_BODY_(tiger2,Tiger2) }
void* tchash_tiger2_192(void* digest, const void* data, size_t dlen) { return tchash_tiger2(digest, TCHASH_TIGER2_192_DIGEST_SIZE, data, dlen); }
void* tchash_tiger2_160(void* digest, const void* data, size_t dlen) { return tchash_tiger2(digest, TCHASH_TIGER2_160_DIGEST_SIZE, data, dlen); }
void* tchash_tiger2_128(void* digest, const void* data, size_t dlen) { return tchash_tiger2(digest, TCHASH_TIGER2_128_DIGEST_SIZE, data, dlen); }

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
static void tchash_i_keccak1600_process_block(uint64_t h[25], const uint64_t* M, size_t bsize, int nrounds)
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
void* tchash_shake128(void* digest, size_t digestlen, const void* data, size_t dlen) { TCHASH_I_SIMPLELEN_BODY_(shake128,SHAKE128) }

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
void* tchash_shake256(void* digest, size_t digestlen, const void* data, size_t dlen) { TCHASH_I_SIMPLELEN_BODY_(shake256,SHAKE256) }

#endif /* TC_HASH_IMPLEMENTATION */
