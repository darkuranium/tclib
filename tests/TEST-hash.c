#define TC_HASH_IMPLEMENTATION
#include "../tc_hash.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>

#include <assert.h>

#include "test.h"

void TEST_h_md5(const char* expected, const void* data, size_t dlen)
{
    char bytes[TCHASH_MD5_DIGEST_SIZE];
    char hexstr[sizeof(bytes)+1];

    TCHash_MD5 md5;
    tchash_md5_init(&md5);
    tchash_md5_process(&md5, data, dlen);
    tchash_md5_get(&md5, bytes);

    tchash_xstring_from_bytes(hexstr, bytes, sizeof(bytes));
    //printf("%s == %s\n", hexstr, expected);
    assert(!strcmp(hexstr, expected));
}

void TEST_md5(void)
{
    static const char* TestVectors[] = {
        "", "d41d8cd98f00b204e9800998ecf8427e",
        "The quick brown fox jumps over the lazy dog", "9e107d9d372bb6826bd81d3542a419d6",
        "The quick brown fox jumps over the lazy dog.", "e4d909c290d0fb1ca068ffaddf22cbd0",
    };

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
        TEST_h_md5(TestVectors[i+1], TestVectors[i+0], strlen(TestVectors[i+0]));
}

static void* read_all(uint64_t* len, const char* fname)
{
    char* data = NULL;
    FILE* file = fopen(fname, "rb");
    if(!file) goto error;

    if(fseek(file, 0, SEEK_END)) goto error;
    long int flen = ftell(file);
    if(flen < 0) goto error;
    if(fseek(file, 0, SEEK_SET)) goto error;

    data = malloc(flen + 1);
    if(fread(data, 1, flen, file) != flen) goto error;
    fclose(file);
    data[flen] = 0;
    *len = flen;
    return data;
error:
    if(file) fclose(file);
    if(data) free(data);
    return NULL;
}
static char* get_line(char** ptr)
{
    if(!**ptr) return NULL;

    char* ret = *ptr;
    char* end = *ptr + strcspn(*ptr, "\r\n");
    if(*end == 0)
        *ptr = end;
    else
    {
        *end = 0;
        *ptr = end + 1;
    }
    return ret;
}
static char* skip_leading_space(char* ptr)
{
    return ptr + strspn(ptr, " \t\v\f");
}
static char* trim_trailing_space(char* ptr, char* end)
{
    if(!end) end = ptr + strlen(ptr);
    while(ptr < end && strchr(" \t\v\f", end[-1])) end--;
    *end = 0;
    return ptr;
}
char* get_field(char** ptr, const char* rkey)
{
    for(;;)
    {
        if(!**ptr) return NULL;

        char* line = skip_leading_space(get_line(ptr));
        if(!line[0] || line[0] == '#' || line[0] == '[') continue;

        char* eq = strchr(line, '=');
        if(!eq) return NULL; /* ERROR */

        *eq = 0;
        char* key = trim_trailing_space(line, eq);
        char* val = trim_trailing_space(skip_leading_space(eq + 1), NULL);

        if(!strcmp(key, rkey)) return val;
    }
    return "---";
}

#define TESTDATA_ROOT "../tests/NIST/shabytetestvectors"

TEST(StringConv,(
    static const unsigned char bytes[] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};

    char sbuf[sizeof(bytes)*2];
    unsigned char bbuf[sizeof(bytes)];

    tchash_xstring_from_bytes(sbuf, bytes, sizeof(bytes));
    tchash_bytes_from_xstring(bbuf, sbuf, sizeof(bytes)*2);

    ASSERT_MEMEQ(bytes,sizeof(bytes),bbuf,sizeof(bbuf));
))

