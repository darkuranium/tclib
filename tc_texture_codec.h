/*
 * tc_texture_codec.h: Hardware texture compression (de)compressor.
 *
 * DEPENDS:
 * VERSION: 0.0.1 (2018-01-14)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Cas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.0.1    initial public release (BC1 to BC7)
 *
 * TODOs:
 * - Handle non-multiple-of-block outputs
 * - Tests!
 * - Optimization
 * - Compression:
 *      - Everything!
 * - Decompression:
 *      - ASTC
 *      - ETC2/EAC/PVRTC/...
 *
 *
 *
 * A library for (de)compressing block-compressed texture files.
 *
 * A single file should contain the following `#define` before including the header:
 *
 *      #define TC_TEXTURE_CODEC_IMPLEMENTATION
 *      #include "tc_texture_codec.h"
 *
 *
 *
 * The following decompressors are available:
 * - S3TC:
 *      - BC1 (a.k.a. DXT1) - also used for BC2 & BC3 [RGB8 or RGBA8 depending on `use_alpha`; if in doubt, `use_select` should be `true`]
 *      - BC2 (a.k.a. DXT2, DXT3; Alpha4 + BC1) [RGBA8]
 *      - BC3 (a.k.a. DXT4, DXT5; BC4 + BC1) [RGBA8]
 * - RGTC:
 *      - BC4 (R; also used for BC3 & BC5) [R8 or SignedR8 depending on `is_signed`]
 *      - BC5 (RG; BC4 + BC4) [RG8 or SignedRG8 depending on `is_signed`]
 * - BPTC:
 *      - BC6H (a.k.a. floating-point BPTC) [RGB16F; this is independent of `is_signed`, which only influences block interpretation]
 *      - BC7 (a.k.a. integer BPTC) [RGBA8]
 * - Miscellaneous:
 *      - Alpha4 (4-bit alpha) - used for BC2 [R8; it has been expanded to 8-bit]
 *
 * Note that some of there merely combine existing formats. Therefore, the "base"
 * formats (those that don't simply combine others) are: BC1, BC4, BC6H, BC7, and Alpha4.
 *
 * Currently, there are no available COMpressors. They are, however, planned.
 */

/* ========== API ==========
 *
 * All of the decompressors use the following pattern; "BC7" in function names
 * is used merely as an example, and can be replaced by any available decompressor.
 *
 * Not counting the utility functions, there are 2 families of functions: block
 * decoders, and full image decoders. In most cases, you will want to use the
 * full decoders, which use the following calls:
 *
 *      tctex_decompress_bc7(dst, dstride_x, dpitch_y, src, w, h);
 *
 * If only a single block of decompression is required for whatever reason,
 * that can be done via the `_block` family of functions:
 *
 *      //equivalent to:
 *      //tctex_decompress_bc7(dst, dstride_x, dpitch_y, block, BLOCK_SIZE_X, BLOCK_SIZE_Y);
 *      tctex_decompress_bc7_block(dst, dstride_x, dpitch_y, block);
 *
 * The following parameters are exposed:
 * - `dst`: Destination (decompressed) image.
 * - `dstride_x`: Distance between consecutive pixels of `dst`, in the X direction.
 *                Typically, this is the number of bytes per pixel --- e.g. 4 for
 *                RGBA8, or 6 for RGB16F.
 * - `dpitch_y`: Distance between consecutive rows. Typically, this is `w*dstride_x + alignment_padding`.
 * - `src`: Source compressed image; the data must be consecutive (in other words,
 *          holes between individual blocks are not allowed).
 * - `w`,`h`: Size of the source (and destination) image. Currently, this must be
 *            a must be a multiple of the block size; this restriction may be relaxed
 *            in the future.
 *
 * Some functions have additional parameters:
 * - `use_select`: BC1 comes with two modes, one that selects an interpolation mode
 *                 based on certain properties of the block, and one that does not.
 *                 If in doubt, set this to `true`!
 * - `use_alpha`: Again with BC1: It can either use a 1-bit alpha for the black color
 *                (expanded by the library to 8 bits), or consider it opaque. This
 *                influences whether the output is RGB or RGBA!
 * - `is_signed`: For decompressors that support it, this toggles whether the input
 *                (& output) data is signed or unsigned. Signed data should be used
 *                with the relevant _SNORM or _SINT types.
 *                EXCEPTION: The BC6H compression method works differently. While
 *                `is_signed` still influences whether the *input* data is signed,
 *                the output is always a *signed* half-float.
 *
 *
 *
 * ========== UTILITY ==========
 *
 * The following utility functions exist; note that they are likely to be moved
 * to a different library in the future, but are kept here for the time being,
 * for easier testing.
 *
 *      float tctex_util_float_from_half(uint16_t half);
 *      float tctex_util_linear_from_srgb(uint8_t srgb);
 *      uint8_t tctex_util_srgb_from_linear(float linear);
 *
 * Their functions should be self-explanatory. Note that they only take single
 * components, so you should iterate over RGB(A) as necessary! Also note that
 * the alpha channel should be left in linear space in Linear<->SRGB conversions
 * (simply multiply or divide it by 255.0f instead of passing it through the function).
 *
 * The sRGB conversions use the formulae outlined in ARB_framebuffer_sRGB.
 */

#ifndef TC_TEXTURE_CODEC_H_
#define TC_TEXTURE_CODEC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This is mentioned above, but for a quick reference, the output formats:
 *
 * Alpha4:          R8 (but encodes alpha)
 * BC1:             RGB8 or RGBA8 (depending on `use_alpha`). If in doubt, `use_select=true`.
 * BC2, BC3, BC7:   RGBA8
 * BC4:             R8 or SignedR8 (depending on `is_signed`)
 * BC5:             RG8 or SignedRG8 (depending on `is_signed`)
 * BC6H:            RGB16F (but see tctex_util_float_from_half for converting to full float, if necessary)
 *
 * NOTE: RGB modes do *NOT* write alpha, even if the stride is large enough. If
 * you want alpha for those modes, fill the empty components manually!
 */

void tctex_decompress_alpha4_block(void* dst, size_t dstride_x, size_t dpitch_y, const void* block);
void tctex_decompress_bc1_block(void* dst, size_t dstride_x, size_t dpitch_y, const void* block, bool use_select, bool use_alpha);
void tctex_decompress_bc4_block(void* dst, size_t dstride_x, size_t dpitch_y, const void* block, bool is_signed);
void tctex_decompress_bc6h_block(void* dst, size_t dstride_x, size_t dpitch_y, const void* block, bool is_signed);
void tctex_decompress_bc7_block(void* dst, size_t dstride_x, size_t dpitch_y, const void* block);

