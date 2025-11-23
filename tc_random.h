/*
 * tc_random.h: Random number generation.
 *
 * DEPENDS:
 * VERSION: 0.0.3 (2018-03-28)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Cas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.0.4    added `tcrand_init_os_crypto` which attempts to use an OS-specific crypto generator
 *          (CryptGenRandom, BCryptGenRandom, arc4random, /dev/[u]random, etc)
 * 0.0.3    fixed a bug where mean & sd were swapped for normal2_{d,f}
 * 0.0.2    fixed a bug in generating u32 and u64 integers
 *          (`max` was not being included in the possible results)
 * 0.0.1    initial public release
 *
 * TODOs:
 * - PCG
 *
 * Windows note: By default, BCryptGenRandom is used on Windows, which is *not* supported on versions older than Vista.
 * If support is required, #define TCRAND_WINDOWS_XP_SUPPORT in the same file that defines TC_RANDOM_IMPLEMENTATION.
 */

#ifndef TC_RANDOM_H_
#define TC_RANDOM_H_

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TC_RandGen
{
    void (*seed)(void* data, const void* seed);
    void (*next)(void* data, void* values);
    struct TC_RandGen* (*clone)(void* data, struct TC_RandGen* ngen);
    void (*dealloc)(void* data);
    void* data;
    size_t seed_len;
    void* seed_ptr;
    size_t value_blen;
    size_t value_alen;
    void* value_ptr;
} TC_RandGen;

typedef struct TC_CFloat
{
    float re, im;
} TC_CFloat;
typedef struct TC_CDouble
{
    double re, im;
} TC_CDouble;
#define TC_REAL(x)  (x).re
#define TC_IMAG(x)  (x).im

#define TCRAND_LCG32_SEEDLEN            sizeof(uint32_t)
#define TCRAND_LCG64_SEEDLEN            sizeof(uint64_t)

#define TCRAND_MINSTD0_SEEDLEN          sizeof(uint32_t)
#define TCRAND_MINSTD_SEEDLEN           sizeof(uint32_t)
#define TCRAND_WELL512_SEEDLEN          (16 * sizeof(uint32_t))
#define TCRAND_XOROSHIRO128PLUS_SEEDLEN (2 * sizeof(uint64_t))
#define TCRAND_MT19937_SEEDLEN          sizeof(uint32_t)
#define TCRAND_MT19937_64_SEEDLEN       sizeof(uint64_t)
#define TCRAND_SPLITMIX64_SEEDLEN       sizeof(uint64_t)

#define TCRAND_MINSTD0_MAX          UINT32_C(0x7FFFFFFE)
#define TCRAND_MINSTD_MAX           UINT32_C(0x7FFFFFFE)
#define TCRAND_WELL512_MAX          UINT32_MAX
#define TCRAND_XOROSHIRO128PLUS_MAX UINT64_MAX
#define TCRAND_MT19937_MAX          UINT32_MAX
#define TCRAND_MT19937_64_MAX       UINT64_MAX
#define TCRAND_SPLITMIX64_MAX       UINT64_MAX

extern const uint32_t tcrand_minstd0_default_seed[];
extern const uint32_t tcrand_minstd_default_seed[];
extern const uint32_t tcrand_mt19937_default_seed[];
extern const uint32_t tcrand_mt19937_64_default_seed[];

TC_RandGen* tcrand_init_os_crypto(TC_RandGen* rgen);

TC_RandGen* tcrand_init_lcg32(TC_RandGen* rgen, uint32_t a, uint32_t c, uint32_t m);
TC_RandGen* tcrand_init_lcg64(TC_RandGen* rgen, uint64_t a, uint64_t c, uint64_t m);
TC_RandGen* tcrand_init_well(TC_RandGen* rgen, size_t statelen);

TC_RandGen* tcrand_init_minstd0(TC_RandGen* rgen);
TC_RandGen* tcrand_init_minstd(TC_RandGen* rgen);
TC_RandGen* tcrand_init_well512(TC_RandGen* rgen);
TC_RandGen* tcrand_init_xoroshiro128plus(TC_RandGen* rgen);
TC_RandGen* tcrand_init_pcg_xsh_rr(TC_RandGen* rgen);
TC_RandGen* tcrand_init_mt19937(TC_RandGen* rgen);
TC_RandGen* tcrand_init_mt19937_64(TC_RandGen* rgen);
TC_RandGen* tcrand_init_splitmix64(TC_RandGen* rgen);
TC_RandGen* tcrand_clone(TC_RandGen* ngen, const TC_RandGen* rgen);
void tcrand_deinit(TC_RandGen* rgen);

void tcrand_gen_seedseq(uint32_t* seq, size_t seqlen, const uint32_t* seed, size_t seedlen);

void tcrand_seed_raw(TC_RandGen* rgen, const uint32_t* seed);
void tcrand_seed_u32seq(TC_RandGen* rgen, const uint32_t* seed, size_t slen);
void tcrand_seed_u32(TC_RandGen* rgen, uint32_t seed);

void tcrand_next_raw(TC_RandGen* rgen, void* values);
void tcrand_next_bytes(TC_RandGen* rgen, void* bytes, size_t nbytes);

uint32_t tcrand_next_uniform_u32(TC_RandGen* rgen, uint32_t min, uint32_t max);
uint64_t tcrand_next_uniform_u64(TC_RandGen* rgen, uint64_t min, uint64_t max);
int32_t tcrand_next_uniform_i32(TC_RandGen* rgen, int32_t min, int32_t max);
int64_t tcrand_next_uniform_i64(TC_RandGen* rgen, int64_t min, int64_t max);

// (min,max)
float tcrand_next_uniform_f_oo(TC_RandGen* rgen, float min, float max);
double tcrand_next_uniform_d_oo(TC_RandGen* rgen, double min, double max);
// [min,max)
float tcrand_next_uniform_f_co(TC_RandGen* rgen, float min, float max);
double tcrand_next_uniform_d_co(TC_RandGen* rgen, double min, double max);
// (min,max]
float tcrand_next_uniform_f_oc(TC_RandGen* rgen, float min, float max);
double tcrand_next_uniform_d_oc(TC_RandGen* rgen, double min, double max);
// [min,max]
float tcrand_next_uniform_f_cc(TC_RandGen* rgen, float min, float max);
double tcrand_next_uniform_d_cc(TC_RandGen* rgen, double min, double max);

TC_CFloat tcrand_next_normal2_f(TC_RandGen* rgen, float mean, float sd);
TC_CDouble tcrand_next_normal2_d(TC_RandGen* rgen, double mean, double sd);

#define tcrand_next_normal_f(rgen, mean, sd)    TC_REAL(tcrand_next_normal2_f(rgen, mean, sd))
#define tcrand_next_normal_d(rgen, mean, sd)    TC_REAL(tcrand_next_normal2_d(rgen, mean, sd))