#define HELPER_MSG(LHASH,UHASH,FNAME) (                                        \
    size_t dlen;                                                               \
    char* text = read_all(&dlen, TESTDATA_ROOT "/" FNAME ".rsp");              \
    ASSERT_NOTNULL(text);                                                      \
    char* ptr = text;                                                          \
                                                                               \
    char result[TCHASH_##UHASH##_DIGEST_SIZE];                                 \
    char tresult[TCHASH_##UHASH##_DIGEST_SIZE*2+1];                            \
    void* data = NULL;                                                         \
                                                                               \
    for(;;)                                                                    \
    {                                                                          \
        char* len = get_field(&ptr, "Len");                                    \
        if(!len) break;                                                        \
        char* msg = get_field(&ptr, "Msg");                                    \
        char* md = get_field(&ptr, "MD");                                      \
                                                                               \
        char* endlen;                                                          \
        unsigned long long ulen = strtoull(len, &endlen, 10);                  \
        /* verify test file input */                                           \
        ASSERT_EQ(endlen,len + strlen(len));                                   \
        ASSERT_EQ(ulen % 8, 0);                                                \
        ASSERT_LE(ulen / 8 * 2, strlen(msg));                                  \
        ASSERT_EQ(TCHASH_##UHASH##_DIGEST_SIZE * 2, strlen(md));               \
                                                                               \
        data = realloc(data, ulen / 8);                                        \
        ASSERT_NOTNULL(data);                                                  \
                                                                               \
        size_t nbytes = tchash_bytes_from_xstring(data, msg, ulen / 8 * 2);    \
        ASSERT_EQ(nbytes, ulen / 8);                                           \
                                                                               \
        tchash_##LHASH(result, data, nbytes);                                  \
        size_t nstring = tchash_xstring_from_bytes(tresult, result, TCHASH_##UHASH##_DIGEST_SIZE);\
        ASSERT_EQ(nstring, TCHASH_##UHASH##_DIGEST_SIZE * 2);                  \
                                                                               \
        /*printf("%s == %s\n", tresult, md);*/                                 \
        /*printf("%lu | %s <=> %s\n", (unsigned long)ulen / 8, tresult, md);*/ \
        ASSERT_STREQ(tresult, md);                                             \
    }                                                                          \
                                                                               \
    free(data);                                                                \
    free(text); )

#define HELPER_MONTE(LHASH,UHASH,FNAME,NUMJ,STARTI,ENDI) (                     \
    size_t dlen;                                                               \
    char* text = read_all(&dlen, TESTDATA_ROOT "/" FNAME ".rsp");              \
    ASSERT_NOTNULL(text);                                                      \
    char* ptr = text;                                                          \
                                                                               \
    char* seed = get_field(&ptr, "Seed");                                      \
    ASSERT_NOTNULL(seed);                                                      \
    ASSERT_EQ(strlen(seed), 2 * TCHASH_##UHASH##_DIGEST_SIZE);                 \
                                                                               \
    char msg[3 * TCHASH_##UHASH##_DIGEST_SIZE];                                \
    size_t mpart = tchash_bytes_from_xstring(msg + 2 * TCHASH_##UHASH##_DIGEST_SIZE, seed, -1);\
    ASSERT_EQ(mpart, TCHASH_##UHASH##_DIGEST_SIZE);                            \
                                                                               \
    long long icount = 0;                                                      \
    char* md = NULL;                                                           \
                                                                               \
    int j, i;                                                                  \
    for(j = 0; j < (NUMJ); j++)                                                \
    {                                                                          \
        while(icount <= j)                                                     \
        {                                                                      \
            char* count = get_field(&ptr, "Count");                            \
            if(!count) { j = (NUMJ); break; }                                  \
            char* endcount;                                                    \
            icount = strtoll(count, &endcount, 10) + 1;                        \
            ASSERT_EQ(endcount, count + strlen(count));                        \
            md = get_field(&ptr, "MD");                                        \
        }                                                                      \
        if(!md) break;                                                         \
        ASSERT_EQ(strlen(md), 2 * TCHASH_##UHASH##_DIGEST_SIZE);               \
                                                                               \
        memcpy(msg + 0 * TCHASH_##UHASH##_DIGEST_SIZE, msg + 2 * TCHASH_##UHASH##_DIGEST_SIZE, TCHASH_##UHASH##_DIGEST_SIZE);\
        memcpy(msg + 1 * TCHASH_##UHASH##_DIGEST_SIZE, msg + 2 * TCHASH_##UHASH##_DIGEST_SIZE, TCHASH_##UHASH##_DIGEST_SIZE);\
        for(i = (STARTI); i <= (ENDI); i++)                                    \
        {                                                                      \
            TCHash_##UHASH LHASH;                                              \
            tchash_##LHASH##_init(&LHASH);                                     \
            tchash_##LHASH##_process(&LHASH, msg, sizeof(msg));                \
            memmove(msg, msg + 1 * TCHASH_##UHASH##_DIGEST_SIZE, 2 * TCHASH_##UHASH##_DIGEST_SIZE);\
            tchash_##LHASH##_get(&LHASH, msg + 2 * TCHASH_##UHASH##_DIGEST_SIZE);\
        }                                                                      \
                                                                               \
        char sresult[TCHASH_##UHASH##_DIGEST_SIZE*2+1];                        \
        size_t slen = tchash_xstring_from_bytes(sresult, msg + 2 * TCHASH_##UHASH##_DIGEST_SIZE, TCHASH_##UHASH##_DIGEST_SIZE);\
        ASSERT_EQ(slen, 2 * TCHASH_##UHASH##_DIGEST_SIZE);                     \
        /*printf("%2d: %s\n", j, sresult);*/                                   \
        ASSERT_STREQ(sresult, md);                                             \
    }                                                                          \
                                                                               \
    free(text); )

// MD5
TEST(MD5,(
    static const char* TestVectors[] = {
        "", "d41d8cd98f00b204e9800998ecf8427e",
        "The quick brown fox jumps over the lazy dog", "9e107d9d372bb6826bd81d3542a419d6",
        "The quick brown fox jumps over the lazy dog.", "e4d909c290d0fb1ca068ffaddf22cbd0",
    };

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
        TEST_h_md5(TestVectors[i+1], TestVectors[i+0], strlen(TestVectors[i+0]));
));

// SHA-1
TEST(SHA1_ShortMsg,HELPER_MSG(sha1,SHA1,"SHA1ShortMsg"))
TEST(SHA1_LongMsg,HELPER_MSG(sha1,SHA1,"SHA1LongMsg"))
TEST(SHA1_MonteCarlo,HELPER_MONTE(sha1,SHA1,"SHA1Monte",100,3,1002))

// SHA-2
// basic
TEST(SHA2_256_ShortMsg,HELPER_MSG(sha2_256,SHA2_256,"SHA256ShortMsg"))
TEST(SHA2_256_LongMsg,HELPER_MSG(sha2_256,SHA2_256,"SHA256LongMsg"))
TEST(SHA2_256_MonteCarlo,HELPER_MONTE(sha2_256,SHA2_256,"SHA256Monte",100,3,1002))
TEST(SHA2_512_ShortMsg,HELPER_MSG(sha2_512,SHA2_512,"SHA512ShortMsg"))
TEST(SHA2_512_LongMsg,HELPER_MSG(sha2_512,SHA2_512,"SHA512LongMsg"))
TEST(SHA2_512_MonteCarlo,HELPER_MONTE(sha2_512,SHA2_512,"SHA512Monte",100,3,1002))
// truncated (A)
TEST(SHA2_224_ShortMsg,HELPER_MSG(sha2_224,SHA2_224,"SHA224ShortMsg"))
TEST(SHA2_224_LongMsg,HELPER_MSG(sha2_224,SHA2_224,"SHA224LongMsg"))
TEST(SHA2_224_MonteCarlo,HELPER_MONTE(sha2_224,SHA2_224,"SHA224Monte",100,3,1002))
TEST(SHA2_384_ShortMsg,HELPER_MSG(sha2_384,SHA2_384,"SHA384ShortMsg"))
TEST(SHA2_384_LongMsg,HELPER_MSG(sha2_384,SHA2_384,"SHA384LongMsg"))
TEST(SHA2_384_MonteCarlo,HELPER_MONTE(sha2_384,SHA2_384,"SHA384Monte",100,3,1002))
// truncated (B)
TEST(SHA2_512_224_ShortMsg,HELPER_MSG(sha2_512_224,SHA2_512_224,"SHA512_224ShortMsg"))
TEST(SHA2_512_224_LongMsg,HELPER_MSG(sha2_512_224,SHA2_512_224,"SHA512_224LongMsg"))
TEST(SHA2_512_224_MonteCarlo,HELPER_MONTE(sha2_512_224,SHA2_512_224,"SHA512_224Monte",100,3,1002))
TEST(SHA2_512_256_ShortMsg,HELPER_MSG(sha2_512_256,SHA2_512_256,"SHA512_256ShortMsg"))
TEST(SHA2_512_256_LongMsg,HELPER_MSG(sha2_512_256,SHA2_512_256,"SHA512_256LongMsg"))
TEST(SHA2_512_256_MonteCarlo,HELPER_MONTE(sha2_512_256,SHA2_512_256,"SHA512_256Monte",100,3,1002))

int main(void)
{
    TESTS_BEGIN();

    TEST_EXEC(StringConv);

    TEST_EXEC(MD5);

    TEST_HEADER("SHA-1");
        TEST_EXEC(SHA1_ShortMsg);
        TEST_EXEC(SHA1_LongMsg);
        TEST_EXEC(SHA1_MonteCarlo);
    TEST_HEADER("SHA-2");
        // basic
        TEST_EXEC(SHA2_256_ShortMsg);
        TEST_EXEC(SHA2_256_LongMsg);
        TEST_EXEC(SHA2_256_MonteCarlo);
        TEST_EXEC(SHA2_512_ShortMsg);
        TEST_EXEC(SHA2_512_LongMsg);
        TEST_EXEC(SHA2_512_MonteCarlo);
        // truncated (A)
        TEST_EXEC(SHA2_224_ShortMsg);
        TEST_EXEC(SHA2_224_LongMsg);
        TEST_EXEC(SHA2_224_MonteCarlo);
        TEST_EXEC(SHA2_384_ShortMsg);
        TEST_EXEC(SHA2_384_LongMsg);
        TEST_EXEC(SHA2_384_MonteCarlo);
        // truncated (B)
        TEST_EXEC(SHA2_512_224_ShortMsg);
        TEST_EXEC(SHA2_512_224_LongMsg);
        TEST_EXEC(SHA2_512_224_MonteCarlo);
        TEST_EXEC(SHA2_512_256_ShortMsg);
        TEST_EXEC(SHA2_512_256_LongMsg);
        TEST_EXEC(SHA2_512_256_MonteCarlo);

    TESTS_END();

    return 0;
}