void tctex_decompress_bc1(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h, bool is_signed, bool use_alpha);
void tctex_decompress_bc2(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h);
void tctex_decompress_bc3(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h);
void tctex_decompress_bc4(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h, bool is_signed);
void tctex_decompress_bc5(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h, bool is_signed);
void tctex_decompress_bc6h(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h, bool is_signed);
void tctex_decompress_bc7(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h);

// n.b.: may be moved to a different library in the future!
float tctex_util_float_from_half(uint16_t half);
float tctex_util_linear_from_srgb(uint8_t srgb);
uint8_t tctex_util_srgb_from_linear(float linear);

#ifdef __cplusplus
}
#endif

#endif /* TC_TEXTURE_CODEC_H_ */

#ifdef TC_TEXTURE_CODEC_IMPLEMENTATION

#include <limits.h>
#include <math.h>

#ifndef TCTEX__FROM_LE16
static uint16_t tctex_i_from_le16(uint16_t x)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return (x >> 8) | (x << 8);
#else
#error "Undefined byte order!"
#endif
}
#define TCTEX__FROM_LE16(x) tctex_i_from_le16(x)
#endif

#ifndef TCTEX__FROM_LE32
static uint32_t tctex_i_from_le32(uint32_t x)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return ((uint32_t)TCTEX__FROM_LE16(x) << 16) | TCTEX__FROM_LE16(x >> 16);
#else
#error "Undefined byte order!"
#endif
}
#define TCTEX__FROM_LE32(x) tctex_i_from_le32(x)
#endif

/*#ifndef TCTEX__FROM_LE64
static uint64_t tctex_i_from_le64(uint64_t x)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return ((uint64_t)TCTEX__FROM_LE32(x) << 32) | TCTEX__FROM_LE32(x >> 32);
#else
#error "Undefined byte order!"
#endif
}
#define TCTEX__FROM_LE64(x) tctex_i_from_le64(x)
#endif*/

typedef union TCTex_ColorB5G6R5A8
{
    struct { uint16_t rgb; uint8_t a; } raw;
    struct { uint16_t b: 5; uint16_t g: 6; uint16_t r: 5; uint8_t a; } c;
} TCTex_ColorB5G6R5A8;
typedef union TCTex_ColorB8G8R8A8
{
    struct { uint8_t b, g, r, a; } c;
    uint8_t array[4];
} TCTex_ColorB8G8R8A8;
typedef union TCTex_ColorB16G16R16
{
    struct { uint16_t b, g, r; } c;
    uint16_t array[3];
} TCTex_ColorB16G16R16;

static TCTex_ColorB5G6R5A8 tctex_i_b5g6r5a8_interpolate3(TCTex_ColorB5G6R5A8 ca, TCTex_ColorB5G6R5A8 cb, uint8_t factor)
{
    uint8_t factora = 3 - factor;
    uint8_t factorb = factor;
    ca.c.r = (factora * ca.c.r + factorb * cb.c.r + 1) / 3;
    ca.c.g = (factora * ca.c.g + factorb * cb.c.g + 1) / 3;
    ca.c.b = (factora * ca.c.b + factorb * cb.c.b + 1) / 3;
    ca.c.a = (factora * ca.c.a + factorb * cb.c.a + 1) / 3;
    return ca;
}
static TCTex_ColorB5G6R5A8 tctex_i_b5g6r5a8_interpolate2(TCTex_ColorB5G6R5A8 ca, TCTex_ColorB5G6R5A8 cb, uint8_t factor)
{
    uint8_t factora = 1 - factor;
    uint8_t factorb = factor;
    ca.c.r = (factora * ca.c.r + factorb * cb.c.r) / 2;
    ca.c.g = (factora * ca.c.g + factorb * cb.c.g) / 2;
    ca.c.b = (factora * ca.c.b + factorb * cb.c.b) / 2;
    ca.c.a = (factora * ca.c.a + factorb * cb.c.a) / 2;
    return ca;
}
static uint8_t tctex_i_interpolate_uodd(uint8_t ca, uint8_t cb, uint8_t factor, uint8_t max)
{
    return ((max - factor) * ca + factor * cb) / max;
}
static int8_t tctex_i_interpolate_sodd(int8_t ca, int8_t cb, int8_t factor, int8_t max)
{
    return ((max - factor) * ca + factor * cb) / max;
}
// as defined for BC6H & BC7 compression
static uint8_t tctex_i_expandchannel8(uint8_t c, uint8_t cbits)
{
    if(cbits == 8) return c;
    c <<= (8 - cbits);
    c |= c >> cbits;
    return c;
}
static TCTex_ColorB8G8R8A8 tctex_i_b8g8r8a8_expandbits(TCTex_ColorB8G8R8A8 scolor, uint8_t rbits, uint8_t gbits, uint8_t bbits, uint8_t abits)
{
    scolor.c.r = tctex_i_expandchannel8(scolor.c.r, rbits);
    scolor.c.g = tctex_i_expandchannel8(scolor.c.g, gbits);
    scolor.c.b = tctex_i_expandchannel8(scolor.c.b, bbits);
    scolor.c.a = tctex_i_expandchannel8(scolor.c.a, abits);
    return scolor;
}
static TCTex_ColorB8G8R8A8 tctex_i_b8g8r8a8_from_b5g6r5a8(TCTex_ColorB5G6R5A8 scolor)
{
    TCTex_ColorB8G8R8A8 dcolor;
    dcolor.c.r = scolor.c.r;
    dcolor.c.g = scolor.c.g;
    dcolor.c.b = scolor.c.b;
    dcolor.c.a = scolor.c.a;
    return tctex_i_b8g8r8a8_expandbits(dcolor, 5, 6, 5, 8);
}
static TCTex_ColorB8G8R8A8 tctex_i_b8g8r8a8_interpolate64a(TCTex_ColorB8G8R8A8 ca, TCTex_ColorB8G8R8A8 cb, uint8_t cfactor, uint8_t afactor)
{
    uint8_t cfactora = 64 - cfactor;
    uint8_t cfactorb = cfactor;
    uint8_t afactora = 64 - afactor;
    uint8_t afactorb = afactor;
    ca.c.r = (cfactora * ca.c.r + cfactorb * cb.c.r + 32) >> 6;
    ca.c.g = (cfactora * ca.c.g + cfactorb * cb.c.g + 32) >> 6;
    ca.c.b = (cfactora * ca.c.b + cfactorb * cb.c.b + 32) >> 6;
    ca.c.a = (afactora * ca.c.a + afactorb * cb.c.a + 32) >> 6;
    return ca;
}
static uint16_t tctex_i_sign_extend16(uint16_t x, uint8_t bits)
{
    uint16_t mask = 1U << (bits - 1);
    return (x ^ mask) - mask;
}
static TCTex_ColorB16G16R16 tctex_i_b16g16r16_interpolate64(TCTex_ColorB16G16R16 ca, TCTex_ColorB16G16R16 cb, uint8_t factor)
{
    uint8_t factora = 64 - factor;
    uint8_t factorb = factor;
    uint8_t i;
    for(i = 0; i < 3; i++)
        ca.array[i] = (factora * ca.array[i] + factorb * cb.array[i] + 32) >> 6;
    return ca;
}

