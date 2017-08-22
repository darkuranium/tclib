#define TC_HASH_IMPLEMENTATION
#include "../tc_hash.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>

#include <assert.h>

#include "test.h"

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
char* get_global(char** ptr, const char* rkey)
{
    for(;;)
    {
        if(!**ptr) return NULL;

        char* line = skip_leading_space(get_line(ptr));
        if(!line[0] || line[0] == '#' || line[0] != '[') continue;
        line++;

        char* end = strchr(line, ']');
        if(!end) return NULL; /* ERROR */
        *end = 0;

        char* eq = strchr(line, '=');
        if(!eq) return NULL; /* ERROR */
        *eq = 0;

        char* key = trim_trailing_space(line, eq);
        char* val = trim_trailing_space(skip_leading_space(eq + 1), NULL);

        if(!strcmp(key, rkey)) return val;
    }
    return NULL;
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
    return NULL;
}

#define TESTDATA_ROOT "../tests/NIST"

#define HELPER_MSG_DSIZE(LHASH,UHASH,FNAME,ONAME,DSIZE,HARGS) (                \
    size_t dlen;                                                               \
    char* text = read_all(&dlen, TESTDATA_ROOT "/" FNAME ".rsp");              \
    ASSERT_NOTNULL(text);                                                      \
    char* ptr = text;                                                          \
                                                                               \
    char result[(DSIZE)];                                                      \
    char tresult[(DSIZE)*2+1];                                                 \
    void* data = NULL;                                                         \
                                                                               \
    for(;;)                                                                    \
    {                                                                          \
        char* len = get_field(&ptr, "Len");                                    \
        if(!len) break;                                                        \
        char* msg = get_field(&ptr, "Msg");                                    \
        char* md = get_field(&ptr, ONAME);                                     \
                                                                               \
        char* endlen;                                                          \
        unsigned long long ulen = strtoull(len, &endlen, 10);                  \
        /* verify test file input */                                           \
        ASSERT_EQ(endlen,len + strlen(len));                                   \
        ASSERT_EQ(ulen % 8, 0);                                                \
        ASSERT_LE(ulen / 8 * 2, strlen(msg));                                  \
        ASSERT_EQ((DSIZE) * 2, strlen(md));                                    \
                                                                               \
        data = realloc(data, ulen / 8);                                        \
        ASSERT_NOTNULL(data);                                                  \
                                                                               \
        size_t nbytes = tchash_bytes_from_xstring(data, msg, ulen / 8 * 2);    \
        ASSERT_EQ(nbytes, ulen / 8);                                           \
                                                                               \
        tchash_##LHASH HARGS;                                                  \
        size_t nstring = tchash_xstring_from_bytes(tresult, result, (DSIZE), 0);\
        ASSERT_EQ(nstring, (DSIZE) * 2);                                       \
                                                                               \
        /*printf("%s == %s\n", tresult, md);*/                                 \
        /*printf("%lu | %s <=> %s\n", (unsigned long)ulen / 8, tresult, md);*/ \
        ASSERT_STREQ(tresult, md);                                             \
    }                                                                          \
                                                                               \
    free(data);                                                                \
    free(text); )
#define HELPER_MSG(LHASH,UHASH,FNAME)   HELPER_MSG_DSIZE(LHASH,UHASH,FNAME,"MD",TCHASH_##UHASH##_DIGEST_SIZE,(result, data, nbytes))

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
            char* count = get_field(&ptr, "COUNT");                            \
            ASSERT_NOTNULL(count);                                             \
            char* endcount;                                                    \
            icount = strtoll(count, &endcount, 10) + 1;                        \
            ASSERT_EQ(endcount, count + strlen(count));                        \
                                                                               \
            md = get_field(&ptr, "MD");                                        \
            ASSERT_NOTNULL(md);                                                \
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
        size_t slen = tchash_xstring_from_bytes(sresult, msg + 2 * TCHASH_##UHASH##_DIGEST_SIZE, TCHASH_##UHASH##_DIGEST_SIZE, 0);\
        ASSERT_EQ(slen, 2 * TCHASH_##UHASH##_DIGEST_SIZE);                     \
        if(strcmp(sresult, md)) printf("%2d: %s\n", j, sresult);               \
        ASSERT_STREQ(sresult, md);                                             \
    }                                                                          \
                                                                               \
    free(text); )