#ifdef __cplusplus
}
#endif

#endif /* TC_RANDOM_H_ */

#ifdef TC_RANDOM_IMPLEMENTATION
#undef TC_RANDOM_IMPLEMENTATION
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <assert.h>

#if defined(__EMSCRIPTEN__)
#   define TC__RANDOM_OS_CRYPTO_WEB_CRYPTO
#   include <emscripten.h>
EM_JS(void, tcrand_i_Crypto_getRandomValues, (void* dataHead, void* dataTail), {
    Crypto.getRandomValues(Module.HEAPU8.subarray(dataHead, dataTail));
});
#error "TODO: __EMSCRIPTEN__"
#elif defined(_WIN32)
#   define TC__RANDOM_OS_CRYPTO_WINDOWS
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <windows.h>
#   ifdef TCRAND_WINDOWS_XP_SUPPORT
#       define TC__RANDOM_OS_CRYPTO_WINDOWS_CRYPT
#       include <wincrypt.h>
typedef BOOL TCRand_CryptAcquireContextW_t(
    HCRYPTPROV *phProv,
    LPCWSTR    szContainer,
    LPCWSTR    szProvider,
    DWORD      dwProvType,
    DWORD      dwFlags
);
typedef BOOL TCRand_CryptReleaseContext_t(
    HCRYPTPROV hProv,
    DWORD      dwFlags
);
typedef BOOL TCRand_CryptContextAddRef_t(
    HCRYPTPROV hProv,
    DWORD      *pdwReserved,
    DWORD      dwFlags
);
typedef BOOL TCRand_CryptGenRandom_t(
    HCRYPTPROV hProv,
    DWORD      dwLen,
    BYTE       *pbBuffer
);
#   else
#       define TC__RANDOM_OS_CRYPTO_WINDOWS_BCRYPT
#       include <bcrypt.h>
typedef NTSTATUS TCRand_BCryptGenRandom_t(
    BCRYPT_ALG_HANDLE hAlgorithm,
    PUCHAR            pbBuffer,
    ULONG             cbBuffer,
    ULONG             dwFlags
);
#   endif
#elif defined(__linux__)
#   define TC__RANDOM_OS_CRYPTO_DEV_URANDOM
#   include <fcntl.h>
#   include <unistd.h>
#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#   include <sys/param.h>  // TODO: can we rely on this existing?
#   if defined(BSD) || defined(__APPLE__)
#       define TC__RANDOM_OS_CRYPTO_ARC4RANDOM
#   else
#       define TC__RANDOM_OS_CRYPTO_DEV_URANDOM
#       include <fcntl.h>
#       include <unistd.h>
#   endif
#else
#   define TC__RANDOM_OS_CRYPTO_NULL    // none available
#endif

#ifndef TC_MALLOC
#define TC_MALLOC(size)         malloc(size)
#endif /* TC_MALLOC */
#ifndef TC_FREE
#define TC_FREE(ptr)            free(ptr)
#endif /* TC_FREE */

#ifndef TC_NO_FMA
#define TC_FMAF(x,y,z)  fmaf(x,y,z)
#define TC_FMA(x,y,z)   fma(x,y,z)
#else
#define TC_FMAF(x,y,z)  ((x)*(y)+(z))
#define TC_FMA(x,y,z)   ((x)*(y)+(z))
#endif /* TC_USE_FMA */

#ifndef TC__STATIC_CAST
#ifdef __cplusplus
#define TC__STATIC_CAST(T,v) static_cast<T>(v)
#else
#define TC__STATIC_CAST(T,v) ((T)(v))
#endif
#endif /* TC__STATIC_CAST */

#ifndef TC__REINTERPRET_CAST
#ifdef __cplusplus
#define TC__REINTERPRET_CAST(T,v) reinterpret_cast<T>(v)
#else
#define TC__REINTERPRET_CAST(T,v) ((T)(v))
#endif
#endif /* TC__REINTERPRET_CAST */

/* no cast done to preserve undefined function warnings in C */
#ifndef TC__VOID_CAST
#ifdef __cplusplus
#define TC__VOID_CAST(T,v)  TC__STATIC_CAST(T,v)
#else
#define TC__VOID_CAST(T,v)  (v)
#endif
#endif /* TC__VOID_CAST */

#ifndef TC_MALLOC_T_
#define TC_MALLOC_T_(T)     TC__VOID_CAST(T*,TC_MALLOC(sizeof(T)))
#endif /* TC_MALLOC_T_ */
#ifndef TC_MALLOC_TADD_
#define TC_MALLOC_TADD_(T,x)    TC__VOID_CAST(T*,TC_MALLOC(sizeof(T) + (x)))
#endif /* TC_MALLOC_TADD_ */

const uint32_t tcrand_minstd0_default_seed[] = { UINT32_C(1) };
const uint32_t tcrand_minstd_default_seed[] = { UINT32_C(1) };
const uint32_t tcrand_mt19937_default_seed[] = { UINT32_C(5489) };
const uint32_t tcrand_mt19937_64_default_seed[] = { UINT32_C(5489), UINT32_C(0) };

static void tcrand_i_free_dealloc(void* data)
{
    TC_FREE(data);
}

#define TCRAND_I_MIN(x,y)   ((x)<(y)?(x):(y))
#define TCRAND_I_MAX(x,y)   ((x)>(y)?(x):(y))

// (x*a) % m
// Schrage's method: http://home.earthlink.net/~pfenerty/pi/schrages_method.html
static uint32_t tcrand_i_schrage32(uint32_t x, uint32_t a, uint32_t m, uint32_t q, uint32_t r)
{
    //uint32_t lhs = a * (x - (x / q) * q);
    uint32_t lhs = a * (x % q);
    uint32_t rhs = r * (x / q);
    return lhs > rhs ? lhs - rhs : lhs + m - rhs;
}
static uint64_t tcrand_i_schrage64(uint64_t x, uint64_t a, uint64_t m, uint64_t q, uint64_t r)
{
    //uint64_t lhs = a * (x - (x / q) * q);
    uint64_t lhs = a * (x % q);
    uint64_t rhs = r * (x / q);
    return lhs > rhs ? lhs - rhs : lhs + m - rhs;
}

static uint64_t tcrand_i_rotl64(uint64_t x, uint64_t k)
{
    return (x << k) | (x >> (64 - k));
}