void tctex_decompress_alpha4_block(void* dst, size_t dstride_x, size_t dpitch_y, const void* block)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,block);

    size_t i, x;
    for(i = 0; i < 8; i++)
    {
        uint8_t aselect[2];
        // alpha4 uses MSB first!
        aselect[1] = (bdata[8-1-i] >> 0) & 0xF; // ---- %%%%
        aselect[0] = (bdata[8-1-i] >> 4) & 0xF; // %%%% ----
        for(x = 0; x < 2; x++)
            ddata[(i >> 1) * dpitch_y + (2 * (i & 1) + x) * dstride_x] = tctex_i_expandchannel8(aselect[x], 4);
    }
}
void tctex_decompress_bc1_block(void* dst, size_t dstride_x, size_t dpitch_y, const void* block, bool use_select, bool use_alpha)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,block);

    TCTex_ColorB5G6R5A8 colors[4];
    colors[0].raw.rgb = TCTEX__FROM_LE16(*TC__STATIC_CAST(const uint16_t*,&bdata[0*sizeof(uint16_t)]));
    colors[0].raw.a = 0xFF;
    colors[1].raw.rgb = TCTEX__FROM_LE16(*TC__STATIC_CAST(const uint16_t*,&bdata[1*sizeof(uint16_t)]));
    colors[1].raw.a = 0xFF;

    //TODO: should use_select be inverted?
    if(!use_select || colors[0].raw.rgb > colors[1].raw.rgb)
    {
        colors[2] = tctex_i_b5g6r5a8_interpolate3(colors[0], colors[1], 1);
        colors[3] = tctex_i_b5g6r5a8_interpolate3(colors[0], colors[1], 2);
    }
    else
    {
        colors[2] = tctex_i_b5g6r5a8_interpolate2(colors[0], colors[1], 1);
        colors[3].raw.rgb = 0x0000U;
        colors[3].raw.a = 0x00U; // if use_alpha == false, this is technically 0xFF; but since we don't write to alpha in that case, it doesn't matter!
    }

    TCTex_ColorB8G8R8A8 colors8[sizeof(colors) / sizeof(*colors)];
    size_t i;
    for(i = 0; i < sizeof(colors) / sizeof(*colors); i++)
        colors8[i] = tctex_i_b8g8r8a8_from_b5g6r5a8(colors[i]);

    size_t x, y;
    for(y = 0; y < 4; y++)
    {
        for(x = 0; x < 4; x++)
        {
            TCTex_ColorB8G8R8A8 curc = colors8[(bdata[4+y] >> (2*x)) & 0x3];

            uint8_t* dptr = &ddata[y * dpitch_y + x * dstride_x];
            dptr[0] = curc.c.r;
            dptr[1] = curc.c.g;
            dptr[2] = curc.c.b;
            if(use_alpha) dptr[3] = curc.c.a;
        }
    }
}
void tctex_decompress_bc4_block(void* dst, size_t dstride, size_t dpitch, const void* block, bool is_signed)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,block);

    uint8_t alpha[8];
    alpha[0] = bdata[0];
    alpha[1] = bdata[1];

    size_t i;
    if(is_signed)
    {
        // n.b.: BC4 is undefined if alpha[0] == -127 && alpha[1] == 128; maybe we should output all-0 in that case?
        if(alpha[0] > alpha[1])
        {
            for(i = 2; i < 8; i++)
                alpha[i] = tctex_i_interpolate_sodd(alpha[0], alpha[1], i - 1, 7);
        }
        else
        {
            for(i = 2; i < 6; i++)
                alpha[i] = tctex_i_interpolate_sodd(alpha[0], alpha[1], i - 1, 5);
            alpha[6] = -128;
            alpha[7] = 127;
        }
    }
    else
    {
        if(alpha[0] > alpha[1])
        {
            for(i = 2; i < 8; i++)
                alpha[i] = tctex_i_interpolate_uodd(alpha[0], alpha[1], i - 1, 7);
        }
        else
        {
            for(i = 2; i < 6; i++)
                alpha[i] = tctex_i_interpolate_uodd(alpha[0], alpha[1], i - 1, 5);
            alpha[6] = 0x00U;
            alpha[7] = 0xFFU;
        }
    }

    size_t x, y;
    for(i = 0; i < 2; i++)
    {
        uint8_t aselect[2][4];
        aselect[0][0] = (bdata[2+3*i+0] >> 0) & 0x7; // -- --- %%%
        aselect[0][1] = (bdata[2+3*i+0] >> 3) & 0x7; // -- %%% ---
        aselect[0][2] = ((bdata[2+3*i+0] >> 6) & 0x7) | (((bdata[2+3*i+1] >> 0) & 0x1) << 2); // - --- --- % | %% --- ---
        aselect[0][3] = (bdata[2+3*i+1] >> 1) & 0x7; // - --- %%% -
        aselect[1][0] = (bdata[2+3*i+1] >> 4) & 0x7; // - %%% --- -
        aselect[1][1] = ((bdata[2+3*i+1] >> 7) & 0x1) | (((bdata[2+3*i+2] >> 0) & 0x3) << 1); // --- --- %% | % --- --- -
        aselect[1][2] = (bdata[2+3*i+2] >> 2) & 0x7; // --- %%% --
        aselect[1][3] = (bdata[2+3*i+2] >> 5) & 0x7; // %%% --- --
        for(y = 0; y < 2; y++)
            for(x = 0; x < 4; x++)
                ddata[(2*i + y) * dpitch + x * dstride] = alpha[aselect[y][x]];
    }
}