#define HELPER_VAROUT(LHASH,UHASH,FNAME,MSGLEN) (                              \
    size_t dlen;                                                               \
    char* text = read_all(&dlen, TESTDATA_ROOT "/" FNAME ".rsp");              \
    ASSERT_NOTNULL(text);                                                      \
    char* ptr = text;                                                          \
                                                                               \
    void* result = NULL;                                                       \
    char* tresult = NULL;                                                      \
    char data[(MSGLEN)];                                                       \
                                                                               \
    for(;;)                                                                    \
    {                                                                          \
        char* olen = get_field(&ptr, "Outputlen");                             \
        if(!olen) break;                                                       \
        char* msg = get_field(&ptr, "Msg");                                    \
        char* md = get_field(&ptr, "Output");                                  \
                                                                               \
        ASSERT_EQ((MSGLEN) * 2, strlen(msg));                                  \
        char* endolen;                                                         \
        unsigned long long ulen = strtoull(olen, &endolen, 10);                \
        /* verify test file input */                                           \
        ASSERT_EQ(endolen,olen + strlen(olen));                                \
        ASSERT_EQ(ulen % 8, 0);                                                \
        ulen /= 8;                                                             \
        ASSERT_LE(ulen * 2, strlen(md));                                       \
                                                                               \
        result = realloc(result, ulen);                                        \
        ASSERT_NOTNULL(result);                                                \
        tresult = realloc(tresult, ulen * 2 + 1);                              \
        ASSERT_NOTNULL(tresult);                                               \
                                                                               \
        size_t nbytes = tchash_bytes_from_xstring(data, msg, (MSGLEN) * 2);    \
        ASSERT_EQ(nbytes, (MSGLEN));                                           \
                                                                               \
        tchash_##LHASH(result, ulen, data, nbytes);                            \
        size_t nstring = tchash_xstring_from_bytes(tresult, result, ulen, 0);  \
        ASSERT_EQ(nstring, ulen * 2);                                          \
                                                                               \
        /*printf("%s == %s\n", tresult, md);*/                                 \
        /*printf("%lu | %s <=> %s\n", (unsigned long)ulen, tresult, md);*/     \
        ASSERT_STREQ(tresult, md);                                             \
    }                                                                          \
                                                                               \
    free(tresult);                                                             \
    free(result);                                                              \
    free(text); )