#ifndef TC__RANDOM_OS_CRYPTO_NULL
#define TCRAND_I_OS_CRYPTO_BUFSIZE   32
struct TC_I_RandGen_OS_CryptoData
{
    unsigned char value_buf[TCRAND_I_OS_CRYPTO_BUFSIZE];
#if defined(TC__RANDOM_OS_CRYPTO_WEB_CRYPTO)
    /* nothing */
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_CRYPT)
    HMODULE lib_advapi32;
    TCRand_CryptAcquireContextW_t* CryptAcquireContextW;
    TCRand_CryptReleaseContext_t* CryptReleaseContext;
    TCRand_CryptContextAddRef_t* CryptContextAddRef;
    TCRand_CryptGenRandom_t* CryptGenRandom;
    HCRYPTPROV crypt_prov;
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_BCRYPT)
    HMODULE lib_bcrypt;
    TCRand_BCryptGenRandom_t* BCryptGenRandom;
#elif defined(TC__RANDOM_OS_CRYPTO_ARC4RANDOM)
    /* nothing */
#elif defined(TC__RANDOM_OS_CRYPTO_DEV_URANDOM)
    int fd_dev_urandom;
#else
#   error "Unhandled OS crypto engine"
#endif
};
static struct TC_I_RandGen_OS_CryptoData* tcrand_i_os_crypto_init(struct TC_I_RandGen_OS_CryptoData* osdata)
{
#if defined(TC__RANDOM_OS_CRYPTO_WEB_CRYPTO)
    /* no-op */
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_CRYPT)
    if(!(osdata->lib_advapi32 = LoadLibraryW(L"Advapi32.dll")))
        return NULL;
    if(!(osdata->CryptAcquireContextW = TC__REINTERPRET_CAST(TCRand_CryptAcquireContextW_t*,GetProcAddress(osdata->lib_advapi32, "CryptAcquireContextW")))
    || !(osdata->CryptReleaseContext = TC__REINTERPRET_CAST(TCRand_CryptReleaseContext_t*,GetProcAddress(osdata->lib_advapi32, "CryptReleaseContext")))
    || !(osdata->CryptContextAddRef = TC__REINTERPRET_CAST(TCRand_CryptContextAddRef_t*,GetProcAddress(osdata->lib_advapi32, "CryptContextAddRef")))
    || !(osdata->CryptGenRandom = TC__REINTERPRET_CAST(TCRand_CryptGenRandom_t*,GetProcAddress(osdata->lib_advapi32, "CryptGenRandom"))))
    {
        FreeLibrary(osdata->lib_advapi32);
        return NULL;
    }
    if(!osdata->CryptAcquireContextW(&osdata->crypt_prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        FreeLibrary(osdata->lib_advapi32);
        return NULL;
    }
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_BCRYPT)
    if(!(osdata->lib_bcrypt = LoadLibraryW(L"Bcrypt.dll")))
        return NULL;
    if(!(osdata->BCryptGenRandom = TC__REINTERPRET_CAST(TCRand_BCryptGenRandom_t*,GetProcAddress(osdata->lib_bcrypt, "BCryptGenRandom"))))
    {
        FreeLibrary(osdata->lib_bcrypt);
        return NULL;
    }
#elif defined(TC__RANDOM_OS_CRYPTO_ARC4RANDOM)
    /* no-op */
#elif defined(TC__RANDOM_OS_CRYPTO_DEV_URANDOM)
    if((osdata->fd_dev_urandom = open("/dev/urandom", O_RDONLY)) < 0)
        return NULL;
#else
#   error "Unhandled OS crypto engine"
#endif

    return osdata;
}
static void tcrand_i_os_crypto_next(void* data, void* value)
{
    struct TC_I_RandGen_OS_CryptoData* osdata = TC__VOID_CAST(struct TC_I_RandGen_OS_CryptoData*,data);

#if defined(TC__RANDOM_OS_CRYPTO_WEB_CRYPTO)
    (void)osdata;
    tcrand_i_Crypto_getRandomValues(value, TC__STATIC_CAST(char*,value) + TCRAND_I_OS_CRYPTO_BUFSIZE);
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_CRYPT)
    BOOL ok = osdata->CryptGenRandom(osdata->crypt_prov, TCRAND_I_OS_CRYPTO_BUFSIZE, value);
    (void)ok; assert(ok);
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_BCRYPT)
    NTSTATUS status = osdata->BCryptGenRandom(NULL, value, TCRAND_I_OS_CRYPTO_BUFSIZE, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    (void)status; assert(!status);
#elif defined(TC__RANDOM_OS_CRYPTO_ARC4RANDOM)
    arc4random_buf(value, TCRAND_I_OS_CRYPTO_BUFSIZE);
#elif defined(TC__RANDOM_OS_CRYPTO_DEV_URANDOM)
    ssize_t len = read(osdata->fd_dev_urandom, value, TCRAND_I_OS_CRYPTO_BUFSIZE);
    (void)len; assert(len == TCRAND_I_OS_CRYPTO_BUFSIZE);
#else
#   error "Unhandled OS crypto engine"
#endif
}
static TC_RandGen* tcrand_i_os_crypto_clone(void* data, TC_RandGen* ngen)
{
    struct TC_I_RandGen_OS_CryptoData* osdata = TC__VOID_CAST(struct TC_I_RandGen_OS_CryptoData*,data);
    struct TC_I_RandGen_OS_CryptoData* nosdata = TC_MALLOC_T_(struct TC_I_RandGen_OS_CryptoData);
    if(!nosdata)
        return NULL;

    *nosdata = *osdata;

#if defined(TC__RANDOM_OS_CRYPTO_WEB_CRYPTO)
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_CRYPT)
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, TC__REINTERPRET_CAST(LPCWSTR,osdata->lib_advapi32), &nosdata->lib_advapi32))
    {
        TC_FREE(nosdata);
        return NULL;
    }
    if(!osdata->CryptContextAddRef(osdata->crypt_prov, NULL, 0))
    {
        FreeLibrary(nosdata->lib_advapi32);
        TC_FREE(nosdata);
        return NULL;
    }
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_BCRYPT)
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, TC__REINTERPRET_CAST(LPCWSTR,osdata->lib_bcrypt), &nosdata->lib_bcrypt))
    {
        TC_FREE(nosdata);
        return NULL;
    }
#elif defined(TC__RANDOM_OS_CRYPTO_ARC4RANDOM)
    /* no-op */
#elif defined(TC__RANDOM_OS_CRYPTO_DEV_URANDOM)
    if((nosdata->fd_dev_urandom = dup(osdata->fd_dev_urandom)) < 0)
    {
        TC_FREE(nosdata);
        return NULL;
    }
#else
#   error "Unhandled OS crypto engine"
#endif

    ngen->data = nosdata;
    ngen->value_ptr = nosdata->value_buf;
    return ngen;
}
static void tcrand_i_os_crypto_dealloc(void* data)
{
    struct TC_I_RandGen_OS_CryptoData* osdata = TC__VOID_CAST(struct TC_I_RandGen_OS_CryptoData*,data);

#if defined(TC__RANDOM_OS_CRYPTO_WEB_CRYPTO)
    /* no-op */
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_CRYPT)
    osdata->CryptReleaseContext(osdata->crypt_prov, 0);
    FreeLibrary(osdata->lib_advapi32);