// Yup, it's a mess. I'll fix it someday. Probably.
// note: NUM <= sizeof(*(ARR) * CHAR_BIT)
// possible cases (LSB first):
//      %%%%----
//      --%%%%--
//      ------%% | %%------
#define TCTEX_I_GETBITS(ARR,START,NUM)  (((START)/(sizeof(*(ARR))*CHAR_BIT)) == (((START)+(NUM))/(sizeof(*(ARR))*CHAR_BIT)) /* is it contained in a single element? */\
                                            ? ((ARR)[(START)/(sizeof(*(ARR))*CHAR_BIT)] & ((1U<<((NUM)+((START)&(sizeof(*(ARR))*CHAR_BIT-1))))-1)) >> ((START)&(sizeof(*(ARR))*CHAR_BIT-1))\
                                            : ((ARR)[(START)/(sizeof(*(ARR))*CHAR_BIT)] >> ((START)&(sizeof(*(ARR))*CHAR_BIT-1))) /* low part */ | (((ARR)[((START)/(sizeof(*(ARR))*CHAR_BIT))+1] & ((1U<<((NUM)-(sizeof(*(ARR))*CHAR_BIT-((START)&(sizeof(*(ARR))*CHAR_BIT-1)))))-1)) << (sizeof(*(ARR))*CHAR_BIT-((START)&(sizeof(*(ARR))*CHAR_BIT-1)))) /* high part */)


static const uint16_t tctex_i_bc6h7_partitions2[] = {
    0xCCCC,0x8888,0xEEEE,0xECC8,0xC880,0xFEEC,0xFEC8,0xEC80,
    0xC800,0xFFEC,0xFE80,0xE800,0xFFE8,0xFF00,0xFFF0,0xF000,
    0xF710,0x008E,0x7100,0x08CE,0x008C,0x7310,0x3100,0x8CCE,
    0x088C,0x3110,0x6666,0x366C,0x17E8,0x0FF0,0x718E,0x399C,
    0xAAAA,0xF0F0,0x5A5A,0x33CC,0x3C3C,0x55AA,0x9696,0xA55A,
    0x73CE,0x13C8,0x324C,0x3BDC,0x6996,0xC33C,0x9966,0x0660,
    0x0272,0x04E4,0x4E40,0x2720,0xC936,0x936C,0x39C6,0x639C,
    0x9336,0x9CC6,0x817E,0xE718,0xCCF0,0x0FCC,0x7744,0xEE22,
};
static const uint32_t tctex_i_bc7_partitions3[] = { /* [KHR] Table.P3; 2 bits per entry */
    0xAA685050,0x6A5A5040,0x5A5A4200,0x5450A0A8,0xA5A50000,0xA0A05050,0x5555A0A0,0x5A5A5050,
    0xAA550000,0xAA555500,0xAAAA5500,0x90909090,0x94949494,0xA4A4A4A4,0xA9A59450,0x2A0A4250,
    0xA5945040,0x0A425054,0xA5A5A500,0x55A0A0A0,0xA8A85454,0x6A6A4040,0xA4A45000,0x1A1A0500,
    0x0050A4A4,0xAAA59090,0x14696914,0x69691400,0xA08585A0,0xAA821414,0x50A4A450,0x6A5A0200,
    0xA9A58000,0x5090A0A8,0xA8A09050,0x24242424,0x00AA5500,0x24924924,0x24499224,0x50A50A50,
    0x500AA550,0xAAAA4444,0x66660000,0xA5A0A5A0,0x50A050A0,0x69286928,0x44AAAA44,0x66666600,
    0xAA444444,0x54A854A8,0x95809580,0x96969600,0xA85454A8,0x80959580,0xAA141414,0x96960000,
    0xAAAA1414,0xA05050A0,0xA0A5A5A0,0x96000000,0x40804080,0xA9A8A9A8,0xAAAAAA44,0x2A4A5254,
};
static const uint8_t tctex_i_bc6h7_partitions2_anchors[] = { /* [KHR] Table.A2 */
    15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,
    15, 2, 8, 2, 2, 8, 8,15,
     2, 8, 2, 2, 8, 8, 2, 2,
    15,15, 6, 8, 2, 8,15,15,
     2, 8, 2, 2, 2,15,15, 6,
     6, 2, 6, 8,15,15, 2, 2,
    15,15,15,15,15, 2, 2,15,
};
static const uint8_t tctex_i_bc7_partitions3_anchors[2][8*8]= {
    { /* [KHR] Table.A3a */
         3, 3,15,15, 8, 3,15,15,
         8, 8, 6, 6, 6, 5, 3, 3,
         3, 3, 8,15, 3, 3, 6,10,
         5, 8, 8, 6, 8, 5,15,15,
         8,15, 3, 5, 6,10, 8,15,
        15, 3,15, 5,15,15,15,15,
         3,15, 5, 5, 5, 8, 5,10,
         5,10, 8,13,15,12, 3, 3,
     },
     { /* [KHR] Table.A3b */
        15, 8, 8, 3,15,15, 3, 8,
        15,15,15,15,15,15,15, 8,
        15, 8,15, 3,15, 8,15, 8,
         3,15, 6,10,15,15,10, 8,
        15, 3,15,10,10, 8, 9,10,
         6,15, 8,15, 3, 6, 6, 8,
        15, 3,15,15,15,15,15,15,
        15,15,15,15, 3,15,15, 8,
     },
};
static const uint8_t tctex_i_bc7_interp_factors2[] = { 0, 21, 43, 64 };
static const uint8_t tctex_i_bc6h7_interp_factors3[] = { 0, 9, 18, 27, 37, 46, 55, 64 };
static const uint8_t tctex_i_bc6h7_interp_factors4[] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };
static const uint8_t* const tctex_i_bc6h7_interp_factors[] = { tctex_i_bc7_interp_factors2, tctex_i_bc6h7_interp_factors3, tctex_i_bc6h7_interp_factors4 };

