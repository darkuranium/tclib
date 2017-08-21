#define TC_HASH_IMPLEMENTATION
#include "../tc_hash.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#define RBUF_SIZE   65536

#define HASH_FILE(LHASH,UHASH,FNAME)                                           \
    do {                                                                       \
        const char* fname_ = (FNAME);                                          \
        FILE* file = fopen(fname_, "rb");                                      \
        if(!file) { fprintf(stderr, "Error: Unable to open file `%s`: %s", fname_, strerror(errno)); retval = 1; break; }\
                                                                               \
        char digest[TCHASH_##UHASH##_DIGEST_SIZE];                             \
        char tdigest[2*sizeof(digest)+1];                                      \
                                                                               \
        TCHash_##UHASH LHASH;                                                  \
        tchash_##LHASH##_init(&LHASH);                                         \
        size_t rlen;                                                           \
        do                                                                     \
        {                                                                      \
            rlen = fread(rbuf, 1, sizeof(rbuf), file);                         \
            tchash_##LHASH##_process(&LHASH, rbuf, rlen);                      \
        }                                                                      \
        while(rlen == sizeof(rbuf));                                           \
        tchash_##LHASH##_get(&LHASH, digest);                                  \
        tchash_xstring_from_bytes(tdigest, digest, sizeof(digest), 0);         \
        printf("%s\t*%s\n", tdigest, fname_);                                  \
    } while(0)
#define HASH_FILE_DLEN(LHASH,UHASH,FNAME,DLEN)                                 \
    do {                                                                       \
        const char* fname_ = (FNAME);                                          \
        FILE* file = fopen(fname_, "rb");                                      \
        if(!file) { fprintf(stderr, "Error: Unable to open file `%s`: %s", fname_, strerror(errno)); break; }\
                                                                               \
        char* digest = malloc((DLEN));                                         \
        char* tdigest = malloc(2*(DLEN)+1);                                    \
                                                                               \
        TCHash_##UHASH LHASH;                                                  \
        tchash_##LHASH##_init(&LHASH);                                         \
        size_t rlen;                                                           \
        do                                                                     \
        {                                                                      \
            rlen = fread(rbuf, 1, sizeof(rbuf), file);                         \
            tchash_##LHASH##_process(&LHASH, rbuf, rlen);                      \
        }                                                                      \
        while(rlen == sizeof(rbuf));                                           \
        tchash_##LHASH##_get(&LHASH, digest, (DLEN));                          \
        tchash_xstring_from_bytes(tdigest, digest, (DLEN), 0);                 \
        printf("%s\t*%s\n", tdigest, fname_);                                  \
    } while(0)

void usage(FILE* file, int ecode)
{
    fprintf(file, "Usage: tchash -<alg> <files>...\n");
    fprintf(file, "\tSupported algorithms:\n");
    fprintf(file, "\t\tMD5\n");
    fprintf(file, "\t\tSHA1\n");
    fprintf(file, "\t\tSHA2-{224,256,384,512,512/224,512/256}\n");
    fprintf(file, "\t\tSHA3-{224,256,384,512}\n");
    fprintf(file, "\t\tSHAKE{128,256}/? (where '?' is the digest size)");
    exit(ecode);
}
int main(int argc, char** argv)
{
    if(argc < 3)
        usage(stderr, 2);

    char* alg = argv[1];
    if(*alg != '-') usage(stderr, 2);
    alg++;

    int i;
    for(i = 0; alg[i]; i++) alg[i] = tolower((unsigned char)alg[i]);

    size_t dlen = 0;
    if(strstr(alg, "shake128/") == alg || strstr(alg, "shake256/") == alg)
    {
        char* dlenstr = alg + sizeof("shake128/") - 1;

        char* end;
        dlen = strtoul(dlenstr, &end, 10);
        if(end != dlenstr + strlen(dlenstr))
        {
            fprintf(stderr, "Error: Invalid SHAKE digest length `%s`\n", dlenstr);
            return 2;
        }

        if(dlen % 8)
        {
            fprintf(stderr, "Error: SHAKE output length must be a multiple of 8\n");
            return 2;
        }
        dlen /= 8;
    }

    int retval = 0;
    static char rbuf[RBUF_SIZE];
    for(i = 2; i < argc; i++)
    {
        if(0) {}
        else if(!strcmp(alg, "md5")) HASH_FILE(md5,MD5,argv[i]);
        else if(!strcmp(alg, "sha1")) HASH_FILE(sha1,SHA1,argv[i]);
        else if(!strcmp(alg, "sha2-224") || !strcmp(alg, "sha224")) HASH_FILE(sha2_224,SHA2_224,argv[i]);
        else if(!strcmp(alg, "sha2-256") || !strcmp(alg, "sha256")) HASH_FILE(sha2_256,SHA2_256,argv[i]);
        else if(!strcmp(alg, "sha2-384") || !strcmp(alg, "sha384")) HASH_FILE(sha2_384,SHA2_384,argv[i]);
        else if(!strcmp(alg, "sha2-512") || !strcmp(alg, "sha512")) HASH_FILE(sha2_512,SHA2_512,argv[i]);
        else if(!strcmp(alg, "sha2-512/224") || !strcmp(alg, "sha512/224") || !strcmp(alg, "sha2-512-224") || !strcmp(alg, "sha-512-224")) HASH_FILE(sha2_512_224,SHA2_512_224,argv[i]);
        else if(!strcmp(alg, "sha2-512/256") || !strcmp(alg, "sha512/256") || !strcmp(alg, "sha2-512-256") || !strcmp(alg, "sha-512-256")) HASH_FILE(sha2_512_256,SHA2_512_256,argv[i]);
        else if(!strcmp(alg, "sha3-224")) HASH_FILE(sha3_224,SHA3_224,argv[i]);
        else if(!strcmp(alg, "sha3-256")) HASH_FILE(sha3_256,SHA3_256,argv[i]);
        else if(!strcmp(alg, "sha3-384")) HASH_FILE(sha3_384,SHA3_384,argv[i]);
        else if(!strcmp(alg, "sha3-512")) HASH_FILE(sha3_512,SHA3_512,argv[i]);
        else if(strstr(alg, "shake128/") == alg) HASH_FILE_DLEN(shake128,SHAKE128,argv[i],dlen);
        else if(strstr(alg, "shake256/") == alg) HASH_FILE_DLEN(shake256,SHAKE256,argv[i],dlen);
        else
        {
            if(!strcmp(alg, "shake128") || !strcmp(alg, "shake256"))
            {
                fprintf(stderr, "Error: SHAKE needs a provided length (use e.g. `SHAKE128/256`)\n");
                return 2;
            }

            fprintf(stderr, "Error: Unknown algorithm `%s`\n", alg);
            return 2;
        }
    }
    return retval;
}