#elif defined(TC__RANDOM_OS_CRYPTO_WINDOWS_BCRYPT)
    FreeLibrary(osdata->lib_bcrypt);
#elif defined(TC__RANDOM_OS_CRYPTO_ARC4RANDOM)
    /* no-op */
#elif defined(TC__RANDOM_OS_CRYPTO_DEV_URANDOM)
    close(osdata->fd_dev_urandom);
#else
#   error "Unhandled OS crypto engine"
#endif

    TC_FREE(osdata);
}
#endif


struct TC_I_RandGen_LCGData
{
    uint32_t seed_buf[1];
    uint32_t value_buf[1];
    // x = (x*a+c)%m
    uint32_t x, a, c, m;
    uint32_t q, r;
};
static void tcrand_i_lcg_seed(void* data, const void* seed)
{
    struct TC_I_RandGen_LCGData* lcgdata = TC__VOID_CAST(struct TC_I_RandGen_LCGData*,data);
    lcgdata->x = *TC__STATIC_CAST(uint32_t*,seed);
}
static void tcrand_i_lcg_next(void* data, void* value)
{
    struct TC_I_RandGen_LCGData* lcgdata = TC__VOID_CAST(struct TC_I_RandGen_LCGData*,data);

    uint32_t x = lcgdata->x;
    uint32_t a = lcgdata->a, c = lcgdata->c, m = lcgdata->m;
    uint32_t q = lcgdata->q, r = lcgdata->r;

    //x = ((uint64_t)x * a + c) % m;
    x = tcrand_i_schrage32(x, a, m, q, r) + c % m;

    *TC__STATIC_CAST(uint32_t*,value) = x;
    lcgdata->x = x;
}
static TC_RandGen* tcrand_i_lcg_clone(void* data, TC_RandGen* ngen)
{
    struct TC_I_RandGen_LCGData* lcgdata = TC__VOID_CAST(struct TC_I_RandGen_LCGData*,data);
    struct TC_I_RandGen_LCGData* nlcgdata = TC_MALLOC_T_(struct TC_I_RandGen_LCGData);
    *nlcgdata = *lcgdata;
    ngen->data = nlcgdata;
    ngen->seed_ptr = nlcgdata->seed_buf;
    ngen->value_ptr = nlcgdata->value_buf;
    return ngen;
}

struct TC_I_RandGen_LCG64Data
{
    uint32_t seed_buf[2];
    uint64_t value_buf[1];
    // x = (x*a+c)%m
    uint64_t x, a, c, m;
    uint64_t q, r;
};
static void tcrand_i_lcg_64_seed(void* data, const void* seed)
{
    struct TC_I_RandGen_LCG64Data* lcgdata = TC__VOID_CAST(struct TC_I_RandGen_LCG64Data*,data);
    lcgdata->x = *TC__STATIC_CAST(uint64_t*,seed);
}
static void tcrand_i_lcg_64_next(void* data, void* value)
{
    struct TC_I_RandGen_LCG64Data* lcgdata = TC__VOID_CAST(struct TC_I_RandGen_LCG64Data*,data);

    uint64_t x = lcgdata->x;
    uint64_t a = lcgdata->a, c = lcgdata->c, m = lcgdata->m;
    uint64_t q = lcgdata->q, r = lcgdata->r;

    x = tcrand_i_schrage64(x, a, m, q, r) + c % m;

    *TC__STATIC_CAST(uint64_t*,value) = x;
    lcgdata->x = x;
}
static TC_RandGen* tcrand_i_lcg_64_clone(void* data, TC_RandGen* ngen)
{
    struct TC_I_RandGen_LCG64Data* lcgdata = TC__VOID_CAST(struct TC_I_RandGen_LCG64Data*,data);
    struct TC_I_RandGen_LCG64Data* nlcgdata = TC_MALLOC_T_(struct TC_I_RandGen_LCG64Data);
    *nlcgdata = *lcgdata;
    ngen->data = nlcgdata;
    ngen->seed_ptr = nlcgdata->seed_buf;
    ngen->value_ptr = nlcgdata->value_buf;
    return ngen;
}