static uint16_t tctex_i_bc6h_unquantize_component(uint16_t x, bool is_signed, uint8_t EPB)
{
    if(is_signed)
    {
        uint8_t s;
        if(EPB >= 16) return x;

        if(x & 0x8000)
        {
            s = 1;
            x = -x;
        }
        else
            s = 0;

        uint16_t unq;
        if(!x) unq = 0;
        else if(x >= ((1<<(EPB-1))-1)) unq = 0x7FFF;
        else unq = ((x<<15) + 0x4000) >> (EPB-1);

        return s ? -unq : unq;
    }
    else
    {
        if(EPB >= 15) return x;
        if(!x) return 0;
        if(x == ((1<<EPB)-1)) return 0xFFFF;
        return ((x<<15) + 0x4000) >> (EPB-1);
    }
}
static TCTex_ColorB16G16R16 tctex_i_bc6h_unquantize(TCTex_ColorB16G16R16 color, bool is_signed, uint8_t EPB)
{
    uint8_t i;
    for(i = 0; i < 3; i++)
        color.array[i] = tctex_i_bc6h_unquantize_component(color.array[i], is_signed, EPB);
    return color;
}
static uint16_t tctex_i_bc6h_unquantize_component_final(uint16_t x, bool is_signed)
{
    if(is_signed)
    {
        uint16_t s = x & 0x8000;
        if(s) x = -x;
        return ((x * 31) >> 5) | s;
    }
    else
        return (x * 31) >> 6;
}
static TCTex_ColorB16G16R16 tctex_i_bc6h_unquantize_final(TCTex_ColorB16G16R16 color, bool is_signed)
{
    uint8_t i;
    for(i = 0; i < 3; i++)
        color.array[i] = tctex_i_bc6h_unquantize_component_final(color.array[i], is_signed);
    return color;
}
void tctex_decompress_bc6h_block(void* dst, size_t dstride, size_t dpitch, const void* block, bool is_signed)
{
    static const struct TCTex_BC6H_ModeInfo
    {
        uint8_t Tr;     // Transformed endpoints
        uint8_t PB;     // Partition bits
        uint8_t EPB;    // Endpoint bits
        uint8_t DB[3];  // Delta bits (R,G,B)
    } ModeInfos[] = { /* [KHR] Table.MF */
                            //[KHR] [MSDN]
        {1,5,10,{5,5,5}},   //0     1
        {1,5,7,{6,6,6}},    //1     2
        {1,5,11,{5,4,4}},   //2     3
        {0,0,10,{10,10,10}},//3     11
        {},
        {},
        {1,5,11,{4,5,4}},   //6     4
        {1,0,11,{9,9,9}},   //7     12
        {},
        {},
        {1,5,11,{4,4,5}},   //10    5
        {1,0,12,{8,8,8}},   //11    13
        {},
        {},
        {1,5,9,{5,5,5}},    //14    6
        {1,0,16,{4,4,4}},   //15    14
        {},
        {},
        {1,5,8,{6,5,5}},    //18    7
        {},                 //19:RESERVED
        {},
        {},
        {1,5,8,{5,6,5}},    //22    8
        {},                 //23:RESERVED
        {},
        {},
        {1,5,8,{5,5,6}},    //26    9
        {},                 //27:RESERVED
        {},
        {},
        {0,5,6,{6,6,6}},    //30    10
        {},                 //31:RESERVED
    };

    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint32_t* bdata = TC__VOID_CAST(const uint32_t*,block);

    uint8_t x, y;

    uint8_t mode = TCTEX_I_GETBITS(bdata, 0, 5);
    if(!(mode & 0x2)) mode &= 0x1; // 2-bit mode
    struct TCTex_BC6H_ModeInfo minfo = ModeInfos[mode];
    if(!minfo.EPB) // invalid; [KHR] and [MSDN] ask for a value of (0,0,0,1)
    {
        for(y = 0; y < 4; y++)
        {
            for(x = 0; x < 4; x++)
            {
                uint16_t* dptr = TC__STATIC_CAST(uint16_t*,&ddata[y * dpitch + x * dstride]);
                dptr[0] = dptr[1] = dptr[2] = 0;
            }
        }
        return;
    }
    uint8_t NS = 1 + !!minfo.PB;
    uint8_t IB = minfo.PB ? 3 : 4;

    uint32_t i;

    TCTex_ColorB16G16R16 colors[2*2] = {};
    const uint8_t* factors = tctex_i_bc6h7_interp_factors[IB-2];

    /* BEGIN AUTOGENERATED SECTION; DO NOT EDIT (modify gen_bc6h_parser.py instead!) */
    switch(mode)
    {
    case 0:
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 2, 1) << 4;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 3, 1) << 4;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 4, 1) << 4;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 10) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 10) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 10) << 0;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 5) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 40, 1) << 4;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 50, 1) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 60, 1) << 1;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 70, 1) << 2;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 76, 1) << 3;
        break;
    case 1:
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 2, 1) << 5;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 3, 1) << 4;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 4, 1) << 5;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 7) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 12, 1) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 13, 1) << 1;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 14, 1) << 4;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 7) << 0;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 22, 1) << 5;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 23, 1) << 2;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 24, 1) << 4;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 7) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 32, 1) << 3;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 33, 1) << 5;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 34, 1) << 4;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 6) << 0;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 6) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 6) << 0;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 6) << 0;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 6) << 0;
        break;
    case 2:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 10) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 10) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 10) << 0;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 5) << 0;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 40, 1) << 10;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 4) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 49, 1) << 10;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 50, 1) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 4) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 59, 1) << 10;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 60, 1) << 1;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 70, 1) << 2;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 76, 1) << 3;
        break;
    case 6:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 10) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 10) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 10) << 0;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 4) << 0;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 39, 1) << 10;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 40, 1) << 4;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 5) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 50, 1) << 10;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 4) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 59, 1) << 10;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 60, 1) << 1;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 4) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 69, 1) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 70, 1) << 2;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 4) << 0;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 75, 1) << 4;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 76, 1) << 3;
        break;
    case 10:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 10) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 10) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 10) << 0;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 4) << 0;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 39, 1) << 10;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 40, 1) << 4;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 4) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 49, 1) << 10;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 50, 1) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 5) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 60, 1) << 10;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 4) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 69, 1) << 1;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 70, 1) << 2;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 4) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 75, 1) << 4;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 76, 1) << 3;
        break;
    case 14:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 9) << 0;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 14, 1) << 4;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 9) << 0;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 24, 1) << 4;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 9) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 34, 1) << 4;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 5) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 40, 1) << 4;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 50, 1) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 60, 1) << 1;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 70, 1) << 2;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 76, 1) << 3;
        break;
    case 18:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 8) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 13, 1) << 4;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 14, 1) << 4;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 8) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 23, 1) << 2;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 24, 1) << 4;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 8) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 33, 1) << 3;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 34, 1) << 4;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 6) << 0;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 50, 1) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 60, 1) << 1;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 6) << 0;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 6) << 0;
        break;
    case 22:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 8) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 13, 1) << 0;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 14, 1) << 4;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 8) << 0;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 23, 1) << 5;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 24, 1) << 4;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 8) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 33, 1) << 5;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 34, 1) << 4;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 5) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 40, 1) << 4;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 6) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 60, 1) << 1;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 70, 1) << 2;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 76, 1) << 3;
        break;
    case 26:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 8) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 13, 1) << 1;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 14, 1) << 4;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 8) << 0;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 23, 1) << 5;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 24, 1) << 4;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 8) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 33, 1) << 5;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 34, 1) << 4;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 5) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 40, 1) << 4;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 50, 1) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 6) << 0;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 70, 1) << 2;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 5) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 76, 1) << 3;
        break;
    case 30:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 6) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 11, 1) << 4;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 12, 1) << 0;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 13, 1) << 1;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 14, 1) << 4;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 6) << 0;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 21, 1) << 5;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 22, 1) << 5;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 23, 1) << 2;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 24, 1) << 4;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 6) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 31, 1) << 5;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 32, 1) << 3;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 33, 1) << 5;
        colors[3].c.b |= TCTEX_I_GETBITS(bdata, 34, 1) << 4;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 6) << 0;
        colors[2].c.g |= TCTEX_I_GETBITS(bdata, 41, 4) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 6) << 0;
        colors[3].c.g |= TCTEX_I_GETBITS(bdata, 51, 4) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 6) << 0;
        colors[2].c.b |= TCTEX_I_GETBITS(bdata, 61, 4) << 0;
        colors[2].c.r |= TCTEX_I_GETBITS(bdata, 65, 6) << 0;
        colors[3].c.r |= TCTEX_I_GETBITS(bdata, 71, 6) << 0;
        break;
    case 3:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 10) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 10) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 10) << 0;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 10) << 0;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 10) << 0;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 10) << 0;
        break;
    case 7:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 10) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 10) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 10) << 0;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 9) << 0;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 44, 1) << 10;
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 9) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 54, 1) << 10;
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 9) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 64, 1) << 10;
        break;
    case 11:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 10) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 10) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 10) << 0;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 8) << 0;
        /* begin reverse read */
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 43, 1) << 11;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 44, 1) << 10;
        /* end reverse read */
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 8) << 0;
        /* begin reverse read */
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 53, 1) << 11;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 54, 1) << 10;
        /* end reverse read */
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 8) << 0;
        /* begin reverse read */
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 63, 1) << 11;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 64, 1) << 10;
        /* end reverse read */
        break;
    case 15:
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 5, 10) << 0;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 15, 10) << 0;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 25, 10) << 0;
        colors[1].c.r |= TCTEX_I_GETBITS(bdata, 35, 4) << 0;
        /* begin reverse read */
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 39, 1) << 15;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 40, 1) << 14;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 41, 1) << 13;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 42, 1) << 12;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 43, 1) << 11;
        colors[0].c.r |= TCTEX_I_GETBITS(bdata, 44, 1) << 10;
        /* end reverse read */
        colors[1].c.g |= TCTEX_I_GETBITS(bdata, 45, 4) << 0;
        /* begin reverse read */
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 49, 1) << 15;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 50, 1) << 14;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 51, 1) << 13;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 52, 1) << 12;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 53, 1) << 11;
        colors[0].c.g |= TCTEX_I_GETBITS(bdata, 54, 1) << 10;
        /* end reverse read */
        colors[1].c.b |= TCTEX_I_GETBITS(bdata, 55, 4) << 0;
        /* begin reverse read */
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 59, 1) << 15;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 60, 1) << 14;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 61, 1) << 13;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 62, 1) << 12;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 63, 1) << 11;
        colors[0].c.b |= TCTEX_I_GETBITS(bdata, 64, 1) << 10;
        /* end reverse read */
        break;
    }
    /* END AUTOGENERATED SECTION */

    uint8_t partition_set_id = minfo.PB ? TCTEX_I_GETBITS(bdata, 77, 5) : 0;

    uint32_t c;
    // sign-extend (if necessary)
    if(is_signed)
        for(c = 0; c < 3; c++)
            colors[0].array[c] = tctex_i_sign_extend16(colors[0].array[c], minfo.EPB);
    // now for the rest
    for(i = 1; i < 2 * NS; i++)
    {
        // sign-extend (if necessary) ...
        if(is_signed || minfo.Tr)
            for(c = 0; c < 3; c++)
                colors[i].array[c] = tctex_i_sign_extend16(colors[i].array[c], minfo.DB[3-1-c]); // MSDN describes this as `minfo.DB[i]` instead of `[c]`. That is incorrect.

        // ... apply deltas (if necessary)
        if(minfo.Tr)
        {
            for(c = 0; c < 3; c++)
                colors[i].array[c] = (colors[i].array[c] + colors[0].array[c]) & ((1U<<minfo.EPB)-1U);

            if(is_signed)
                for(c = 0; c < 3; c++)
                    colors[i].array[c] = tctex_i_sign_extend16(colors[i].array[c], minfo.EPB);
        }
    }

    // ... then unquantize
    for(i = 0; i < 2 * NS; i++)
        colors[i] = tctex_i_bc6h_unquantize(colors[i], is_signed, minfo.EPB);
    uint32_t IBoffset = minfo.PB ? 82 : 65;
    for(y = 0; y < 4; y++)
    {
        for(x = 0; x < 4; x++)
        {
            i = y * 4 + x;

            uint8_t subset_index;
            uint8_t index_anchor;
            if(minfo.PB)
            {
                subset_index = (tctex_i_bc6h7_partitions2[partition_set_id] >> i) & 1;
                index_anchor = subset_index ? tctex_i_bc6h7_partitions2_anchors[partition_set_id] : 0;
            }
            else
            {
                subset_index = 0;
                index_anchor = 0;
            }

            uint8_t index;
            uint8_t num;
            num = IB - (i == index_anchor);
            index = TCTEX_I_GETBITS(bdata, IBoffset, num);
            IBoffset += num;

            TCTex_ColorB16G16R16 endA = colors[2 * subset_index + 0];
            TCTex_ColorB16G16R16 endB = colors[2 * subset_index + 1];
            TCTex_ColorB16G16R16 ccol = tctex_i_bc6h_unquantize_final(tctex_i_b16g16r16_interpolate64(endA, endB, factors[index]), is_signed);

            uint16_t* dptr = TC__STATIC_CAST(uint16_t*,&ddata[y * dpitch + x * dstride]);
            dptr[0] = ccol.c.r;
            dptr[1] = ccol.c.g;
            dptr[2] = ccol.c.b;
        }
    }
}