#define HELPER_MONTE_SHA3(LHASH,UHASH,FNAME,NUMJ,STARTI,ENDI) (                \
    size_t dlen;                                                               \
    char* text = read_all(&dlen, TESTDATA_ROOT "/" FNAME ".rsp");              \
    ASSERT_NOTNULL(text);                                                      \
    char* ptr = text;                                                          \
                                                                               \
    char* seed = get_field(&ptr, "Seed");                                      \
    ASSERT_NOTNULL(seed);                                                      \
    ASSERT_EQ(strlen(seed), 2 * TCHASH_##UHASH##_DIGEST_SIZE);                 \
                                                                               \
    char msg[TCHASH_##UHASH##_DIGEST_SIZE];                                    \
    size_t mlen = tchash_bytes_from_xstring(msg, seed, -1);                    \
    ASSERT_EQ(mlen, TCHASH_##UHASH##_DIGEST_SIZE);                             \
                                                                               \
    long long icount = 0;                                                      \
    char* md = NULL;                                                           \
                                                                               \
    int j, i;                                                                  \
    for(j = 0; j < (NUMJ); j++)                                                \
    {                                                                          \
        while(icount <= j)                                                     \
        {                                                                      \
            char* count = get_field(&ptr, "COUNT");                            \
            ASSERT_NOTNULL(count);                                             \
            char* endcount;                                                    \
            icount = strtoll(count, &endcount, 10) + 1;                        \
            ASSERT_EQ(endcount, count + strlen(count));                        \
                                                                               \
            md = get_field(&ptr, "MD");                                        \
            ASSERT_NOTNULL(md);                                                \
        }                                                                      \
        if(!md) break;                                                         \
        ASSERT_EQ(strlen(md), 2 * TCHASH_##UHASH##_DIGEST_SIZE);               \
                                                                               \
        for(i = (STARTI); i <= (ENDI); i++)                                    \
            tchash_##LHASH(msg, msg, mlen);                                    \
                                                                               \
        char sresult[TCHASH_##UHASH##_DIGEST_SIZE*2+1];                        \
        size_t slen = tchash_xstring_from_bytes(sresult, msg, mlen, 0);        \
        ASSERT_EQ(slen, 2 * TCHASH_##UHASH##_DIGEST_SIZE);                     \
        if(strcmp(sresult, md)) printf("%2d: %s\n", j, sresult);               \
        ASSERT_STREQ(sresult, md);                                             \
    }                                                                          \
                                                                               \
    free(text); )
#define HELPER_MONTE_SHAKE(LHASH,UHASH,FNAME) (                                \
    size_t dlen;                                                               \
    char* text = read_all(&dlen, TESTDATA_ROOT "/" FNAME ".rsp");              \
    ASSERT_NOTNULL(text);                                                      \
    char* ptr = text;                                                          \
                                                                               \
    const size_t ILen = 128 / 8;                                               \
                                                                               \
    char* minlen = get_global(&ptr, "Minimum Output Length (bits)");           \
    ASSERT_NOTNULL(minlen);                                                    \
    char* endmin;                                                              \
    unsigned long long uminlen = strtoull(minlen, &endmin, 10);                \
    ASSERT_EQ(minlen + strlen(minlen), endmin);                                \
    ASSERT_EQ(uminlen % 8, 0);                                                 \
    uminlen /= 8;                                                              \
                                                                               \
    char* maxlen = get_global(&ptr, "Maximum Output Length (bits)");           \
    ASSERT_NOTNULL(maxlen);                                                    \
    char* endmax;                                                              \
    unsigned long long umaxlen = strtoull(maxlen, &endmax, 10);                \
    ASSERT_EQ(maxlen + strlen(maxlen), endmax);                                \
    ASSERT_EQ(umaxlen % 8, 0);                                                 \
    umaxlen /= 8;                                                              \
                                                                               \
    char* seed = get_field(&ptr, "Msg");                                       \
    ASSERT_EQ(strlen(seed), 2 * ILen);                                         \
                                                                               \
    char* msg[2];                                                              \
    msg[0] = malloc(ILen);                                                     \
    ASSERT_NOTNULL(msg[0]);                                                    \
    msg[1] = NULL;                                                             \
    size_t mlen = tchash_bytes_from_xstring(msg[0], seed, -1);                 \
    ASSERT_EQ(mlen, ILen);                                                     \
    int cmsg = 1;                                                              \
                                                                               \
    long long icount = 0;                                                      \
    unsigned long long uoutlen;                                                \
    char* md = NULL;                                                           \
                                                                               \
    char* sdigest = NULL;                                                      \
                                                                               \
    size_t outputlen = umaxlen;                                                \
                                                                               \
    int j, i;                                                                  \
    for(j = 0; j < 100; j++)                                                   \
    {                                                                          \
        while(icount <= j)                                                     \
        {                                                                      \
            char* count = get_field(&ptr, "COUNT");                            \
            ASSERT_NOTNULL(count);                                             \
            char* endcount;                                                    \
            icount = strtoll(count, &endcount, 10) + 1;                        \
            ASSERT_EQ(endcount, count + strlen(count));                        \
                                                                               \
            char* outlen = get_field(&ptr, "Outputlen");                       \
            ASSERT_NOTNULL(outlen);                                            \
            char* endoutlen;                                                   \
            uoutlen = strtoull(outlen, &endoutlen, 10);                        \
            ASSERT_EQ(endoutlen, outlen + strlen(outlen));                     \
            ASSERT_EQ(uoutlen % 8, 0);                                         \
            uoutlen /= 8;                                                      \
                                                                               \
            md = get_field(&ptr, "Output");                                    \
            ASSERT_NOTNULL(md);                                                \
        }                                                                      \
        if(!md) break;                                                         \
        for(i = 1; i <= 1000; i++)                                             \
        {                                                                      \
            msg[cmsg] = realloc(msg[cmsg], outputlen > ILen ? outputlen : ILen);\
            ASSERT_NOTNULL(msg[cmsg]);                                         \
            if(mlen < ILen)                                                    \
                memset(msg[!cmsg] + mlen, 0, ILen - mlen);                     \
            tchash_##LHASH(msg[cmsg], outputlen, msg[!cmsg], ILen);            \
            cmsg = !cmsg;                                                      \
            mlen = outputlen;                                                  \
            uint16_t mright;                                                   \
            memcpy(&mright, &msg[!cmsg][mlen-sizeof(mright)], sizeof(mright)); \
            mright = TCHASH_I_FROM_BE16(mright);                               \
            size_t range = umaxlen - uminlen + 1;                              \
            outputlen = uminlen + mright % range;                              \
        }                                                                      \
        ASSERT_EQ(mlen, uoutlen);                                              \
        sdigest = realloc(sdigest, 2 * mlen + 1);                              \
        ASSERT_NOTNULL(sdigest);                                               \
        size_t slen = tchash_xstring_from_bytes(sdigest, msg[!cmsg], mlen, 0); \
        ASSERT_EQ(slen, 2 * mlen);                                             \
        /*if(strcmp(sdigest, md)) printf("%2d: %.0s\n", j, sdigest);*/         \
        ASSERT_STREQ(sdigest, md);                                             \
    }                                                                          \
                                                                               \
    free(sdigest);                                                             \
    free(msg[1]);                                                              \
    free(msg[0]);                                                              \
    free(text); )

TEST(XStringConv,(
    static const unsigned char bytes[] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};

    char sbuf[sizeof(bytes)*2+1];
    unsigned char bbuf[sizeof(bytes)];

    tchash_xstring_from_bytes(sbuf, bytes, sizeof(bytes), 0);
    tchash_bytes_from_xstring(bbuf, sbuf, sizeof(bytes)*2);

    ASSERT_MEMEQ(bytes,sizeof(bytes),bbuf,sizeof(bbuf));
))

TEST(Base64Conv,(
    static const char* TestVectors[] = {
        // Wikipedia
        "M",    "TQ==",
        "Ma",   "TWE=",
        "Man",  "TWFu",
        "any carnal pleasure.", "YW55IGNhcm5hbCBwbGVhc3VyZS4=",
        "any carnal pleasure",  "YW55IGNhcm5hbCBwbGVhc3VyZQ==",
        "any carnal pleasur",   "YW55IGNhcm5hbCBwbGVhc3Vy",
        "any carnal pleasu",    "YW55IGNhcm5hbCBwbGVhc3U=",
        "any carnal pleas",     "YW55IGNhcm5hbCBwbGVhcw==",
        "pleasure.",    "cGxlYXN1cmUu",
        "leasure.",     "bGVhc3VyZS4=",
        "easure.",      "ZWFzdXJlLg==",
        "asure.",       "YXN1cmUu",
        "sure.",        "c3VyZS4=",
        // RFC-4648
        "",         "",
        "f",        "Zg==",
        "fo",       "Zm8=",
        "foo",      "Zm9v",
        "foob",     "Zm9vYg==",
        "fooba",    "Zm9vYmE=",
        "foobar",   "Zm9vYmFy",
    };

    unsigned char bytes[512];
    char base64str[512];

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
    {
        const char* data = TestVectors[i+0];
        size_t dlen = strlen(data);
        const char* expected = TestVectors[i+1];
        size_t blen;

        tchash_base64_from_bytes(base64str, data, dlen, -1, -1, -1);
        //printf("%s == %s\n", base64str, expected);
        ASSERT_STREQ(base64str, expected);
        blen = tchash_bytes_from_base64(bytes, base64str, -1, -1, -1, -1);
        ASSERT_MEMEQ(bytes, blen, data, dlen);

        tchash_base64_from_bytes(base64str, data, dlen, '!', '.', '?');
        blen = tchash_bytes_from_base64(bytes, base64str, -1, '!', '.', '?');
        ASSERT_MEMEQ(bytes, blen, data, dlen);

        tchash_base64_from_bytes(base64str, data, dlen, '~', ':', 0);
        blen = tchash_bytes_from_base64(bytes, base64str, -1, '~', ':', 0);
        ASSERT_MEMEQ(bytes, blen, data, dlen);
    }
))

// MD5
TEST(MD5,(
    static const char* TestVectors[] = {
        "", "d41d8cd98f00b204e9800998ecf8427e",
        "The quick brown fox jumps over the lazy dog", "9e107d9d372bb6826bd81d3542a419d6",
        "The quick brown fox jumps over the lazy dog.", "e4d909c290d0fb1ca068ffaddf22cbd0",
    };

    char bytes[TCHASH_MD5_DIGEST_SIZE];
    char hexstr[2*sizeof(bytes)+1];

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
    {
        const char* data = TestVectors[i+0];
        size_t dlen = strlen(data);
        const char* expected = TestVectors[i+1];

        tchash_md5(bytes, data, dlen);
        tchash_xstring_from_bytes(hexstr, bytes, sizeof(bytes), 0);
        //printf("%s == %s\n", hexstr, expected);
        ASSERT_STREQ(hexstr, expected);
    }
))

// Tiger
TEST(Tiger,(
    static const char* TestVectors[] = {
        "", "3293ac630c13f0245f92bbb1766e16167a4e58492dde73f3",
        "The quick brown fox jumps over the lazy dog", "6d12a41e72e644f017b6f0e2f7b44c6285f06dd5d2c5b075",
        "The quick brown fox jumps over the lazy cog", "a8f04b0f7201a0d728101c9d26525b31764a3493fcd8458f",
    };

    char bytes[TCHASH_TIGER192_DIGEST_SIZE];
    char hexstr[2*sizeof(bytes)+1];

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
    {
        const char* data = TestVectors[i+0];
        size_t dlen = strlen(data);
        const char* expected = TestVectors[i+1];

        tchash_tiger192(bytes, data, dlen);
        tchash_xstring_from_bytes(hexstr, bytes, sizeof(bytes), 0);
        //printf("%s == %s\n", hexstr, expected);
        ASSERT_STREQ(hexstr, expected);
    }
))
TEST(Tiger2,(
    static const char* TestVectors[] = {
        "", "4441be75f6018773c206c22745374b924aa8313fef919f41",
        "The quick brown fox jumps over the lazy dog", "976abff8062a2e9dcea3a1ace966ed9c19cb85558b4976d8",
        "The quick brown fox jumps over the lazy cog", "09c11330283a27efb51930aa7dc1ec624ff738a8d9bdd3df",
    };

    char bytes[TCHASH_TIGER2_192_DIGEST_SIZE];
    char hexstr[2*sizeof(bytes)+1];

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
    {
        const char* data = TestVectors[i+0];
        size_t dlen = strlen(data);
        const char* expected = TestVectors[i+1];

        tchash_tiger2_192(bytes, data, dlen);
        tchash_xstring_from_bytes(hexstr, bytes, sizeof(bytes), 0);
        //printf("%s == %s\n", hexstr, expected);
        ASSERT_STREQ(hexstr, expected);
    }
))

// RIPEMD
TEST(RIPEMD128,(
    static const char* TestVectors[] = {
        // AB-9601
        "", "cdf26213a150dc3ecb610f18f6b38b46",
        "a", "86be7afa339d0fc7cfc785e72f578d33",
        "abc", "c14a12199c66e4ba84636b0f69144c77",
        "message digest", "9e327b3d6e523062afc1132d7df9d1b8",
        "abcdefghijklmnopqrstuvwxyz", "fd2aa607f71dc8f510714922b371834e",
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "a1aa0689d0fafa2ddc22e88b49133a06",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d1e959eb179c911faea4624c60c5c702",
        /*8 times "1234567890"*/"12345678901234567890123456789012345678901234567890123456789012345678901234567890", "3f45ef194732c2dbb2c4a2c769795fa3",
        /*1 million times "a": "...", "4a7f5723f954eba1216c9d8f6320431f"*/
    };

    char bytes[TCHASH_RIPEMD128_DIGEST_SIZE];
    char hexstr[2*sizeof(bytes)+1];

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
    {
        const char* data = TestVectors[i+0];
        size_t dlen = strlen(data);
        const char* expected = TestVectors[i+1];

        tchash_ripemd128(bytes, data, dlen);
        tchash_xstring_from_bytes(hexstr, bytes, sizeof(bytes), 0);
        //printf("%s == %s\n", hexstr, expected);
        ASSERT_STREQ(hexstr, expected);
    }
))
TEST(RIPEMD160,(
    static const char* TestVectors[] = {
        // Wikipedia
        "", "9c1185a5c5e9fc54612808977ee8f548b2258d31",
        "The quick brown fox jumps over the lazy dog", "37f332f68db77bd9d7edd4969571ad671cf9dd3b",
        "The quick brown fox jumps over the lazy cog", "132072df690933835eb8b6ad0b77e7b6f14acad7",
        // AB-9601
        "a", "0bdc9d2d256b3ee9daae347be6f4dc835a467ffe",
        "abc", "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc",
        "message digest", "5d0689ef49d2fae572b881b123a85ffa21595f36",
        "abcdefghijklmnopqrstuvwxyz", "f71c27109c692c1b56bbdceb5b9d2865b3708dbc",
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "12a053384a9c0c88e405a06c27dcf49ada62eb2b",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "b0e20b6e3116640286ed3a87a5713079b21f5189",
        /*8 times "1234567890"*/"12345678901234567890123456789012345678901234567890123456789012345678901234567890", "9b752e45573d4b39f4dbd3323cab82bf63326bfb",
        /*1 million times "a": "...", "52783243c1697bdbe16d37f97f68f08325dc1528"*/
    };

    char bytes[TCHASH_RIPEMD160_DIGEST_SIZE];
    char hexstr[2*sizeof(bytes)+1];

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
    {
        const char* data = TestVectors[i+0];
        size_t dlen = strlen(data);
        const char* expected = TestVectors[i+1];

        tchash_ripemd160(bytes, data, dlen);
        tchash_xstring_from_bytes(hexstr, bytes, sizeof(bytes), 0);
        //printf("%s == %s\n", hexstr, expected);
        ASSERT_STREQ(hexstr, expected);
    }
))
TEST(RIPEMD256,(
    static const char* TestVectors[] = {
        // https://homes.esat.kuleuven.be/~bosselae/ripemd160.html
        "", "02ba4c4e5f8ecd1877fc52d64d30e37a2d9774fb1e5d026380ae0168e3c5522d",
        "a", "f9333e45d857f5d90a91bab70a1eba0cfb1be4b0783c9acfcd883a9134692925",
        "abc", "afbd6e228b9d8cbbcef5ca2d03e6dba10ac0bc7dcbe4680e1e42d2e975459b65",
        "message digest", "87e971759a1ce47a514d5c914c392c9018c7c46bc14465554afcdf54a5070c0e",
        "abcdefghijklmnopqrstuvwxyz", "649d3034751ea216776bf9a18acc81bc7896118a5197968782dd1fd97d8d5133",
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "3843045583aac6c8c8d9128573e7a9809afb2a0f34ccc36ea9e72f16f6368e3f",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "5740a408ac16b720b84424ae931cbb1fe363d1d0bf4017f1a89f7ea6de77a0b8",
        /*8 times "1234567890"*/"12345678901234567890123456789012345678901234567890123456789012345678901234567890", "06fdcc7a409548aaf91368c06a6275b553e3f099bf0ea4edfd6778df89a890dd",
        /*1 million times "a": "...", "ac953744e10e31514c150d4d8d7b677342e33399788296e43ae4850ce4f97978"*/
    };

    char bytes[TCHASH_RIPEMD256_DIGEST_SIZE];
    char hexstr[2*sizeof(bytes)+1];

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
    {
        const char* data = TestVectors[i+0];
        size_t dlen = strlen(data);
        const char* expected = TestVectors[i+1];

        tchash_ripemd256(bytes, data, dlen);
        tchash_xstring_from_bytes(hexstr, bytes, sizeof(bytes), 0);
        //printf("%s == %s\n", hexstr, expected);
        ASSERT_STREQ(hexstr, expected);
    }
))
TEST(RIPEMD320,(
    static const char* TestVectors[] = {
        // https://homes.esat.kuleuven.be/~bosselae/ripemd160.html
        "", "22d65d5661536cdc75c1fdf5c6de7b41b9f27325ebc61e8557177d705a0ec880151c3a32a00899b8",
        "a", "ce78850638f92658a5a585097579926dda667a5716562cfcf6fbe77f63542f99b04705d6970dff5d",
        "abc", "de4c01b3054f8930a79d09ae738e92301e5a17085beffdc1b8d116713e74f82fa942d64cdbc4682d",
        "message digest", "3a8e28502ed45d422f68844f9dd316e7b98533fa3f2a91d29f84d425c88d6b4eff727df66a7c0197",
        "abcdefghijklmnopqrstuvwxyz", "cabdb1810b92470a2093aa6bce05952c28348cf43ff60841975166bb40ed234004b8824463e6b009",
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "d034a7950cf722021ba4b84df769a5de2060e259df4c9bb4a4268c0e935bbc7470a969c9d072a1ac",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "ed544940c86d67f250d232c30b7b3e5770e0c60c8cb9a4cafe3b11388af9920e1b99230b843c86a4",
        /*8 times "1234567890"*/"12345678901234567890123456789012345678901234567890123456789012345678901234567890", "557888af5f6d8ed62ab66945c6d2a0a47ecd5341e915eb8fea1d0524955f825dc717e4a008ab2d42",
        /*1 million times "a": "...", "bdee37f4371e20646b8b0d862dda16292ae36f40965e8c8509e63d1dbddecc503e2b63eb9245bb66"*/
    };

    char bytes[TCHASH_RIPEMD320_DIGEST_SIZE];
    char hexstr[2*sizeof(bytes)+1];

    size_t i;
    for(i = 0; i < sizeof(TestVectors) / sizeof(*TestVectors); i += 2)
    {
        const char* data = TestVectors[i+0];
        size_t dlen = strlen(data);
        const char* expected = TestVectors[i+1];

        tchash_ripemd320(bytes, data, dlen);
        tchash_xstring_from_bytes(hexstr, bytes, sizeof(bytes), 0);
        //printf("%s == %s\n", hexstr, expected);
        ASSERT_STREQ(hexstr, expected);
    }
))

// SHA-1
TEST(SHA1_ShortMsg,HELPER_MSG(sha1,SHA1,"sha/SHA1ShortMsg"))
TEST(SHA1_LongMsg,HELPER_MSG(sha1,SHA1,"sha/SHA1LongMsg"))
TEST(SHA1_MonteCarlo,HELPER_MONTE(sha1,SHA1,"sha/SHA1Monte",100,3,1002))

// SHA-2
// basic
TEST(SHA2_256_ShortMsg,HELPER_MSG(sha2_256,SHA2_256,"sha/SHA256ShortMsg"))
TEST(SHA2_256_LongMsg,HELPER_MSG(sha2_256,SHA2_256,"sha/SHA256LongMsg"))
TEST(SHA2_256_MonteCarlo,HELPER_MONTE(sha2_256,SHA2_256,"sha/SHA256Monte",100,3,1002))
TEST(SHA2_512_ShortMsg,HELPER_MSG(sha2_512,SHA2_512,"sha/SHA512ShortMsg"))
TEST(SHA2_512_LongMsg,HELPER_MSG(sha2_512,SHA2_512,"sha/SHA512LongMsg"))
TEST(SHA2_512_MonteCarlo,HELPER_MONTE(sha2_512,SHA2_512,"sha/SHA512Monte",100,3,1002))
// truncated (A)
TEST(SHA2_224_ShortMsg,HELPER_MSG(sha2_224,SHA2_224,"sha/SHA224ShortMsg"))
TEST(SHA2_224_LongMsg,HELPER_MSG(sha2_224,SHA2_224,"sha/SHA224LongMsg"))
TEST(SHA2_224_MonteCarlo,HELPER_MONTE(sha2_224,SHA2_224,"sha/SHA224Monte",100,3,1002))
TEST(SHA2_384_ShortMsg,HELPER_MSG(sha2_384,SHA2_384,"sha/SHA384ShortMsg"))
TEST(SHA2_384_LongMsg,HELPER_MSG(sha2_384,SHA2_384,"sha/SHA384LongMsg"))
TEST(SHA2_384_MonteCarlo,HELPER_MONTE(sha2_384,SHA2_384,"sha/SHA384Monte",100,3,1002))
// truncated (B)
TEST(SHA2_512_224_ShortMsg,HELPER_MSG(sha2_512_224,SHA2_512_224,"sha/SHA512_224ShortMsg"))
TEST(SHA2_512_224_LongMsg,HELPER_MSG(sha2_512_224,SHA2_512_224,"sha/SHA512_224LongMsg"))
TEST(SHA2_512_224_MonteCarlo,HELPER_MONTE(sha2_512_224,SHA2_512_224,"sha/SHA512_224Monte",100,3,1002))
TEST(SHA2_512_256_ShortMsg,HELPER_MSG(sha2_512_256,SHA2_512_256,"sha/SHA512_256ShortMsg"))
TEST(SHA2_512_256_LongMsg,HELPER_MSG(sha2_512_256,SHA2_512_256,"sha/SHA512_256LongMsg"))
TEST(SHA2_512_256_MonteCarlo,HELPER_MONTE(sha2_512_256,SHA2_512_256,"sha/SHA512_256Monte",100,3,1002))

// SHA-3
TEST(SHA3_224_ShortMsg,HELPER_MSG(sha3_224,SHA3_224,"sha3/SHA3_224ShortMsg"))
TEST(SHA3_224_LongMsg,HELPER_MSG(sha3_224,SHA3_224,"sha3/SHA3_224LongMsg"))
TEST(SHA3_224_MonteCarlo,HELPER_MONTE_SHA3(sha3_224,SHA3_224,"sha3/SHA3_224Monte",100,1,1000))
TEST(SHA3_256_ShortMsg,HELPER_MSG(sha3_256,SHA3_256,"sha3/SHA3_256ShortMsg"))
TEST(SHA3_256_LongMsg,HELPER_MSG(sha3_256,SHA3_256,"sha3/SHA3_256LongMsg"))
TEST(SHA3_256_MonteCarlo,HELPER_MONTE_SHA3(sha3_256,SHA3_256,"sha3/SHA3_256Monte",100,1,1000))
TEST(SHA3_384_ShortMsg,HELPER_MSG(sha3_384,SHA3_384,"sha3/SHA3_384ShortMsg"))
TEST(SHA3_384_LongMsg,HELPER_MSG(sha3_384,SHA3_384,"sha3/SHA3_384LongMsg"))
TEST(SHA3_384_MonteCarlo,HELPER_MONTE_SHA3(sha3_384,SHA3_384,"sha3/SHA3_384Monte",100,1,1000))
TEST(SHA3_512_ShortMsg,HELPER_MSG(sha3_512,SHA3_512,"sha3/SHA3_512ShortMsg"))
TEST(SHA3_512_LongMsg,HELPER_MSG(sha3_512,SHA3_512,"sha3/SHA3_512LongMsg"))
TEST(SHA3_512_MonteCarlo,HELPER_MONTE_SHA3(sha3_512,SHA3_512,"sha3/SHA3_512Monte",100,1,1000))

// SHAKE
TEST(SHAKE128_ShortMsg,HELPER_MSG_DSIZE(shake128,SHAKE128,"shake/SHAKE128ShortMsg","Output",128/8,(result, sizeof(result), data, nbytes)))
TEST(SHAKE128_LongMsg,HELPER_MSG_DSIZE(shake128,SHAKE128,"shake/SHAKE128LongMsg","Output",128/8,(result, sizeof(result), data, nbytes)))
TEST(SHAKE128_VariableOut,HELPER_VAROUT(shake128,SHAKE128,"shake/SHAKE128VariableOut",128/8))
TEST(SHAKE128_MonteCarlo,HELPER_MONTE_SHAKE(shake128,SHAKE128,"shake/SHAKE128Monte"))
TEST(SHAKE256_ShortMsg,HELPER_MSG_DSIZE(shake256,SHAKE256,"shake/SHAKE256ShortMsg","Output",256/8,(result, sizeof(result), data, nbytes)))
TEST(SHAKE256_LongMsg,HELPER_MSG_DSIZE(shake256,SHAKE256,"shake/SHAKE256LongMsg","Output",256/8,(result, sizeof(result), data, nbytes)))
TEST(SHAKE256_VariableOut,HELPER_VAROUT(shake256,SHAKE256,"shake/SHAKE256VariableOut",256/8))
TEST(SHAKE256_MonteCarlo,HELPER_MONTE_SHAKE(shake256,SHAKE256,"shake/SHAKE256Monte"))

int main(void)
{
    TESTS_BEGIN();

    TEST_EXEC(XStringConv);
    TEST_EXEC(Base64Conv);

    TEST_EXEC(MD5);

    TEST_HEADER("Tiger");
        TEST_EXEC(Tiger);
        TEST_EXEC(Tiger2);

    TEST_HEADER("RIPEMD");
        TEST_EXEC(RIPEMD128);
        TEST_EXEC(RIPEMD160);
        TEST_EXEC(RIPEMD256);
        TEST_EXEC(RIPEMD320);

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
    TEST_HEADER("SHA-3");
        TEST_EXEC(SHA3_224_ShortMsg);
        TEST_EXEC(SHA3_224_LongMsg);
        TEST_EXEC(SHA3_224_MonteCarlo);
        TEST_EXEC(SHA3_256_ShortMsg);
        TEST_EXEC(SHA3_256_LongMsg);
        TEST_EXEC(SHA3_256_MonteCarlo);
        TEST_EXEC(SHA3_384_ShortMsg);
        TEST_EXEC(SHA3_384_LongMsg);
        TEST_EXEC(SHA3_384_MonteCarlo);
        TEST_EXEC(SHA3_512_ShortMsg);
        TEST_EXEC(SHA3_512_LongMsg);
        TEST_EXEC(SHA3_512_MonteCarlo);
    TEST_HEADER("SHAKE");
        TEST_EXEC(SHAKE128_ShortMsg);
        TEST_EXEC(SHAKE128_LongMsg);
        TEST_EXEC(SHAKE128_VariableOut);
        TEST_EXEC(SHAKE128_MonteCarlo);
        TEST_EXEC(SHAKE256_ShortMsg);
        TEST_EXEC(SHAKE256_LongMsg);
        TEST_EXEC(SHAKE256_VariableOut);
        TEST_EXEC(SHAKE256_MonteCarlo);

    TESTS_END();

    return 0;
}