struct TC_I_RandGen_WELLData
{
    uint32_t index;
    uint32_t value_buf[1];
    size_t statelen;
    uint32_t state[1];
};
static void tcrand_i_well_seed(void* data, const void* seed)
{
    struct TC_I_RandGen_WELLData* wdata = TC__VOID_CAST(struct TC_I_RandGen_WELLData*,data);

    wdata->index = 0;
    memmove(wdata->state, seed, wdata->statelen * sizeof(*wdata->state));
}
// http://lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf
static void tcrand_i_well_next(void* data, void* value)
{
    struct TC_I_RandGen_WELLData* wdata = TC__VOID_CAST(struct TC_I_RandGen_WELLData*,data);

    uint32_t* state = wdata->state;
    uint32_t index = wdata->index;
    uint32_t statelen = wdata->statelen;
    uint32_t a, b, c, d;
    a = state[index];
    c = state[(index+13)%statelen];
    b = a ^ c ^ (a << 16) ^ (c << 15);
    c = state[(index+9)%statelen];
    c ^= c >> 11;
    a = state[index] = b ^ c;
    d = a ^ ((a << 5) & UINT32_C(0xDA442D24));
    index = (index + statelen - 1) % statelen;
    a = state[index];
    state[index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
    wdata->index = index;
    *TC__STATIC_CAST(uint32_t*,value) = state[index];
}
static TC_RandGen* tcrand_i_well_clone(void* data, TC_RandGen* ngen)
{
    struct TC_I_RandGen_WELLData* wdata = TC__VOID_CAST(struct TC_I_RandGen_WELLData*,data);
    struct TC_I_RandGen_WELLData* nwdata = TC_MALLOC_TADD_(struct TC_I_RandGen_WELLData,(wdata->statelen - 1) * sizeof(*wdata->state));
    *nwdata = *wdata;
    ngen->data = nwdata;
    ngen->seed_ptr = nwdata->state;
    ngen->value_ptr = nwdata->value_buf;
    return ngen;
}

struct TC_I_RandGen_XorOshiRo128PlusData
{
    uint64_t state[2];
    uint64_t seed_buf[2];
    uint64_t value_buf[1];
};
static void tcrand_i_xoroshiro128plus_seed(void* data, const void* seed)
{
    struct TC_I_RandGen_XorOshiRo128PlusData* xdata = TC__VOID_CAST(struct TC_I_RandGen_XorOshiRo128PlusData*,data);
    memmove(xdata->state, seed, sizeof(xdata->state));
}
// http://xoroshiro.di.unimi.it/xoroshiro128plus.c
static void tcrand_i_xoroshiro128plus_next(void* data, void* value)
{
    struct TC_I_RandGen_XorOshiRo128PlusData* xdata = TC__VOID_CAST(struct TC_I_RandGen_XorOshiRo128PlusData*,data);

    const uint64_t s0 = xdata->state[0];
    uint64_t s1 = xdata->state[1];

    *TC__STATIC_CAST(uint64_t*,value) = s0 + s1;

    s1 ^= s0;
    xdata->state[0] = tcrand_i_rotl64(s0, 55) ^ s1 ^ (s1 << 14); /* a, b */
    xdata->state[1] = tcrand_i_rotl64(s1, 36); /* c */
}
static TC_RandGen* tcrand_i_xoroshiro128plus_clone(void* data, TC_RandGen* ngen)
{
    struct TC_I_RandGen_XorOshiRo128PlusData* xdata = TC__VOID_CAST(struct TC_I_RandGen_XorOshiRo128PlusData*,data);
    struct TC_I_RandGen_XorOshiRo128PlusData* nxdata = TC_MALLOC_T_(struct TC_I_RandGen_XorOshiRo128PlusData);
    *nxdata = *xdata;
    ngen->data = nxdata;
    ngen->seed_ptr = nxdata->seed_buf;
    ngen->value_ptr = nxdata->value_buf;
    return ngen;
}

struct TC_I_RandGen_MTData
{
    uint32_t mt[624];
    uint32_t seed_buf[1];
    uint32_t value_buf[1];
    unsigned short index;
};
static void tcrand_i_mt19937_seed(void* data, const void* seed)
{
    struct TC_I_RandGen_MTData* mtdata = TC__VOID_CAST(struct TC_I_RandGen_MTData*,data);

    mtdata->mt[0] = *TC__STATIC_CAST(uint32_t*,seed);
    unsigned short i;
    for(i = 1; i < sizeof(mtdata->mt) / sizeof(*mtdata->mt); i++)
        mtdata->mt[i] = UINT32_C(0x6C078965) * (mtdata->mt[i-1] ^ (mtdata->mt[i-1] >> 30)) + i;
    mtdata->index = sizeof(mtdata->mt) / sizeof(*mtdata->mt);
}
static void tcrand_i_mt19937_twist(struct TC_I_RandGen_MTData* mtdata)
{
    uint32_t x, xA;

    unsigned short i;
    for(i = 0; i < sizeof(mtdata->mt) / sizeof(*mtdata->mt); i++)
    {
        x = (mtdata->mt[i] & ~UINT32_C(0x7FFFFFFF)) + (mtdata->mt[(i+1) % (sizeof(mtdata->mt) / sizeof(*mtdata->mt))] & UINT32_C(0x7FFFFFFF));
        xA = x >> 1;
        if(x & 1) xA ^= UINT32_C(0x9908B0DF);
        mtdata->mt[i] = mtdata->mt[(i + 397) % (sizeof(mtdata->mt) / sizeof(*mtdata->mt))] ^ xA;
    }
    mtdata->index = 0;
}
static void tcrand_i_mt19937_next(void* data, void* value)
{
    struct TC_I_RandGen_MTData* mtdata = TC__VOID_CAST(struct TC_I_RandGen_MTData*,data);

    if(mtdata->index >= sizeof(mtdata->mt) / sizeof(*mtdata->mt))
        tcrand_i_mt19937_twist(mtdata);

    uint32_t y = mtdata->mt[mtdata->index++];

    y ^= (y >> 11);
    y ^= (y << 7) & UINT32_C(0x9D2C5680);
    y ^= (y << 15) & UINT32_C(0xEFC60000);
    y ^= (y >> 18);

    *TC__STATIC_CAST(uint32_t*,value) = y;
}
static TC_RandGen* tcrand_i_mt19937_clone(void* data, TC_RandGen* ngen)
{
    struct TC_I_RandGen_MTData* mtdata = TC__VOID_CAST(struct TC_I_RandGen_MTData*,data);
    struct TC_I_RandGen_MTData* nmtdata = TC_MALLOC_T_(struct TC_I_RandGen_MTData);
    *nmtdata = *mtdata;
    ngen->data = nmtdata;
    ngen->seed_ptr = nmtdata->seed_buf;
    ngen->value_ptr = nmtdata->value_buf;
    return ngen;
}

struct TC_I_RandGen_MT64Data
{
    uint64_t mt[312];
    uint64_t seed_buf[1];
    uint64_t value_buf[1];
    unsigned short index;
};
static void tcrand_i_mt19937_64_seed(void* data, const void* seed)
{
    struct TC_I_RandGen_MT64Data* mtdata = TC__VOID_CAST(struct TC_I_RandGen_MT64Data*,data);

    mtdata->mt[0] = *TC__STATIC_CAST(uint64_t*,seed);
    unsigned short i;
    for(i = 1; i < sizeof(mtdata->mt) / sizeof(*mtdata->mt); i++)
        mtdata->mt[i] = UINT64_C(0x5851F42D4C957F2D) * (mtdata->mt[i-1] ^ (mtdata->mt[i-1] >> 62)) + i;
    mtdata->index = sizeof(mtdata->mt) / sizeof(*mtdata->mt);
}
static void tcrand_i_mt19937_64_twist(struct TC_I_RandGen_MT64Data* mtdata)
{
    uint64_t x, xA;

    unsigned short i;
    for(i = 0; i < sizeof(mtdata->mt) / sizeof(*mtdata->mt); i++)
    {
        x = (mtdata->mt[i] & ~UINT64_C(0x7FFFFFFF)) + (mtdata->mt[(i+1) % (sizeof(mtdata->mt) / sizeof(*mtdata->mt))] & UINT64_C(0x7FFFFFFF));
        xA = x >> 1;
        if(x & 1) xA ^= UINT64_C(0xB5026F5AA96619E9);
        mtdata->mt[i] = mtdata->mt[(i + 156) % (sizeof(mtdata->mt) / sizeof(*mtdata->mt))] ^ xA;
    }
    mtdata->index = 0;
}
static void tcrand_i_mt19937_64_next(void* data, void* value)
{
    struct TC_I_RandGen_MT64Data* mtdata = TC__VOID_CAST(struct TC_I_RandGen_MT64Data*,data);

    if(mtdata->index >= sizeof(mtdata->mt) / sizeof(*mtdata->mt))
        tcrand_i_mt19937_64_twist(mtdata);

    uint64_t y = mtdata->mt[mtdata->index++];

    y ^= (y >> 29) & UINT64_C(0x5555555555555555);
    y ^= (y << 17) & UINT64_C(0x71D67FFFEDA60000);
    y ^= (y << 37) & UINT64_C(0xFFF7EEE000000000);
    y ^= (y >> 43);

    *TC__STATIC_CAST(uint64_t*,value) = y;
}
static TC_RandGen* tcrand_i_mt19937_64_clone(void* data, TC_RandGen* ngen)
{
    struct TC_I_RandGen_MT64Data* mtdata = TC__VOID_CAST(struct TC_I_RandGen_MT64Data*,data);
    struct TC_I_RandGen_MT64Data* nmtdata = TC_MALLOC_T_(struct TC_I_RandGen_MT64Data);
    *nmtdata = *mtdata;
    ngen->data = nmtdata;
    ngen->seed_ptr = nmtdata->seed_buf;
    ngen->value_ptr = nmtdata->value_buf;
    return ngen;
}

struct TC_I_RandGen_SplitMix64Data
{
    uint64_t state;
    uint64_t value_buf[1];
};
static void tcrand_i_splitmix64_seed(void* data, const void* seed)
{
    struct TC_I_RandGen_SplitMix64Data* smdata = TC__VOID_CAST(struct TC_I_RandGen_SplitMix64Data*,data);
    smdata->state = *TC__STATIC_CAST(uint64_t*,seed);
}
// http://xoroshiro.di.unimi.it/splitmix64.c
static void tcrand_i_splitmix64_next(void* data, void* value)
{
    struct TC_I_RandGen_SplitMix64Data* smdata = TC__VOID_CAST(struct TC_I_RandGen_SplitMix64Data*,data);

    uint64_t z = (smdata->state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    *TC__STATIC_CAST(uint64_t*,value) = z ^ (z >> 31);
}
static TC_RandGen* tcrand_i_splitmix64_clone(void* data, TC_RandGen* ngen)
{
    struct TC_I_RandGen_SplitMix64Data* smdata = TC__VOID_CAST(struct TC_I_RandGen_SplitMix64Data*,data);
    struct TC_I_RandGen_SplitMix64Data* nsmdata = TC_MALLOC_T_(struct TC_I_RandGen_SplitMix64Data);
    *nsmdata = *smdata;
    ngen->data = nsmdata;
    ngen->seed_ptr = &nsmdata->state;
    ngen->value_ptr = nsmdata->value_buf;
    return ngen;
}

TC_RandGen* tcrand_init_os_crypto(TC_RandGen* rgen)
{
#ifndef TC__RANDOM_OS_CRYPTO_NULL
    if(!rgen) return NULL;

    struct TC_I_RandGen_OS_CryptoData* osdata = TC_MALLOC_T_(struct TC_I_RandGen_OS_CryptoData);
    if(!osdata) return NULL;

    if(!tcrand_i_os_crypto_init(osdata))
    {
        TC_FREE(osdata);
        return NULL;
    }

    *rgen = (TC_RandGen){
        .seed = NULL,
        .next = tcrand_i_os_crypto_next,
        .clone = tcrand_i_os_crypto_clone,
        .dealloc = tcrand_i_os_crypto_dealloc,
        .data = osdata,
        .seed_len = 0,
        .seed_ptr = NULL,
        .value_blen = sizeof(osdata->value_buf),
        .value_alen = 0,
        .value_ptr = osdata->value_buf,
    };

    return rgen;
#else
    return NULL;    // no crypto engine => always fail
#endif
}

TC_RandGen* tcrand_init_lcg32(TC_RandGen* rgen, uint32_t a, uint32_t c, uint32_t m)
{
    if(!rgen) return NULL;
    if(m < 0xFF) return NULL;
    struct TC_I_RandGen_LCGData* lcgdata = TC_MALLOC_T_(struct TC_I_RandGen_LCGData);
    lcgdata->a = a;
    lcgdata->c = c;
    lcgdata->m = m;
    lcgdata->q = m / a;
    lcgdata->r = m % a;
    rgen->seed = tcrand_i_lcg_seed;
    rgen->next = tcrand_i_lcg_next;
    rgen->clone = tcrand_i_lcg_clone;
    rgen->dealloc = tcrand_i_free_dealloc;
    rgen->data = lcgdata;
    rgen->seed_len = sizeof(lcgdata->seed_buf);
    rgen->seed_ptr = lcgdata->seed_buf;
    rgen->value_blen = sizeof(lcgdata->value_buf);
    unsigned char alen = 0;
    while((m >>= 8) >= 0xFF) alen++;
    rgen->value_alen = alen;
    rgen->value_ptr = lcgdata->value_buf;
    return rgen;
}
TC_RandGen* tcrand_init_lcg64(TC_RandGen* rgen, uint64_t a, uint64_t c, uint64_t m)
{
    if(!rgen) return NULL;
    if(m < 0xFF) return NULL;
    struct TC_I_RandGen_LCG64Data* lcgdata = TC_MALLOC_T_(struct TC_I_RandGen_LCG64Data);
    lcgdata->a = a;
    lcgdata->c = c;
    lcgdata->m = m;
    lcgdata->q = m / a;
    lcgdata->r = m % a;
    rgen->seed = tcrand_i_lcg_64_seed;
    rgen->next = tcrand_i_lcg_64_next;
    rgen->clone = tcrand_i_lcg_64_clone;
    rgen->dealloc = tcrand_i_free_dealloc;
    rgen->data = lcgdata;
    rgen->seed_len = sizeof(lcgdata->seed_buf);
    rgen->seed_ptr = lcgdata->seed_buf;
    rgen->value_blen = sizeof(lcgdata->value_buf);
    unsigned char alen = 0;
    while((m >>= 8) >= 0xFF) alen++;
    rgen->value_alen = alen;
    rgen->value_ptr = lcgdata->value_buf;
    return rgen;
}
TC_RandGen* tcrand_init_well(TC_RandGen* rgen, size_t statelen)
{
    if(!rgen) return NULL;
    struct TC_I_RandGen_WELLData* wdata = TC_MALLOC_TADD_(struct TC_I_RandGen_WELLData,(statelen - 1) * sizeof(*wdata->state));
    rgen->seed = tcrand_i_well_seed;
    rgen->next = tcrand_i_well_next;
    rgen->clone = tcrand_i_well_clone;
    rgen->dealloc = tcrand_i_free_dealloc;
    rgen->data = wdata;
    rgen->seed_len = statelen * sizeof(*wdata->state);
    rgen->seed_ptr = wdata->state;
    rgen->value_blen = sizeof(wdata->value_buf);
    rgen->value_alen = sizeof(uint32_t);
    rgen->value_ptr = wdata->value_buf;
    return rgen;
}

TC_RandGen* tcrand_init_minstd0(TC_RandGen* rgen)
{
    return tcrand_init_lcg32(rgen, UINT32_C(16807), UINT32_C(0), UINT32_C(0x7FFFFFFF));
}
TC_RandGen* tcrand_init_minstd(TC_RandGen* rgen)
{
    return tcrand_init_lcg32(rgen, UINT32_C(48271), UINT32_C(0), UINT32_C(0x7FFFFFFF));
}
TC_RandGen* tcrand_init_well512(TC_RandGen* rgen)
{
    return tcrand_init_well(rgen, 16);
}
// http://xoroshiro.di.unimi.it/xoroshiro128plus.c
TC_RandGen* tcrand_init_xoroshiro128plus(TC_RandGen* rgen)
{
    if(!rgen) return NULL;
    struct TC_I_RandGen_XorOshiRo128PlusData* xdata = TC_MALLOC_T_(struct TC_I_RandGen_XorOshiRo128PlusData);
    rgen->seed = tcrand_i_xoroshiro128plus_seed;
    rgen->next = tcrand_i_xoroshiro128plus_next;
    rgen->clone = tcrand_i_xoroshiro128plus_clone;
    rgen->dealloc = tcrand_i_free_dealloc;
    rgen->data = xdata;
    rgen->seed_len = sizeof(xdata->seed_buf);
    rgen->seed_ptr = xdata->seed_buf;
    rgen->value_blen = sizeof(xdata->value_buf);
    rgen->value_alen = sizeof(uint64_t);
    rgen->value_ptr = xdata->value_buf;
    return rgen;
}
TC_RandGen* tcrand_init_mt19937(TC_RandGen* rgen)
{
    if(!rgen) return NULL;
    struct TC_I_RandGen_MTData* mtdata = TC_MALLOC_T_(struct TC_I_RandGen_MTData);
    rgen->seed = tcrand_i_mt19937_seed;
    rgen->next = tcrand_i_mt19937_next;
    rgen->clone = tcrand_i_mt19937_clone;
    rgen->dealloc = tcrand_i_free_dealloc;
    rgen->data = mtdata;
    rgen->seed_len = sizeof(mtdata->seed_buf);
    rgen->seed_ptr = mtdata->seed_buf;
    rgen->value_blen = sizeof(mtdata->value_buf);
    rgen->value_alen = sizeof(uint32_t);
    rgen->value_ptr = mtdata->value_buf;
    return rgen;
}
TC_RandGen* tcrand_init_mt19937_64(TC_RandGen* rgen)
{
    if(!rgen) return NULL;
    struct TC_I_RandGen_MT64Data* mtdata = TC_MALLOC_T_(struct TC_I_RandGen_MT64Data);
    rgen->seed = tcrand_i_mt19937_64_seed;
    rgen->next = tcrand_i_mt19937_64_next;
    rgen->clone = tcrand_i_mt19937_64_clone;
    rgen->dealloc = tcrand_i_free_dealloc;
    rgen->data = mtdata;
    rgen->seed_len = sizeof(mtdata->seed_buf);
    rgen->seed_ptr = mtdata->seed_buf;
    rgen->value_blen = sizeof(mtdata->value_buf);
    rgen->value_alen = sizeof(uint64_t);
    rgen->value_ptr = mtdata->value_buf;
    return rgen;
}
TC_RandGen* tcrand_init_splitmix64(TC_RandGen* rgen)
{
    if(!rgen) return NULL;
    struct TC_I_RandGen_SplitMix64Data* smdata = TC_MALLOC_T_(struct TC_I_RandGen_SplitMix64Data);
    rgen->seed = tcrand_i_splitmix64_seed;
    rgen->next = tcrand_i_splitmix64_next;
    rgen->clone = tcrand_i_splitmix64_clone;
    rgen->dealloc = tcrand_i_free_dealloc;
    rgen->data = smdata;
    rgen->seed_len = sizeof(smdata->state);
    rgen->seed_ptr = &smdata->state;
    rgen->value_blen = sizeof(smdata->value_buf);
    rgen->value_alen = sizeof(uint64_t);
    rgen->value_ptr = smdata->value_buf;
    return rgen;
}
TC_RandGen* tcrand_clone(TC_RandGen* ngen, const TC_RandGen* rgen)
{
    if(!ngen || !rgen) return NULL;
    if(!rgen->clone) return NULL;
    *ngen = *rgen;
    return rgen->clone(rgen->data, ngen);
}
void tcrand_deinit(TC_RandGen* rgen)
{
    if(!rgen) return;
    if(rgen->dealloc) rgen->dealloc(rgen->data);
}

// https://stackoverflow.com/questions/33282662/is-the-algorithm-behind-stdseed-seq-defined
static uint32_t tcrand_i_gen_seedseq_T(uint32_t x)
{
    return x ^ (x >> 27);
}
void tcrand_gen_seedseq(uint32_t* seq, size_t seqlen, const uint32_t* seed, size_t seedlen)
{
    uint32_t n = seqlen;
    uint32_t s = seedlen;

    for(uint32_t i = 0; i < n; i++)
        seq[i] = UINT32_C(0x8B8B8B8B);

    uint32_t t = (n >= 623) ? 11 : (n >= 68) ? 7 : (n >= 39) ? 5 : (n >= 7) ? 3 : (n - 1) / 2;

    uint32_t p = (n - t) / 2;
    uint32_t q = p + t;

    uint32_t m = TCRAND_I_MAX(s+1,n);
    for(uint32_t k = 0; k < m; k++)
    {
        uint32_t r1 = UINT32_C(1664525) * tcrand_i_gen_seedseq_T(seq[k%n] ^ seq[(k+p)%n] ^ seq[(k+n-1)%n]);
        uint32_t r2 = r1 + (k == 0 ? s : k <= s ? k%n + seed[k-1] : k%n);

        seq[(k+p)%n] += r1;
        seq[(k+q)%n] += r2;
        seq[k%n] = r2;
    }
    for(uint32_t k = m; k < m+n; k++)
    {
        uint32_t r3 = UINT32_C(1566083941) * tcrand_i_gen_seedseq_T(seq[k%n] + seq[(k+p)%n] + seq[(k+n-1)%n]);
        uint32_t r4 = r3 - k%n;

        seq[(k+p)%n] ^= r3;
        seq[(k+q)%n] ^= r4;
        seq[k%n] = r4;
    }
}

void tcrand_seed_raw(TC_RandGen* rgen, const uint32_t* seed)
{
    if(rgen->seed) rgen->seed(rgen->data, seed);
}
void tcrand_seed_u32seq(TC_RandGen* rgen, const uint32_t* seed, size_t slen)
{
    assert(!(rgen->seed_len & (sizeof(uint32_t) - 1)));
    tcrand_gen_seedseq(rgen->seed_ptr, rgen->seed_len / sizeof(uint32_t), seed, slen);
    tcrand_seed_raw(rgen, rgen->seed_ptr);
}
void tcrand_seed_u32(TC_RandGen* rgen, uint32_t seed)
{
    tcrand_seed_u32seq(rgen, &seed, 1);
}

void tcrand_next_raw(TC_RandGen* rgen, void* values)
{
    rgen->next(rgen->data, values);
    //rgen->next(rgen->data, rgen->value_ptr);
    //memcpy(values, rgen->value_ptr, rgen->value_blen);
}
void tcrand_next_bytes(TC_RandGen* rgen, void* bytes, size_t nbytes)
{
    unsigned char* ubytes = TC__VOID_CAST(unsigned char*,bytes);

    while(nbytes)
    {
        tcrand_next_raw(rgen, rgen->value_ptr);
        size_t ubytesn = TCRAND_I_MIN(nbytes,rgen->value_alen);
        memcpy(ubytes, rgen->value_ptr, ubytesn);
        nbytes -= ubytesn;
    }
}

// source https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static uint32_t tcrand_i_next_pow2_u32(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}
static uint64_t tcrand_i_next_pow2_u64(uint64_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return v + 1;
}
uint32_t tcrand_next_uniform_u32(TC_RandGen* rgen, uint32_t min, uint32_t max)
{
    uint32_t num;
    max -= min;
    if(max == UINT32_MAX)
    {
        tcrand_next_bytes(rgen, &num, sizeof(num));
        return min + num;
    }
    uint32_t mask = tcrand_i_next_pow2_u32(max + 1U) - 1U;
    do
    {
        tcrand_next_bytes(rgen, &num, sizeof(num));
        num &= mask;
    }
    while(num > max);
    return min + num;
}
uint64_t tcrand_next_uniform_u64(TC_RandGen* rgen, uint64_t min, uint64_t max)
{
    uint64_t num;
    max -= min;
    if(max == UINT64_MAX)
    {
        tcrand_next_bytes(rgen, &num, sizeof(num));
        return min + num;
    }
    uint64_t mask = tcrand_i_next_pow2_u64(max + 1U) - 1U;
    do
    {
        tcrand_next_bytes(rgen, &num, sizeof(num));
        num &= mask;
    }
    while(num > max);
    return min + num;
}
int32_t tcrand_next_uniform_i32(TC_RandGen* rgen, int32_t min, int32_t max);
int64_t tcrand_next_uniform_i64(TC_RandGen* rgen, int64_t min, int64_t max);
// http://www.doornik.com/research/randomdouble.pdf
#define TCRAND_I_MAX24      (UINT32_C(1) << 24)
#define TCRAND_I_MAX53      (UINT64_C(1) << 53)
#define TCRAND_I_INVM24     5.9604644775390625e-8f
#define TCRAND_I_INVM53     1.1102230246251565404236316680908203125e-16
#define TCRAND_I_INVM24M1   5.9604648328104515558750364705941957589504575103794044e-8f
#define TCRAND_I_INVM53M1   1.1102230246251566636831481088739285926379039974785759e-16
static float tcrand_i_lerp_f(float t, float a, float b) { return TC_FMAF(t, b-a, a); }
static double tcrand_i_lerp_d(double t, double a, double b) { return TC_FMA(t, b-a, a); }
float tcrand_next_uniform_f_oo(TC_RandGen* rgen, float min, float max)
{
    return tcrand_i_lerp_f(tcrand_next_uniform_u32(rgen, 1, TCRAND_I_MAX24 - 1U) * TCRAND_I_INVM24, min, max);
}
double tcrand_next_uniform_d_oo(TC_RandGen* rgen, double min, double max)
{
    return tcrand_i_lerp_d(tcrand_next_uniform_u64(rgen, 1, TCRAND_I_MAX53 - 1U) * TCRAND_I_INVM53, min, max);
}
float tcrand_next_uniform_f_co(TC_RandGen* rgen, float min, float max)
{
    return tcrand_i_lerp_f(tcrand_next_uniform_u32(rgen, 0, TCRAND_I_MAX24 - 1U) * TCRAND_I_INVM24, min, max);
}
double tcrand_next_uniform_d_co(TC_RandGen* rgen, double min, double max)
{
    return tcrand_i_lerp_d(tcrand_next_uniform_u64(rgen, 0, TCRAND_I_MAX53 - 1U) * TCRAND_I_INVM53, min, max);
}
float tcrand_next_uniform_f_oc(TC_RandGen* rgen, float min, float max)
{
    return tcrand_i_lerp_f(1.0f - tcrand_next_uniform_u32(rgen, 0, TCRAND_I_MAX24 - 1U) * TCRAND_I_INVM24, min, max);
}
double tcrand_next_uniform_d_oc(TC_RandGen* rgen, double min, double max)
{
    return tcrand_i_lerp_d(1.0 - tcrand_next_uniform_u64(rgen, 0, TCRAND_I_MAX53 - 1U) * TCRAND_I_INVM53, min, max);
}
float tcrand_next_uniform_f_cc(TC_RandGen* rgen, float min, float max)
{
    return tcrand_i_lerp_f(tcrand_next_uniform_u32(rgen, 0, TCRAND_I_MAX24 - 1U) * TCRAND_I_INVM24M1, min, max);
}
double tcrand_next_uniform_d_cc(TC_RandGen* rgen, double min, double max)
{
    return tcrand_i_lerp_d(tcrand_next_uniform_u64(rgen, 0, TCRAND_I_MAX53 - 1U) * TCRAND_I_INVM53M1, min, max);
}

TC_CFloat tcrand_next_normal2_f(TC_RandGen* rgen, float mean, float sd)
{
    static const float TwoPI = 6.28318530717958647692528676655900576839433879875021164195f;

    float u1 = tcrand_next_uniform_f_oc(rgen, 0.0f, 1.0f);
    float u2 = tcrand_next_uniform_f_oc(rgen, 0.0f, 1.0f);

    float sqrtMsd = sd * sqrtf(-2.0f * logf(u1));
    float A = TwoPI * u2;
    TC_CFloat ret;
    ret.re = sqrtMsd * cosf(A) + mean;
    ret.im = sqrtMsd * sinf(A) + mean;
    return ret;
}
TC_CDouble tcrand_next_normal2_d(TC_RandGen* rgen, double mean, double sd)
{
    static const double TwoPI = 6.28318530717958647692528676655900576839433879875021164195;

    double u1 = tcrand_next_uniform_d_oc(rgen, 0.0, 1.0);
    double u2 = tcrand_next_uniform_d_oc(rgen, 0.0, 1.0);

    double sqrtMsd = sd * sqrt(-2.0 * log(u1));
    double A = TwoPI * u2;
    TC_CDouble ret;
    ret.re = sqrtMsd * cos(A) + mean;
    ret.im = sqrtMsd * sin(A) + mean;
    return ret;
}

#endif /* TC_RANDOM_IMPLEMENTATION */