static void tctex_i_bc7_get_color_data(TCTex_ColorB8G8R8A8* colors, uint8_t ncolors, const uint8_t* bdata, uint8_t offset, uint8_t cbits, uint8_t abits, uint8_t pbits, bool pshared)
{
    uint32_t i;
    for(i = 0; i < ncolors; i++)
    {
        colors[i].c.r = TCTEX_I_GETBITS(bdata, offset + (0 * ncolors + i) * cbits, cbits) << pbits;
        colors[i].c.g = TCTEX_I_GETBITS(bdata, offset + (1 * ncolors + i) * cbits, cbits) << pbits;
        colors[i].c.b = TCTEX_I_GETBITS(bdata, offset + (2 * ncolors + i) * cbits, cbits) << pbits;
        colors[i].c.a = abits ? (TCTEX_I_GETBITS(bdata, offset + 3 * ncolors * cbits + i * abits, abits) << pbits) : 0xFF;
    }
    if(pbits)
    {
        if(pshared)
        {
            for(i = 0; i < ncolors / 2; i++)
            {
                uint8_t pdata = TCTEX_I_GETBITS(bdata, offset + (3 * cbits + abits) * ncolors + i * pbits, pbits);
                colors[2*i+0].c.r |= pdata;
                colors[2*i+0].c.g |= pdata;
                colors[2*i+0].c.b |= pdata;
                colors[2*i+0].c.a |= pdata; // no need to check for alpha presence here: if it's not present, we're doing `0xFF | pdata` anyhow!

                colors[2*i+1].c.r |= pdata;
                colors[2*i+1].c.g |= pdata;
                colors[2*i+1].c.b |= pdata;
                colors[2*i+1].c.a |= pdata;
            }
        }
        else
        {
            for(i = 0; i < ncolors; i++)
            {
                uint8_t pdata = TCTEX_I_GETBITS(bdata, offset + (3 * cbits + abits) * ncolors + i * pbits, pbits);
                colors[i].c.r |= pdata;
                colors[i].c.g |= pdata;
                colors[i].c.b |= pdata;
                colors[i].c.a |= pdata;
            }
        }
    }
}
void tctex_decompress_bc7_block(void* dst, size_t dstride, size_t dpitch, const void* block)
{
    static const struct TCTex_BC7_ModeInfo
    {
        uint8_t NS;     // Number of subsets in each partition
        uint8_t PB;     // Partition bits
        uint8_t RB;     // Rotation bits
        uint8_t ISB;    // Index selection bits
        uint8_t CB;     // Color bits
        uint8_t AB;     // Alpha bits
        uint8_t EPB;    // Endpoint P-bits
        uint8_t SPB;    // Shared P-bits
        uint8_t IB;     // Index bits per element
        uint8_t IB2;    // Secondary index bits per element
    } ModeInfos[] = { /* [KHR] Table.M */
        {3,4,0,0,4,0,1,0,3,0},
        {2,6,0,0,6,0,0,1,3,0},
        {3,6,0,0,5,0,0,0,2,0},
        {2,6,0,0,7,0,1,0,2,0},
        {1,0,2,1,5,6,0,0,2,3},
        {1,0,2,0,7,8,0,0,2,2},
        {1,0,0,0,7,7,1,0,4,0},
        {2,6,0,0,5,5,1,0,2,0},
    };

    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,block);

    uint8_t x, y;

    uint8_t mode;
    for(mode = 0; !(bdata[0] & (1 << mode)); mode++) {}
    if(mode == 8) // invalid; [MSDN] asks for 0-filling erroneous decodes, which sounds sensible enough
    {
        for(y = 0; y < 4; y++)
            for(x = 0; x < 4; x++)
                memset(&ddata[y * dpitch + x * dstride], 0, 4 * sizeof(uint8_t));
        return;
    }

    struct TCTex_BC7_ModeInfo minfo = ModeInfos[mode];
    uint8_t MPB = minfo.EPB ? minfo.EPB : minfo.SPB;

    uint32_t i;

    TCTex_ColorB8G8R8A8 colors[3*2];
    const uint8_t* factors[2];

    uint8_t partition_set_id, rotation, index_selection;
    partition_set_id = minfo.PB ? TCTEX_I_GETBITS(bdata, mode+1, minfo.PB) : 0;
    rotation = minfo.RB ? TCTEX_I_GETBITS(bdata, mode+1 + minfo.PB, minfo.RB) : 0;
    index_selection = minfo.ISB ? TCTEX_I_GETBITS(bdata, mode+1 + minfo.PB + minfo.RB, minfo.ISB) : 0;
    tctex_i_bc7_get_color_data(colors, 2 * minfo.NS, bdata, mode+1 + minfo.PB + minfo.RB + minfo.ISB, minfo.CB, minfo.AB, MPB, minfo.SPB);
    for(i = 0; i < 2 * minfo.NS; i++)
        colors[i] = tctex_i_b8g8r8a8_expandbits(colors[i], minfo.CB + MPB, minfo.CB + MPB, minfo.CB + MPB, minfo.AB ? minfo.AB + MPB : 8);
    factors[0] = tctex_i_bc6h7_interp_factors[minfo.IB-2];
    factors[1] = minfo.IB2 ? tctex_i_bc6h7_interp_factors[minfo.IB2-2] : factors[0];

    uint32_t IBoffset = mode+1 + minfo.PB + minfo.RB + minfo.ISB + minfo.NS*(2*(3*minfo.CB + minfo.AB + minfo.EPB) + minfo.SPB);
    uint32_t IB2offset = IBoffset + minfo.NS*(16*minfo.IB-1);
    for(y = 0; y < 4; y++)
    {
        for(x = 0; x < 4; x++)
        {
            i = y * 4 + x;

            uint8_t subset_index;
            uint8_t index_anchor;
            if(minfo.NS == 3)
            {
                subset_index = (tctex_i_bc7_partitions3[partition_set_id] >> (2*i)) & 3;
                index_anchor = subset_index ? tctex_i_bc7_partitions3_anchors[subset_index-1][partition_set_id] : 0;
            }
            else if(minfo.NS == 2)
            {
                subset_index = (tctex_i_bc6h7_partitions2[partition_set_id] >> i) & 1;
                index_anchor = subset_index ? tctex_i_bc6h7_partitions2_anchors[partition_set_id] : 0;
            }
            else
            {
                subset_index = 0;
                index_anchor = 0;
            }

            uint8_t index[2];
            uint8_t num[2];
            num[0] = minfo.IB - (i == index_anchor);
            num[1] = minfo.IB2 - (i == index_anchor);
            // TODO: check for out of bounds
            index[0] = TCTEX_I_GETBITS(bdata, IBoffset, num[0]);
            index[1] = minfo.IB2 ? TCTEX_I_GETBITS(bdata, IB2offset, num[1]) : index[0];
            IBoffset += num[0];
            IB2offset += num[1];

            TCTex_ColorB8G8R8A8 endA = colors[2 * subset_index + 0];
            TCTex_ColorB8G8R8A8 endB = colors[2 * subset_index + 1];
            TCTex_ColorB8G8R8A8 ccol = tctex_i_b8g8r8a8_interpolate64a(endA, endB, factors[index_selection][index[index_selection]], factors[!index_selection][index[!index_selection]]);

            uint8_t ctmp;
            switch(rotation)
            {
            case 0: break;
            case 1: ctmp = ccol.c.a; ccol.c.a = ccol.c.r; ccol.c.r = ctmp; break;
            case 2: ctmp = ccol.c.a; ccol.c.a = ccol.c.g; ccol.c.g = ctmp; break;
            case 3: ctmp = ccol.c.a; ccol.c.a = ccol.c.b; ccol.c.b = ctmp; break;
            }

            uint8_t* dptr = &ddata[y * dpitch + x * dstride];
            dptr[0] = ccol.c.r;
            dptr[1] = ccol.c.g;
            dptr[2] = ccol.c.b;
            dptr[3] = ccol.c.a;
        }
    }
}


void tctex_decompress_bc1(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h, bool use_select, bool use_alpha)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,src);

    size_t y;
    for(y = 0; y < h; y += 4)
    {
        size_t x;
        for(x = 0; x < w; x += 4)
        {
            tctex_decompress_bc1_block(&ddata[y * dpitch_y + x * dstride_x], dstride_x, dpitch_y, bdata, use_select, use_alpha);
            bdata += 8;
        }
    }
}
void tctex_decompress_bc2(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,src);

    size_t y;
    for(y = 0; y < h; y += 4)
    {
        size_t x;
        for(x = 0; x < w; x += 4)
        {
            tctex_decompress_alpha4_block(&ddata[y * dpitch_y + x * dstride_x + 3], dstride_x, dpitch_y, bdata);
            tctex_decompress_bc1_block(&ddata[y * dpitch_y + x * dstride_x + 0], dstride_x, dpitch_y, bdata + 8, false, false);
            bdata += 16;
        }
    }
}
void tctex_decompress_bc3(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,src);

    size_t y;
    for(y = 0; y < h; y += 4)
    {
        size_t x;
        for(x = 0; x < w; x += 4)
        {
            tctex_decompress_bc4_block(&ddata[y * dpitch_y + x * dstride_x + 3], dstride_x, dpitch_y, bdata, false);
            tctex_decompress_bc1_block(&ddata[y * dpitch_y + x * dstride_x + 0], dstride_x, dpitch_y, bdata + 8, true, false);
            bdata += 16;
        }
    }
}
void tctex_decompress_bc4(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h, bool is_signed)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,src);

    size_t y;
    for(y = 0; y < h; y += 4)
    {
        size_t x;
        for(x = 0; x < w; x += 4)
        {
            tctex_decompress_bc4_block(&ddata[y * dpitch_y + x * dstride_x], dstride_x, dpitch_y, bdata, is_signed);
            bdata += 8;
        }
    }
}
void tctex_decompress_bc5(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h, bool is_signed)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,src);

    size_t y;
    for(y = 0; y < h; y += 4)
    {
        size_t x;
        for(x = 0; x < w; x += 4)
        {
            tctex_decompress_bc4_block(&ddata[y * dpitch_y + x * dstride_x + 0], dstride_x, dpitch_y, bdata, is_signed);
            tctex_decompress_bc4_block(&ddata[y * dpitch_y + x * dstride_x + 1], dstride_x, dpitch_y, bdata + 4, is_signed);
            bdata += 16;
        }
    }
}
void tctex_decompress_bc6h(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h, bool is_signed)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,src);

    size_t y;
    for(y = 0; y < h; y += 4)
    {
        size_t x;
        for(x = 0; x < w; x += 4)
        {
            tctex_decompress_bc6h_block(&ddata[y * dpitch_y + x * dstride_x], dstride_x, dpitch_y, bdata, is_signed);
            bdata += 16;
        }
    }
}
void tctex_decompress_bc7(void* dst, size_t dstride_x, size_t dpitch_y, const void* src, size_t w, size_t h)
{
    uint8_t* ddata = TC__VOID_CAST(uint8_t*,dst);
    const uint8_t* bdata = TC__VOID_CAST(const uint8_t*,src);

    size_t y;
    for(y = 0; y < h; y += 4)
    {
        size_t x;
        for(x = 0; x < w; x += 4)
        {
            tctex_decompress_bc7_block(&ddata[y * dpitch_y + x * dstride_x], dstride_x, dpitch_y, bdata);
            bdata += 16;
        }
    }
}


float tctex_util_linear_from_srgb(uint8_t srgb)
{
    float fsrgb = srgb / 255.0f;
    if(fsrgb <= 0.04045f) return fsrgb / 12.92f;
    return powf((fsrgb + 0.055f) / 1.055f, 2.4f);
}
static uint8_t tctex_i_clamp_u8f32(float f)
{
    int i = (int)f;
    return i < 0 ? 0 : i > 255 ? 255 : i;
}
uint8_t tctex_util_srgb_from_linear(float linear)
{
    if(linear <= 0.0f) return 0;
    if(linear < 0.0031308f) return tctex_i_clamp_u8f32(255.0f * 12.92f * linear);
    if(linear < 1.0f) return tctex_i_clamp_u8f32(255.0f * (1.055f * powf(linear, 0.41666f) - 0.055f));
    return 255;
}
float tctex_util_float_from_half(uint16_t shalf)
{
    // s=1,e=5,m=10
    // seeeeemmmmmmmmmm
    uint16_t s = shalf & 0x8000;
    if(s) shalf = -shalf;
    float f = ldexpf((shalf & 0x3FF) | 0x400, (int)((shalf >> 10) & 0x1F) - 15 - 10);
    return s ? -f : f;
}

#endif /* TC_TEXTURE_CODEC_IMPLEMENTATION */
