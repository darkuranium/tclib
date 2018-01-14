/*
 * tc_texture_load.h: Texture image format loading.
 *
 * DEPENDS:
 * VERSION: 0.0.1 (2018-01-14)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Cas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.0.1    initial public release (DDS files only)
 *
 * TODOs:
 * - Test, test, test, and then test some more!
 * - Handle a lack of provided pitch or linear size.
 * - More FourCC codes, more legacy DDS formats, et cetera.
 * - KTX (Khronos Texture) support
 *      - https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
 * - ASTC support
 *      - https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_texture_compression_astc_hdr.txt
 *      - https://msdn.microsoft.com/en-us/library/windows/desktop/dn903790(v=vs.85).aspx
 *
 *
 *
 * A library for loading texture image formats, such as DDS.
 *
 * A single file should contain the following `#define` before including the header:
 *
 *      #define TC_TEXTURE_LOAD_IMPLEMENTATION
 *      #include "tc_texture_load.h"
 *
 * The following `#define`s are optional, and should be used to enable the
 * relevant conveniency functions:
 *
 *      #define TC_TEXTURE_LOAD_VULKAN_FORMATS
 *      #define TC_TEXTURE_LOAD_OPENGL_FORMATS
 *      #define TC_TEXTURE_LOAD_DIRECT3D_FORMATS
 */

#ifndef TC_TEXTURE_LOAD_H_
#define TC_TEXTURE_LOAD_H_

#include <stdint.h>
#ifndef TC_TEXTURE_LOAD_NO_STDIO
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TCTex_InternalFormat
{
    /* these map 1:1 to DDS in values (but might not always, so we keep a separate enum!) */
    TCTEX_IFORMAT_UNDEFINED = 0,
    TCTEX_IFORMAT_R32G32B32A32_TYPELESS = 1,
    TCTEX_IFORMAT_R32G32B32A32_SFLOAT = 2,
    TCTEX_IFORMAT_R32G32B32A32_UINT = 3,
    TCTEX_IFORMAT_R32G32B32A32_SINT = 4,
    TCTEX_IFORMAT_R32G32B32_TYPELESS = 5,
    TCTEX_IFORMAT_R32G32B32_SFLOAT = 6,
    TCTEX_IFORMAT_R32G32B32_UINT = 7,
    TCTEX_IFORMAT_R32G32B32_SINT = 8,
    TCTEX_IFORMAT_R16G16B16A16_TYPELESS = 9,
    TCTEX_IFORMAT_R16G16B16A16_SFLOAT = 10,
    TCTEX_IFORMAT_R16G16B16A16_UNORM = 11,
    TCTEX_IFORMAT_R16G16B16A16_UINT = 12,
    TCTEX_IFORMAT_R16G16B16A16_SNORM = 13,
    TCTEX_IFORMAT_R16G16B16A16_SINT = 14,
    TCTEX_IFORMAT_R32G32_TYPELESS = 15,
    TCTEX_IFORMAT_R32G32_SFLOAT = 16,
    TCTEX_IFORMAT_R32G32_UINT = 17,
    TCTEX_IFORMAT_R32G32_SINT = 18,
    TCTEX_IFORMAT_R32G8X24_TYPELESS = 19,
    TCTEX_IFORMAT_D32_SFLOAT_S8X24_UINT = 20,
    TCTEX_IFORMAT_R32_SFLOAT_X8X24_TYPELESS = 21,
    TCTEX_IFORMAT_X32_TYPELESS_G8X24_UINT = 22,
    TCTEX_IFORMAT_R10G10B10A2_TYPELESS = 23,
    TCTEX_IFORMAT_R10G10B10A2_UNORM = 24,
    TCTEX_IFORMAT_R10G10B10A2_UINT = 25,
    TCTEX_IFORMAT_R11G11B10_SFLOAT = 26,
    TCTEX_IFORMAT_R8G8B8A8_TYPELESS = 27,
    TCTEX_IFORMAT_R8G8B8A8_UNORM = 28,
    TCTEX_IFORMAT_R8G8B8A8_SRGB = 29,
    TCTEX_IFORMAT_R8G8B8A8_UINT = 30,
    TCTEX_IFORMAT_R8G8B8A8_SNORM = 31,
    TCTEX_IFORMAT_R8G8B8A8_SINT = 32,
    TCTEX_IFORMAT_R16G16_TYPELESS = 33,
    TCTEX_IFORMAT_R16G16_SFLOAT = 34,
    TCTEX_IFORMAT_R16G16_UNORM = 35,
    TCTEX_IFORMAT_R16G16_UINT = 36,
    TCTEX_IFORMAT_R16G16_SNORM = 37,
    TCTEX_IFORMAT_R16G16_SINT = 38,
    TCTEX_IFORMAT_R32_TYPELESS = 39,
    TCTEX_IFORMAT_D32_SFLOAT = 40,
    TCTEX_IFORMAT_R32_SFLOAT = 41,
    TCTEX_IFORMAT_R32_UINT = 42,
    TCTEX_IFORMAT_R32_SINT = 43,
    TCTEX_IFORMAT_R24G8_TYPELESS = 44,
    TCTEX_IFORMAT_D24_UNORM_S8_UINT = 45,
    TCTEX_IFORMAT_R24_UNORM_X8_TYPELESS = 46,
    TCTEX_IFORMAT_X24_TYPELESS_G8_UINT = 47,
    TCTEX_IFORMAT_R8G8_TYPELESS = 48,
    TCTEX_IFORMAT_R8G8_UNORM = 49,
    TCTEX_IFORMAT_R8G8_UINT = 50,
    TCTEX_IFORMAT_R8G8_SNORM = 51,
    TCTEX_IFORMAT_R8G8_SINT = 52,
    TCTEX_IFORMAT_R16_TYPELESS = 53,
    TCTEX_IFORMAT_R16_SFLOAT = 54,
    TCTEX_IFORMAT_D16_UNORM = 55,
    TCTEX_IFORMAT_R16_UNORM = 56,
    TCTEX_IFORMAT_R16_UINT = 57,
    TCTEX_IFORMAT_R16_SNORM = 58,
    TCTEX_IFORMAT_R16_SINT = 59,
    TCTEX_IFORMAT_R8_TYPELESS = 60,
    TCTEX_IFORMAT_R8_UNORM = 61,
    TCTEX_IFORMAT_R8_UINT = 62,
    TCTEX_IFORMAT_R8_SNORM = 63,
    TCTEX_IFORMAT_R8_SINT = 64,
    TCTEX_IFORMAT_A8_UNORM = 65,
    TCTEX_IFORMAT_R1_UNORM = 66,
    TCTEX_IFORMAT_R9G9B9E5_UFLOAT = 67,
    TCTEX_IFORMAT_R8G8_B8G8_UNORM = 68,
    TCTEX_IFORMAT_G8R8_G8B8_UNORM = 69,
    TCTEX_IFORMAT_COMPRESSED_BC1_TYPELESS = 70,
    TCTEX_IFORMAT_COMPRESSED_BC1_UNORM = 71,
    TCTEX_IFORMAT_COMPRESSED_BC1_SRGB = 72,
    TCTEX_IFORMAT_COMPRESSED_BC2_TYPELESS = 73,
    TCTEX_IFORMAT_COMPRESSED_BC2_UNORM = 74,
    TCTEX_IFORMAT_COMPRESSED_BC2_SRGB = 75,
    TCTEX_IFORMAT_COMPRESSED_BC3_TYPELESS = 76,
    TCTEX_IFORMAT_COMPRESSED_BC3_UNORM = 77,
    TCTEX_IFORMAT_COMPRESSED_BC3_SRGB = 78,
    TCTEX_IFORMAT_COMPRESSED_BC4_TYPELESS = 79,
    TCTEX_IFORMAT_COMPRESSED_BC4_UNORM = 80,
    TCTEX_IFORMAT_COMPRESSED_BC4_SNORM = 81,
    TCTEX_IFORMAT_COMPRESSED_BC5_TYPELESS = 82,
    TCTEX_IFORMAT_COMPRESSED_BC5_UNORM = 83,
    TCTEX_IFORMAT_COMPRESSED_BC5_SNORM = 84,
    TCTEX_IFORMAT_B5G6R5_UNORM = 85,
    TCTEX_IFORMAT_B5G5R5A1_UNORM = 86,
    TCTEX_IFORMAT_B8G8R8A8_UNORM = 87,
    TCTEX_IFORMAT_B8G8R8X8_UNORM = 88,
    TCTEX_IFORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    TCTEX_IFORMAT_B8G8R8A8_TYPELESS = 90,
    TCTEX_IFORMAT_B8G8R8A8_SRGB = 91,
    TCTEX_IFORMAT_B8G8R8X8_TYPELESS = 92,
    TCTEX_IFORMAT_B8G8R8X8_SRGB = 93,
    TCTEX_IFORMAT_COMPRESSED_BC6H_TYPELESS = 94,
    TCTEX_IFORMAT_COMPRESSED_BC6H_UFLOAT = 95,
    TCTEX_IFORMAT_COMPRESSED_BC6H_SFLOAT = 96,
    TCTEX_IFORMAT_COMPRESSED_BC7_TYPELESS = 97,
    TCTEX_IFORMAT_COMPRESSED_BC7_UNORM = 98,
    TCTEX_IFORMAT_COMPRESSED_BC7_SRGB = 99,
    TCTEX_IFORMAT_AYUV = 100,
    TCTEX_IFORMAT_Y410 = 101,
    TCTEX_IFORMAT_Y416 = 102,
    TCTEX_IFORMAT_NV12 = 103,
    TCTEX_IFORMAT_P010 = 104,
    TCTEX_IFORMAT_P016 = 105,
    TCTEX_IFORMAT_420_OPAQUE = 106,
    TCTEX_IFORMAT_YUY2 = 107,
    TCTEX_IFORMAT_Y210 = 108,
    TCTEX_IFORMAT_Y216 = 109,
    TCTEX_IFORMAT_NV11 = 110,
    TCTEX_IFORMAT_AI44 = 111,
    TCTEX_IFORMAT_IA44 = 112,
    TCTEX_IFORMAT_P8 = 113,
    TCTEX_IFORMAT_A8P8 = 114,
    TCTEX_IFORMAT_B4G4R4A4_UNORM = 115,
    TCTEX_IFORMAT_P208 = 130,
    TCTEX_IFORMAT_V208 = 131,
    TCTEX_IFORMAT_V408 = 132,
} TCTex_InternalFormat;
// TODO: rename these to descriptive once I figure out which is which (UP/DOWN/LEFT/RIGHT/...)
#define TCTEX_CUBE_FACE_POSX 0x01
#define TCTEX_CUBE_FACE_NEGX 0x02
#define TCTEX_CUBE_FACE_POSY 0x04
#define TCTEX_CUBE_FACE_NEGY 0x08
#define TCTEX_CUBE_FACE_POSZ 0x10
#define TCTEX_CUBE_FACE_NEGZ 0x20
// use it as: `(cubefaces.mask & TCTEX_CUBE_FACE_ALL) == TCTEX_CUBE_FACE_ALL`
#define TCTEX_CUBE_FACE_ALL  (TCTEX_CUBE_FACE_POSX|TCTEX_CUBE_FACE_NEGX|TCTEX_CUBE_FACE_POSY|TCTEX_CUBE_FACE_NEGY|TCTEX_CUBE_FACE_POSZ|TCTEX_CUBE_FACE_NEGZ)

typedef enum TCTex_AlphaMode
{
    /* these map 1:1 to DDS */
    TCTEX_ALPHA_MODE_UNKNOWN = 0,
    TCTEX_ALPHA_MODE_STRAIGHT = 1,
    TCTEX_ALPHA_MODE_PREMULTIPLIED = 2,
    TCTEX_ALPHA_MODE_OPAQUE = 3,
    TCTEX_ALPHA_MODE_CUSTOM = 4,
} TCTex_AlphaMode;

typedef struct TCTex_Texture
{
    const uint8_t* memory;
    uint32_t offset0;
    uint32_t nbytes;
    struct { uint32_t x, y, z; } size;
    struct { uint32_t y, z; } pitch;

    uint32_t arraylen;
    uint32_t nmiplevels;
    uint8_t dimension; // 1-3 (for TEXTURE_nD)
    struct
    {
        uint8_t num;
        uint8_t mask;
    } cubefaces;

    uint8_t alphamode;
    uint8_t isvolume: 1;
    uint8_t :7;

    TCTex_InternalFormat iformat;
    const char* errmsg;

    void* imem_; // internal!
} TCTex_Texture;

TCTex_Texture* tctex_load_mem(TCTex_Texture* tex, const void* data, size_t datalen);

#ifndef TC_TEXTURE_LOAD_NO_STDIO
TCTex_Texture* tctex_load_file(TCTex_Texture* tex, FILE* file);
TCTex_Texture* tctex_load_fname(TCTex_Texture* tex, const char* fname);
#endif

typedef struct TCTex_MipMapInfo
{
    uint64_t offset;
    uint32_t nbytes;
    struct { uint32_t x, y, z; } size;
    struct { uint32_t y, z; } pitch;
} TCTex_MipMapInfo;
// Get all the mipmaps for a texture at a specific array index.
// Cubemap order is: +X, -X, +Y, -Y, +Z, -Z.
uint32_t tctex_get_mipmaps(const TCTex_Texture* tex, TCTex_MipMapInfo* mipmaps, uint32_t maxmipmaps, uint32_t textureidx);

void tctex_close(TCTex_Texture* tex);

#ifdef TC_TEXTURE_LOAD_VULKAN_FORMATS
typedef struct TCTex_VK_FormatInfo
{
    uint32_t format;
    uint8_t is_approx: 1;
    uint8_t padding0_: 7;
} TCTex_VK_FormatInfo;
TCTex_VK_FormatInfo tctex_vk_get_formatinfo(const TCTex_Texture* tex);
#endif
#ifdef TC_TEXTURE_LOAD_OPENGL_FORMATS
typedef struct TCTex_GL_FormatInfo
{
    uint32_t baseInternalFormat;
    uint32_t internalFormat;
    uint32_t format;
    uint32_t type;
    uint16_t extensions;
    uint8_t is_approx: 1;
    uint8_t padding0_: 7;
} TCTex_GL_FormatInfo;
TCTex_GL_FormatInfo tctex_gl_get_formatinfo(const TCTex_Texture* tex);
#endif
#ifdef TC_TEXTURE_LOAD_DIRECT3D_FORMATS
typedef struct TCTex_D3D_FormatInfo
{
    uint32_t dxgiFormat;
} TCTex_D3D_FormatInfo;
TCTex_D3D_FormatInfo tctex_d3d_get_formatinfo(const TCTex_Texture* tex);
#endif

#ifdef __cplusplus
}
#endif

#endif /* TC_TEXTURE_LOAD_H_ */

#ifdef TC_TEXTURE_LOAD_IMPLEMENTATION

#include <stdlib.h> // for malloc, free
#include <stdbool.h>

#ifndef TC_TEXTURE_LOAD_NO_STDIO
#include <string.h> // for strerror(int)
#include <errno.h> // for int errno
#define TC_FTELL(stream)                ftell(stream)
#define TC_FSEEK(stream,offset,origin)  fseek(stream,offset,origin)
typedef long int TC_foffset;
#endif

#ifndef TC__STATIC_OR_RUNTIME_ASSERT
#if __STDC_VERSION__ >= 201112L
#define TC__STATIC_OR_RUNTIME_ASSERT(x,msg) _Static_assert(x,msg)
#elif __cplusplus >= 201103L
#define TC__STATIC_OR_RUNTIME_ASSERT(x,msg) static_assert(x,msg)
#else /* oh well ... fallback to runtime! */
#include <assert.h>
#define TC__STATIC_OR_RUNTIME_ASSERT(x,msg) assert((x) && (msg))
#endif
#endif

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

// TODO: clean this up.
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
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

typedef enum TCTex_DXGI_FORMAT {
    TCTEX_DXGI_FORMAT_UNKNOWN = 0,
    TCTEX_DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
    TCTEX_DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
    TCTEX_DXGI_FORMAT_R32G32B32A32_UINT = 3,
    TCTEX_DXGI_FORMAT_R32G32B32A32_SINT = 4,
    TCTEX_DXGI_FORMAT_R32G32B32_TYPELESS = 5,
    TCTEX_DXGI_FORMAT_R32G32B32_FLOAT = 6,
    TCTEX_DXGI_FORMAT_R32G32B32_UINT = 7,
    TCTEX_DXGI_FORMAT_R32G32B32_SINT = 8,
    TCTEX_DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
    TCTEX_DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
    TCTEX_DXGI_FORMAT_R16G16B16A16_UNORM = 11,
    TCTEX_DXGI_FORMAT_R16G16B16A16_UINT = 12,
    TCTEX_DXGI_FORMAT_R16G16B16A16_SNORM = 13,
    TCTEX_DXGI_FORMAT_R16G16B16A16_SINT = 14,
    TCTEX_DXGI_FORMAT_R32G32_TYPELESS = 15,
    TCTEX_DXGI_FORMAT_R32G32_FLOAT = 16,
    TCTEX_DXGI_FORMAT_R32G32_UINT = 17,
    TCTEX_DXGI_FORMAT_R32G32_SINT = 18,
    TCTEX_DXGI_FORMAT_R32G8X24_TYPELESS = 19,
    TCTEX_DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
    TCTEX_DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
    TCTEX_DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
    TCTEX_DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
    TCTEX_DXGI_FORMAT_R10G10B10A2_UNORM = 24,
    TCTEX_DXGI_FORMAT_R10G10B10A2_UINT = 25,
    TCTEX_DXGI_FORMAT_R11G11B10_FLOAT = 26,
    TCTEX_DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
    TCTEX_DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    TCTEX_DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    TCTEX_DXGI_FORMAT_R8G8B8A8_UINT = 30,
    TCTEX_DXGI_FORMAT_R8G8B8A8_SNORM = 31,
    TCTEX_DXGI_FORMAT_R8G8B8A8_SINT = 32,
    TCTEX_DXGI_FORMAT_R16G16_TYPELESS = 33,
    TCTEX_DXGI_FORMAT_R16G16_FLOAT = 34,
    TCTEX_DXGI_FORMAT_R16G16_UNORM = 35,
    TCTEX_DXGI_FORMAT_R16G16_UINT = 36,
    TCTEX_DXGI_FORMAT_R16G16_SNORM = 37,
    TCTEX_DXGI_FORMAT_R16G16_SINT = 38,
    TCTEX_DXGI_FORMAT_R32_TYPELESS = 39,
    TCTEX_DXGI_FORMAT_D32_FLOAT = 40,
    TCTEX_DXGI_FORMAT_R32_FLOAT = 41,
    TCTEX_DXGI_FORMAT_R32_UINT = 42,
    TCTEX_DXGI_FORMAT_R32_SINT = 43,
    TCTEX_DXGI_FORMAT_R24G8_TYPELESS = 44,
    TCTEX_DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
    TCTEX_DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
    TCTEX_DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
    TCTEX_DXGI_FORMAT_R8G8_TYPELESS = 48,
    TCTEX_DXGI_FORMAT_R8G8_UNORM = 49,
    TCTEX_DXGI_FORMAT_R8G8_UINT = 50,
    TCTEX_DXGI_FORMAT_R8G8_SNORM = 51,
    TCTEX_DXGI_FORMAT_R8G8_SINT = 52,
    TCTEX_DXGI_FORMAT_R16_TYPELESS = 53,
    TCTEX_DXGI_FORMAT_R16_FLOAT = 54,
    TCTEX_DXGI_FORMAT_D16_UNORM = 55,
    TCTEX_DXGI_FORMAT_R16_UNORM = 56,
    TCTEX_DXGI_FORMAT_R16_UINT = 57,
    TCTEX_DXGI_FORMAT_R16_SNORM = 58,
    TCTEX_DXGI_FORMAT_R16_SINT = 59,
    TCTEX_DXGI_FORMAT_R8_TYPELESS = 60,
    TCTEX_DXGI_FORMAT_R8_UNORM = 61,
    TCTEX_DXGI_FORMAT_R8_UINT = 62,
    TCTEX_DXGI_FORMAT_R8_SNORM = 63,
    TCTEX_DXGI_FORMAT_R8_SINT = 64,
    TCTEX_DXGI_FORMAT_A8_UNORM = 65,
    TCTEX_DXGI_FORMAT_R1_UNORM = 66,
    TCTEX_DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
    TCTEX_DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
    TCTEX_DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
    TCTEX_DXGI_FORMAT_BC1_TYPELESS = 70,
    TCTEX_DXGI_FORMAT_BC1_UNORM = 71,
    TCTEX_DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    TCTEX_DXGI_FORMAT_BC2_TYPELESS = 73,
    TCTEX_DXGI_FORMAT_BC2_UNORM = 74,
    TCTEX_DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    TCTEX_DXGI_FORMAT_BC3_TYPELESS = 76,
    TCTEX_DXGI_FORMAT_BC3_UNORM = 77,
    TCTEX_DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    TCTEX_DXGI_FORMAT_BC4_TYPELESS = 79,
    TCTEX_DXGI_FORMAT_BC4_UNORM = 80,
    TCTEX_DXGI_FORMAT_BC4_SNORM = 81,
    TCTEX_DXGI_FORMAT_BC5_TYPELESS = 82,
    TCTEX_DXGI_FORMAT_BC5_UNORM = 83,
    TCTEX_DXGI_FORMAT_BC5_SNORM = 84,
    TCTEX_DXGI_FORMAT_B5G6R5_UNORM = 85,
    TCTEX_DXGI_FORMAT_B5G5R5A1_UNORM = 86,
    TCTEX_DXGI_FORMAT_B8G8R8A8_UNORM = 87,
    TCTEX_DXGI_FORMAT_B8G8R8X8_UNORM = 88,
    TCTEX_DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    TCTEX_DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
    TCTEX_DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    TCTEX_DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
    TCTEX_DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    TCTEX_DXGI_FORMAT_BC6H_TYPELESS = 94,
    TCTEX_DXGI_FORMAT_BC6H_UF16 = 95,
    TCTEX_DXGI_FORMAT_BC6H_SF16 = 96,
    TCTEX_DXGI_FORMAT_BC7_TYPELESS = 97,
    TCTEX_DXGI_FORMAT_BC7_UNORM = 98,
    TCTEX_DXGI_FORMAT_BC7_UNORM_SRGB = 99,
    TCTEX_DXGI_FORMAT_AYUV = 100,
    TCTEX_DXGI_FORMAT_Y410 = 101,
    TCTEX_DXGI_FORMAT_Y416 = 102,
    TCTEX_DXGI_FORMAT_NV12 = 103,
    TCTEX_DXGI_FORMAT_P010 = 104,
    TCTEX_DXGI_FORMAT_P016 = 105,
    TCTEX_DXGI_FORMAT_420_OPAQUE = 106,
    TCTEX_DXGI_FORMAT_YUY2 = 107,
    TCTEX_DXGI_FORMAT_Y210 = 108,
    TCTEX_DXGI_FORMAT_Y216 = 109,
    TCTEX_DXGI_FORMAT_NV11 = 110,
    TCTEX_DXGI_FORMAT_AI44 = 111,
    TCTEX_DXGI_FORMAT_IA44 = 112,
    TCTEX_DXGI_FORMAT_P8 = 113,
    TCTEX_DXGI_FORMAT_A8P8 = 114,
    TCTEX_DXGI_FORMAT_B4G4R4A4_UNORM = 115,
    TCTEX_DXGI_FORMAT_P208 = 130,
    TCTEX_DXGI_FORMAT_V208 = 131,
    TCTEX_DXGI_FORMAT_V408 = 132,

    TCTEX_DXGI_FORMAT_MAX_ = 132,
} TCTex_DXGI_FORMAT;
// TC_DDS_PIXELFORMAT.dwFlags
#define TCTEX_DDPF_ALPHAPIXELS 0x1     // contains alpha data; dwRGBAlphaBitMask contains valid data
#define TCTEX_DDPF_ALPHA       0x2     // (older DDS) alpha channel only uncompressed; dwRGBBitCount contains bitcount, dwABitMask contains valid data
#define TCTEX_DDPF_FOURCC      0x4     // compressed RGB; dwFourCC contains valid data
#define TCTEX_DDPF_RGB         0x40    // uncompressed RGB; dwRGBBitCount and the RGB masks (dwRBitMask, dwGBitMask, dwBBitMask) contain valid data.
#define TCTEX_DDPF_YUV         0x200   // (older DDS) YUV uncompressed; dwRGBBitCount=>bit count, dwRBitMask=>Y mask, dwGBitMask=>U mask, dwBBitMask=>V mask
#define TCTEX_DDPF_LUMINANCE   0x20000 // (older DDS) single channel uncompressed; dwRGBBitCount=>bit count; dwRBitMask=>mask; can be combined with DDPF_ALPHAPIXELS for 2 channels
// TC_DDS_HEADER.dwFlags | NOTE: Do not rely on TC_DDSD_CAPS / TC_DDSD_PIXELFORMAT / TC_DDSD_MIPMAPCOUNT when reading!
#define TCTEX_DDSD_CAPS        0x1         // required
#define TCTEX_DDSD_HEIGHT      0x2         // required
#define TCTEX_DDSD_WIDTH       0x4         // required
#define TCTEX_DDSD_PITCH       0x8         // required when pitch provided for uncompressed
#define TCTEX_DDSD_PIXELFORMAT 0x1000      // required
#define TCTEX_DDSD_MIPMAPCOUNT 0x20000     // required when mipmapped texture
#define TCTEX_DDSD_LINEARSIZE  0x80000     // required when pitch provided for compressed
#define TCTEX_DDSD_DEPTH       0x800000    // required when depth texture
// TC_DDS_HEADER.dwCaps | NOTE: Do not rely on TC_DDSCAPS_COMPLEX / TC_DDSCAPS_TEXTURE when reading!
#define TCTEX_DDSCAPS_COMPLEX 0x8          // required when file contains more than 1 surface (mipmap, cubemap, or mipmapped volume texture)
#define TCTEX_DDSCAPS_MIPMAP  0x400000     // required when mipmap
#define TCTEX_DDSCAPS_TEXTURE 0x1000       // required
// TC_DDS_HEADER.dwCaps2
#define TCTEX_DDSCAPS2_CUBEMAP           0x200     // required when cubemap
#define TCTEX_DDSCAPS2_CUBEMAP_POSITIVEX 0x400     // required when stored in cubemap
#define TCTEX_DDSCAPS2_CUBEMAP_NEGATIVEX 0x800
#define TCTEX_DDSCAPS2_CUBEMAP_POSITIVEY 0x1000
#define TCTEX_DDSCAPS2_CUBEMAP_NEGATIVEY 0x2000
#define TCTEX_DDSCAPS2_CUBEMAP_POSITIVEZ 0x4000
#define TCTEX_DDSCAPS2_CUBEMAP_NEGATIVEZ 0x8000
#define TCTEX_DDSCAPS2_VOLUME            0x200000  // required when volume texture

// undocumented stuff
#define TCTEX_DDPF_BUMPDUDV 0x80000 //

// TC_DDS_HEADER_DXT10.resourceDimension
typedef enum TCTex_D3D10_RESOURCE_DIMENSION {
    TCTEX_D3D10_RESOURCE_DIMENSION_UNKNOWN    = 0,
    TCTEX_D3D10_RESOURCE_DIMENSION_BUFFER     = 1,
    TCTEX_D3D10_RESOURCE_DIMENSION_TEXTURE1D  = 2,
    TCTEX_D3D10_RESOURCE_DIMENSION_TEXTURE2D  = 3,
    TCTEX_D3D10_RESOURCE_DIMENSION_TEXTURE3D  = 4
} TCTex_D3D10_RESOURCE_DIMENSION;
#define TCTEX_DDS_DIMENSION_TEXTURE1D TCTEX_D3D10_RESOURCE_DIMENSION_TEXTURE1D
#define TCTEX_DDS_DIMENSION_TEXTURE2D TCTEX_D3D10_RESOURCE_DIMENSION_TEXTURE2D
#define TCTEX_DDS_DIMENSION_TEXTURE3D TCTEX_D3D10_RESOURCE_DIMENSION_TEXTURE3D
// TC_DDS_HEADER_DXT10.miscFlag
#define TCTEX_DDS_RESOURCE_MISC_TEXTURECUBE 0x4
// TC_DDS_HEADER_DXT10.miscFlags2
#define TCTEX_DDS_ALPHA_MODE_UNKNOWN       0x0
#define TCTEX_DDS_ALPHA_MODE_STRAIGHT      0x1
#define TCTEX_DDS_ALPHA_MODE_PREMULTIPLIED 0x2
#define TCTEX_DDS_ALPHA_MODE_OPAQUE        0x3
#define TCTEX_DDS_ALPHA_MODE_CUSTOM        0x4

// https://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx
typedef struct TCTex_DDS_PIXELFORMAT
{
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC; // basic: DXT1, DXT2, DXT3, DXT4, DXT5; extended: DX10
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
} TCTex_DDS_PIXELFORMAT;
typedef struct TCTex_DDS_HEADER
{
    uint32_t              dwSize;
    uint32_t              dwFlags;
    uint32_t              dwHeight;
    uint32_t              dwWidth;
    uint32_t              dwPitchOrLinearSize;
    uint32_t              dwDepth;
    uint32_t              dwMipMapCount;
    uint32_t              dwReserved1[11];
    TCTex_DDS_PIXELFORMAT ddspf;
    uint32_t              dwCaps;
    uint32_t              dwCaps2;
    uint32_t              dwCaps3;
    uint32_t              dwCaps4;
    uint32_t              dwReserved2;
} TCTex_DDS_HEADER;
typedef struct TCTex_DDS_HEADER_DXT10
{
  uint32_t dxgiFormat;
  uint32_t resourceDimension;
  uint32_t miscFlag;
  uint32_t arraySize;
  uint32_t miscFlags2;
} TCTex_DDS_HEADER_DXT10;

static uint32_t tctex_i_max32u(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}
static uint32_t tctex_i_min32u(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

#define TCTEX_I_RETERROR(tex,msg)   do { (tex)->errmsg = msg; return NULL; } while(0)

// source: bit twiddling hacks (https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightParallel)
static uint8_t tctex_i_count_trailing0(uint32_t v)
{
    uint8_t c = 32;
    v &= -v;
    if(v) c--;
    if(v & 0x0000FFFF) c -= 16;
    if(v & 0x00FF00FF) c -= 8;
    if(v & 0x0F0F0F0F) c -= 4;
    if(v & 0x33333333) c -= 2;
    if(v & 0x55555555) c -= 1;
    return c;
}
static uint8_t tctex_i_count_trailing1(uint32_t v)
{
    return tctex_i_count_trailing0(~v);
}
static void tctex_i_dds_get_cmask_info(uint8_t* shift, uint8_t* nbits, uint32_t mask)
{
    uint8_t cshift = tctex_i_count_trailing0(mask);
    if(cshift == 32)
    {
        *shift = *nbits = 0;
        return;
    }
    mask >>= cshift;
    uint8_t cnbits = tctex_i_count_trailing1(mask);

    // this tests if the bits are consecutive (no holes!)
    if((1U << cnbits) - 1U != mask)
    {
        *shift = 32;
        *nbits = 0;
        return;
    }
    *shift = cshift;
    *nbits = cnbits;
}
#define TCTEX_I_DDS_CHECKMASK_BGRA(shift,nbits,B,G,R,A) (                      \
        (nbits)[0]==(R) && (!(R)||(shift)[0]==(B)+(G))                         \
     && (nbits)[1]==(G) && (!(G)||(shift)[1]==(B))                             \
     && (nbits)[2]==(B) && (!(B)||(shift)[2]==0)                               \
     && (nbits)[3]==(A) && (!(A)||(shift)[3]==(B)+(G)+(R))                     \
    )
#define TCTEX_I_DDS_CHECKMASK_RGBA(shift,nbits,R,G,B,A) (                      \
        (nbits)[0]==(R) && (!(R)||(shift)[0]==0)                               \
     && (nbits)[1]==(G) && (!(G)||(shift)[1]==(R))                             \
     && (nbits)[2]==(B) && (!(B)||(shift)[2]==(R)+(G))                         \
     && (nbits)[3]==(A) && (!(A)||(shift)[3]==(R)+(G)+(B))                     \
    )
static TCTex_Texture* tctex_i_dds_handle_pixel_format(TCTex_Texture* tex, const TCTex_DDS_PIXELFORMAT* pformat, bool* hasDX10)
{
    TC__STATIC_OR_RUNTIME_ASSERT(sizeof(TCTex_DDS_PIXELFORMAT) == 32, "Invalid size of DDS_PIXELFORMAT structure");

    if(TCTEX__FROM_LE32(pformat->dwSize) != sizeof(TCTex_DDS_PIXELFORMAT)) TCTEX_I_RETERROR(tex, "Invalid DDS pixel format");

    uint32_t dwFlags = TCTEX__FROM_LE32(pformat->dwFlags);
    uint32_t dwFourCC;

    uint32_t dwRGBBitCount;
    uint8_t shift[4], nbits[4];
    size_t i;

    *hasDX10 = false;
    switch(dwFlags & (TCTEX_DDPF_ALPHA | TCTEX_DDPF_FOURCC | TCTEX_DDPF_RGB | TCTEX_DDPF_YUV | TCTEX_DDPF_LUMINANCE | TCTEX_DDPF_BUMPDUDV))
    {
    case TCTEX_DDPF_ALPHA:
        tex->alphamode = TCTEX_ALPHA_MODE_STRAIGHT;
        switch(TCTEX__FROM_LE32(pformat->dwRGBBitCount))
        {
        case 8:
            if(TCTEX__FROM_LE32(pformat->dwABitMask) != 0xFF) TCTEX_I_RETERROR(tex, "[TODO] UNHANDLED: Unknown alpha bit mask");
            tex->iformat = TCTEX_IFORMAT_A8_UNORM;
            break;
        default: TCTEX_I_RETERROR(tex, "[TODO] UNHANDLED: Unknown alpha bit count");
        }
        break;
    case TCTEX_DDPF_FOURCC:
        dwFourCC = TCTEX__FROM_LE32(pformat->dwFourCC);
        switch(dwFourCC)
        {
        case 0x30315844U: // "DX10"
            *hasDX10 = true;
            break;

        case 0x32495441U: // "ATI2"
        case 0x55354342U: // "BC5U"
            tex->iformat = TCTEX_IFORMAT_COMPRESSED_BC5_UNORM;
            break;
        case 0x53354342U: // "BC5S"
            tex->iformat = TCTEX_IFORMAT_COMPRESSED_BC5_SNORM;
            break;
        case 0x31495441U: // "ATI1"
        case 0x55344342U: // "BC4U"
            tex->iformat = TCTEX_IFORMAT_COMPRESSED_BC4_UNORM;
            break;
        case 0x53344342U: // "BC4S"
            tex->iformat = TCTEX_IFORMAT_COMPRESSED_BC4_SNORM;
            break;

        case 0x35545844U: // "DXT5"
            tex->iformat = TCTEX_IFORMAT_COMPRESSED_BC3_UNORM;
            tex->alphamode = TCTEX_ALPHA_MODE_STRAIGHT;
            break;
        case 0x34545844U: // "DXT4"
            tex->iformat = TCTEX_IFORMAT_COMPRESSED_BC3_UNORM;
            tex->alphamode = TCTEX_ALPHA_MODE_PREMULTIPLIED;
            break;
        case 0x33545844U: // "DXT3"
            tex->iformat = TCTEX_IFORMAT_COMPRESSED_BC2_UNORM;
            tex->alphamode = TCTEX_ALPHA_MODE_STRAIGHT;
            break;
        case 0x32545844U: // "DXT2"
            tex->iformat = TCTEX_IFORMAT_COMPRESSED_BC2_UNORM;
            tex->alphamode = TCTEX_ALPHA_MODE_PREMULTIPLIED;
            break;
        case 0x31545844U: // "DXT1"
            tex->iformat = TCTEX_IFORMAT_COMPRESSED_BC1_UNORM;
            tex->alphamode = TCTEX_ALPHA_MODE_PREMULTIPLIED;
            break;

        case 0x47424752U: // "RGBG"
            tex->iformat = TCTEX_IFORMAT_R8G8_B8G8_UNORM;
            break;
        case 0x42475247U: // "GRGB"
            tex->iformat = TCTEX_IFORMAT_G8R8_G8B8_UNORM;
            break;
        case 0x32595559: // "YUY2"
            tex->iformat = TCTEX_IFORMAT_YUY2;
            break;

        case 0x00000074U: // "t\0\0\0"
            tex->iformat = TCTEX_IFORMAT_R32G32B32A32_SFLOAT;
            break;
        case 0x00000073U: // "s\0\0\0"
            tex->iformat = TCTEX_IFORMAT_R32G32_SFLOAT;
            break;
        case 0x00000072U: // "r\0\0\0"
            tex->iformat = TCTEX_IFORMAT_R32_SFLOAT;
            break;
        case 0x00000071U: // "q\0\0\0"
            tex->iformat = TCTEX_IFORMAT_R16G16B16A16_SFLOAT;
            break;
        case 0x00000070U: // "p\0\0\0"
            tex->iformat = TCTEX_IFORMAT_R16G16_SFLOAT;
            break;
        case 0x0000006FU: // "o\0\0\0"
            tex->iformat = TCTEX_IFORMAT_R16_SFLOAT;
            break;
        case 0x0000006EU: // "N\0\0\0"
            tex->iformat = TCTEX_IFORMAT_R16G16B16A16_SNORM;
            break;
        case 0x00000024U: // "$\0\0\0"
            tex->iformat = TCTEX_IFORMAT_R16G16B16A16_UNORM;
            break;

        default: printf("FourCC %.4s 0x%.8X\n", (const char*)&dwFourCC, dwFourCC); TCTEX_I_RETERROR(tex, "Unknown DDS FourCC code");
        }
        break;
    case TCTEX_DDPF_RGB:
        dwRGBBitCount = TCTEX__FROM_LE32(pformat->dwRGBBitCount);
        tctex_i_dds_get_cmask_info(&shift[0], &nbits[0], TCTEX__FROM_LE32(pformat->dwRBitMask));
        tctex_i_dds_get_cmask_info(&shift[1], &nbits[1], TCTEX__FROM_LE32(pformat->dwGBitMask));
        tctex_i_dds_get_cmask_info(&shift[2], &nbits[2], TCTEX__FROM_LE32(pformat->dwBBitMask));
        if(dwFlags & TCTEX_DDPF_ALPHAPIXELS)
            tctex_i_dds_get_cmask_info(&shift[3], &nbits[3], TCTEX__FROM_LE32(pformat->dwABitMask));
        else
            shift[3] = nbits[3] = 0;
        for(i = 0; i < 4; i++)
            if(shift[i] == 32) TCTEX_I_RETERROR(tex, "Invalid DDS channel masks");
        switch(dwRGBBitCount)
        {
        case 0: TCTEX_I_RETERROR(tex, "Invalid DDS pixel bit count");
        case 16:
            if(TCTEX_I_DDS_CHECKMASK_BGRA(shift,nbits,5,6,5,0)) tex->iformat = TCTEX_IFORMAT_B5G6R5_UNORM;
            else if(TCTEX_I_DDS_CHECKMASK_BGRA(shift,nbits,5,5,5,1)) tex->iformat = TCTEX_IFORMAT_B5G5R5A1_UNORM;
            else if(TCTEX_I_DDS_CHECKMASK_BGRA(shift,nbits,4,4,4,4)) tex->iformat = TCTEX_IFORMAT_B4G4R4A4_UNORM;
            break;
        case 32:
            if(TCTEX_I_DDS_CHECKMASK_RGBA(shift,nbits,8,8,8,8)) tex->iformat = TCTEX_IFORMAT_R8G8B8A8_UNORM;
            else if(TCTEX_I_DDS_CHECKMASK_RGBA(shift,nbits,16,16,0,0)) tex->iformat = TCTEX_IFORMAT_R16G16_UNORM;
            else if(TCTEX_I_DDS_CHECKMASK_BGRA(shift,nbits,8,8,8,8)) tex->iformat = TCTEX_IFORMAT_B8G8R8A8_UNORM;
            else if(TCTEX_I_DDS_CHECKMASK_BGRA(shift,nbits,8,8,8,0)) tex->iformat = TCTEX_IFORMAT_B8G8R8X8_UNORM;
            break;
        }
        if(!tex->iformat) TCTEX_I_RETERROR(tex, "Unknown DDS channel masks (cannot convert to enum)");
        break;
    case TCTEX_DDPF_YUV: TCTEX_I_RETERROR(tex, "[TODO] UNHANDLED: YUV pixel format");
    case TCTEX_DDPF_LUMINANCE:
        dwRGBBitCount = TCTEX__FROM_LE32(pformat->dwRGBBitCount);
        tctex_i_dds_get_cmask_info(&shift[0], &nbits[0], TCTEX__FROM_LE32(pformat->dwRBitMask));
        if(dwFlags & TCTEX_DDPF_ALPHAPIXELS)
            tctex_i_dds_get_cmask_info(&shift[1], &nbits[1], TCTEX__FROM_LE32(pformat->dwABitMask));
        else
            shift[1] = nbits[1] = 0;
        for(i = 0; i < 4; i++)
            if(shift[i] == 32) TCTEX_I_RETERROR(tex, "Invalid DDS channel masks");
        switch(dwRGBBitCount)
        {
        case 0: TCTEX_I_RETERROR(tex, "Invalid DDS pixel bit count");
        case 8:
            if(TCTEX_I_DDS_CHECKMASK_RGBA(shift,nbits,8,0,0,0)) tex->iformat = TCTEX_IFORMAT_R8_UNORM;
            break;
        case 16:
            if(TCTEX_I_DDS_CHECKMASK_RGBA(shift,nbits,8,8,0,0)) tex->iformat = TCTEX_IFORMAT_R8G8_UNORM;
            else if(TCTEX_I_DDS_CHECKMASK_RGBA(shift,nbits,16,0,0,0)) tex->iformat = TCTEX_IFORMAT_R16_UNORM;
            break;
        }
        if(!tex->iformat) TCTEX_I_RETERROR(tex, "Unknown DDS channel masks (cannot convert to enum)");
        break;
    case TCTEX_DDPF_BUMPDUDV:
        dwRGBBitCount = TCTEX__FROM_LE32(pformat->dwRGBBitCount);
        tctex_i_dds_get_cmask_info(&shift[0], &nbits[0], TCTEX__FROM_LE32(pformat->dwRBitMask));
        tctex_i_dds_get_cmask_info(&shift[1], &nbits[1], TCTEX__FROM_LE32(pformat->dwGBitMask));
        tctex_i_dds_get_cmask_info(&shift[2], &nbits[2], TCTEX__FROM_LE32(pformat->dwBBitMask));
        tctex_i_dds_get_cmask_info(&shift[3], &nbits[3], TCTEX__FROM_LE32(pformat->dwABitMask));
        for(i = 0; i < 4; i++)
            if(shift[i] == 32) TCTEX_I_RETERROR(tex, "Invalid DDS channel masks");
        switch(dwRGBBitCount)
        {
        case 0: TCTEX_I_RETERROR(tex, "Invalid DDS pixel bit count");
        case 16:
            if(TCTEX_I_DDS_CHECKMASK_RGBA(shift,nbits,8,8,0,0)) tex->iformat = TCTEX_IFORMAT_R8G8_SNORM;
            break;
        case 32:
            if(TCTEX_I_DDS_CHECKMASK_RGBA(shift,nbits,8,8,8,8)) tex->iformat = TCTEX_IFORMAT_R8G8B8A8_SNORM;
            else if(TCTEX_I_DDS_CHECKMASK_RGBA(shift,nbits,16,16,0,0)) tex->iformat = TCTEX_IFORMAT_R16G16_SNORM;
            break;
        }
        if(!tex->iformat) TCTEX_I_RETERROR(tex, "Unknown DDS channel masks (cannot convert to enum)");
        break;
    default:
        TCTEX_I_RETERROR(tex, "Invalid DDS pixel format (conflicting flags)");
    }
    return tex;
}
static TCTex_Texture* tctex_i_dds_handle_header10(TCTex_Texture* tex, const TCTex_DDS_HEADER_DXT10* header10)
{
    uint32_t dxgiFormat = TCTEX__FROM_LE32(header10->dxgiFormat);
    if(dxgiFormat > TCTEX_DXGI_FORMAT_MAX_) TCTEX_I_RETERROR(tex, "Invalid DDS file (unknown internal format)");
    tex->iformat = dxgiFormat;

    switch(TCTEX__FROM_LE32(header10->resourceDimension))
    {
    case TCTEX_DDS_DIMENSION_TEXTURE1D: tex->dimension = 1; break;
    case TCTEX_DDS_DIMENSION_TEXTURE2D: tex->dimension = 2; break;
    case TCTEX_DDS_DIMENSION_TEXTURE3D: tex->dimension = 3; break;
    default: TCTEX_I_RETERROR(tex, "Invalid DDS file (unknown or missing resource dimension)");
    }

    uint32_t miscFlag = TCTEX__FROM_LE32(header10->miscFlag);
    if(miscFlag & TCTEX_DDS_RESOURCE_MISC_TEXTURECUBE)
    {
        if(!tex->cubefaces.num) TCTEX_I_RETERROR(tex, "Invalid DDS file (cubemap with no defined cubemap faces)");
        if(tex->dimension != 2) TCTEX_I_RETERROR(tex, "Invalid DDS file (cubemap with dimension != 2)");
    }

    tex->arraylen = TCTEX__FROM_LE32(header10->arraySize);
    if(tex->dimension == 3 && tex->arraylen != 1) TCTEX_I_RETERROR(tex, "Invalid DDS file (arrays of 3D textures are not permitted)");

    uint32_t miscFlags2 = TCTEX__FROM_LE32(header10->miscFlags2);
    uint32_t alphaMode = miscFlags2 & 0x7;
    switch(alphaMode)
    {
    case TCTEX_DDS_ALPHA_MODE_UNKNOWN:
    case TCTEX_DDS_ALPHA_MODE_STRAIGHT:
    case TCTEX_DDS_ALPHA_MODE_PREMULTIPLIED:
    case TCTEX_DDS_ALPHA_MODE_OPAQUE:
    case TCTEX_DDS_ALPHA_MODE_CUSTOM:
        tex->alphamode = alphaMode;
        break;
    default: TCTEX_I_RETERROR(tex, "Invalid DDS file (invalid alpha mode)");
    }
    return tex;
}

static TCTex_Texture* tctex_i_dds_load(TCTex_Texture* tex, const uint8_t* udata, size_t udataoffset, size_t udatalen)
{
    TC__STATIC_OR_RUNTIME_ASSERT(sizeof(TCTex_DDS_HEADER) == 124, "Invalid size of DDS_HEADER structure");
    udata += udataoffset;
    udatalen -= udataoffset;
    const TCTex_DDS_HEADER* header = TC__STATIC_CAST(const TCTex_DDS_HEADER*,udata);

    uint32_t dwSize = TCTEX__FROM_LE32(header->dwSize);
    if(dwSize < sizeof(TCTex_DDS_HEADER)) TCTEX_I_RETERROR(tex, "Invalid DDS file (header truncated)");

    /* I don't like Hungarian notation either, but for consistency with DDS_HEADER, I'll maintain the names ... */
    uint32_t dwFlags = TCTEX__FROM_LE32(header->dwFlags);
    if((dwFlags & (TCTEX_DDSD_HEIGHT | TCTEX_DDSD_WIDTH)) != (TCTEX_DDSD_HEIGHT | TCTEX_DDSD_WIDTH)) TCTEX_I_RETERROR(tex, "Invallid DDS file (invalid file flags)");

    tex->size.x = TCTEX__FROM_LE32(header->dwWidth);
    tex->size.y = TCTEX__FROM_LE32(header->dwHeight);
    tex->size.z = (dwFlags & TCTEX_DDSD_DEPTH) ? TCTEX__FROM_LE32(header->dwDepth) : 1;

    uint32_t dwPitchOrLinearSize = TCTEX__FROM_LE32(header->dwPitchOrLinearSize);
    switch(dwFlags & (TCTEX_DDSD_PITCH | TCTEX_DDSD_LINEARSIZE))
    {
    case 0: TCTEX_I_RETERROR(tex, "[TODO] UNHANDLED: Compute pitch *and* linear size");
    case TCTEX_DDSD_PITCH:
        tex->pitch.y = TCTEX__FROM_LE32(dwPitchOrLinearSize);
        tex->pitch.z = tex->size.y * tex->pitch.y;
        tex->nbytes = tex->size.z * tex->pitch.z;
        break;
    case TCTEX_DDSD_LINEARSIZE:
        tex->nbytes = TCTEX__FROM_LE32(header->dwPitchOrLinearSize);
        tex->pitch.y = tex->nbytes / tex->size.y;//TODO: verify
        tex->pitch.z = tex->nbytes;
        tex->nbytes *= tex->size.z;
        break;
    default: TCTEX_I_RETERROR(tex, "Invalid DDS file (conflicting file flags)");
    }
    tex->nmiplevels = TCTEX__FROM_LE32(header->dwMipMapCount);

    bool hasDX10;
    if(!tctex_i_dds_handle_pixel_format(tex, &header->ddspf, &hasDX10)) return NULL;

    uint32_t dwCaps = TCTEX__FROM_LE32(header->dwCaps);
    if(!(dwCaps & TCTEX_DDSCAPS_MIPMAP))
    {
        if(tex->nmiplevels > 1) TCTEX_I_RETERROR(tex, "Conflicting DDS flags (has no mipmaps, but defines more than 1 anyway)");
        else tex->nmiplevels = 1;
    }
    uint32_t dwCaps2 = TCTEX__FROM_LE32(header->dwCaps2);
    if(dwCaps2 & TCTEX_DDSCAPS2_CUBEMAP)
    {
        if(dwCaps2 & TCTEX_DDSCAPS2_CUBEMAP_POSITIVEX) { tex->cubefaces.mask |= TCTEX_CUBE_FACE_POSX; tex->cubefaces.num++; }
        if(dwCaps2 & TCTEX_DDSCAPS2_CUBEMAP_NEGATIVEX) { tex->cubefaces.mask |= TCTEX_CUBE_FACE_NEGX; tex->cubefaces.num++; }
        if(dwCaps2 & TCTEX_DDSCAPS2_CUBEMAP_POSITIVEY) { tex->cubefaces.mask |= TCTEX_CUBE_FACE_POSY; tex->cubefaces.num++; }
        if(dwCaps2 & TCTEX_DDSCAPS2_CUBEMAP_NEGATIVEY) { tex->cubefaces.mask |= TCTEX_CUBE_FACE_NEGY; tex->cubefaces.num++; }
        if(dwCaps2 & TCTEX_DDSCAPS2_CUBEMAP_POSITIVEZ) { tex->cubefaces.mask |= TCTEX_CUBE_FACE_POSZ; tex->cubefaces.num++; }
        if(dwCaps2 & TCTEX_DDSCAPS2_CUBEMAP_NEGATIVEZ) { tex->cubefaces.mask |= TCTEX_CUBE_FACE_NEGZ; tex->cubefaces.num++; }
        if(!tex->cubefaces.num) TCTEX_I_RETERROR(tex, "Conflicting DDS flags (is cubemap, but no faces defined)");
    }
    tex->isvolume = !!(dwCaps2 & TCTEX_DDSCAPS2_VOLUME);// TODO: do we really need this? we have ->dimension and ->size.z

    tex->offset0 = udataoffset + dwSize;
    if(hasDX10)
    {
        TC__STATIC_OR_RUNTIME_ASSERT(sizeof(TCTex_DDS_HEADER_DXT10) == 20, "Invalid size of DDS_HEADER_DXT10 structure");
        if(udatalen < dwSize + sizeof(TCTex_DDS_HEADER_DXT10)) TCTEX_I_RETERROR(tex, "Invalid DDS file (file truncated)");

        if(!tctex_i_dds_handle_header10(tex, TC__STATIC_CAST(const TCTex_DDS_HEADER_DXT10*,udata + dwSize))) return NULL;
        tex->offset0 += sizeof(TCTex_DDS_HEADER_DXT10);
    }
    else
    {
        tex->arraylen = 1;
        tex->dimension = 2;
    }
    return tex;
}

TCTex_Texture* tctex_load_mem(TCTex_Texture* tex, const void* data, size_t datalen)
{
    static const TCTex_Texture Initial = {};

    if(!tex) return NULL;
    *tex = Initial;
    if(datalen < 4 + sizeof(TCTex_DDS_HEADER)) TCTEX_I_RETERROR(tex, "Invalid file (not a DDS file or file truncated)");

    tex->memory = TC__VOID_CAST(const uint8_t*,data);

    if(TCTEX__FROM_LE32(*TC__STATIC_CAST(const uint32_t*,data)) == 0x20534444U /* "DDS " */) return tctex_i_dds_load(tex, tex->memory, 4, datalen);
    tex->errmsg = "Invalid filetype (not a DDS file)";
    return NULL;
}
#ifndef TC_TEXTURE_LOAD_NO_STDIO
TCTex_Texture* tctex_load_file(TCTex_Texture* tex, FILE* file)
{
    if(!tex) return NULL;

    TC_foffset head, tail;
    if((head = TC_FTELL(file)) < 0) return NULL;
    if(TC_FSEEK(file, 0, SEEK_END)) return NULL;
    if((tail = TC_FTELL(file)) < 0) return NULL;
    if(TC_FSEEK(file, head, SEEK_SET)) return NULL;

    TC_foffset len = tail - head;
    tex->imem_ = malloc(len);
    if(!tex->imem_) { tex->errmsg = "Unable to allocate memory"; return NULL; }
    if(fread(tex->imem_, 1, len, file) != len) { tex->errmsg = strerror(errno); goto err_freemem; }
    if(!tctex_load_mem(tex, tex->imem_, len)) goto err_freemem;
    return tex;
err_freemem:
    free(tex->imem_);
    return NULL;
}
TCTex_Texture* tctex_load_fname(TCTex_Texture* tex, const char* fname)
{
    if(!tex) return NULL;
    FILE* file = fopen(fname, "rb");
    if(!file)
    {
        tex->errmsg = strerror(errno);
        return NULL;
    }
    tex = tctex_load_file(tex, file);
    fclose(file);
    return tex;
}
#endif

uint32_t tctex_get_mipmaps(const TCTex_Texture* tex, TCTex_MipMapInfo* mipmaps, uint32_t maxmipmaps, uint32_t textureidx)
{
    uint32_t nmipmaps = tctex_i_min32u(tex->nmiplevels, maxmipmaps);

    TCTex_MipMapInfo mipmap;
    mipmap.offset = 0;
    mipmap.size.x = tex->size.x;
    mipmap.size.y = tex->size.y;
    mipmap.size.z = tex->size.z;
    mipmap.pitch.y = tex->pitch.y;
    mipmap.pitch.z = tex->pitch.z;
    mipmap.nbytes = tex->nbytes;

    uint32_t i;
    // first, compute the base info
    for(i = 0; i < nmipmaps; i++)
    {
        mipmaps[i] = mipmap;
        mipmap.offset += mipmap.nbytes;
        mipmap.size.x = tctex_i_max32u(mipmap.size.x >> 1, 1U);
        mipmap.size.y = tctex_i_max32u(mipmap.size.y >> 1, 1U);
        mipmap.size.z = tctex_i_max32u(mipmap.size.z >> 1, 1U);
        //mipmap.pitch.y = tctex_i_max32u(((mipmap.pitch.y >> 1) + 3U) & ~3U, 4U);
        mipmap.pitch.y = tctex_i_max32u(mipmap.pitch.y >> 1, 1U);
        mipmap.pitch.z = mipmap.size.y * mipmap.pitch.y;
        mipmap.nbytes = mipmap.size.z * mipmap.pitch.z;
    }
    // mipmap.offset now stores the total size!
    for(i = 0; i < nmipmaps; i++) // now, adjust all offsets
        mipmaps[i].offset += tex->offset0 + textureidx * mipmap.offset;
    return nmipmaps;
}

void tctex_close(TCTex_Texture* tex)
{
    if(!tex) return;
    if(tex->imem_) free(tex->imem_);
}

#ifdef TC_TEXTURE_LOAD_VULKAN_FORMATS
typedef enum TCTex_VkFormat {
    TCTEX_VK_FORMAT_UNDEFINED = 0,
    TCTEX_VK_FORMAT_R4G4_UNORM_PACK8 = 1,
    TCTEX_VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
    TCTEX_VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
    TCTEX_VK_FORMAT_R5G6B5_UNORM_PACK16 = 4,
    TCTEX_VK_FORMAT_B5G6R5_UNORM_PACK16 = 5,
    TCTEX_VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
    TCTEX_VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
    TCTEX_VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
    TCTEX_VK_FORMAT_R8_UNORM = 9,
    TCTEX_VK_FORMAT_R8_SNORM = 10,
    TCTEX_VK_FORMAT_R8_USCALED = 11,
    TCTEX_VK_FORMAT_R8_SSCALED = 12,
    TCTEX_VK_FORMAT_R8_UINT = 13,
    TCTEX_VK_FORMAT_R8_SINT = 14,
    TCTEX_VK_FORMAT_R8_SRGB = 15,
    TCTEX_VK_FORMAT_R8G8_UNORM = 16,
    TCTEX_VK_FORMAT_R8G8_SNORM = 17,
    TCTEX_VK_FORMAT_R8G8_USCALED = 18,
    TCTEX_VK_FORMAT_R8G8_SSCALED = 19,
    TCTEX_VK_FORMAT_R8G8_UINT = 20,
    TCTEX_VK_FORMAT_R8G8_SINT = 21,
    TCTEX_VK_FORMAT_R8G8_SRGB = 22,
    TCTEX_VK_FORMAT_R8G8B8_UNORM = 23,
    TCTEX_VK_FORMAT_R8G8B8_SNORM = 24,
    TCTEX_VK_FORMAT_R8G8B8_USCALED = 25,
    TCTEX_VK_FORMAT_R8G8B8_SSCALED = 26,
    TCTEX_VK_FORMAT_R8G8B8_UINT = 27,
    TCTEX_VK_FORMAT_R8G8B8_SINT = 28,
    TCTEX_VK_FORMAT_R8G8B8_SRGB = 29,
    TCTEX_VK_FORMAT_B8G8R8_UNORM = 30,
    TCTEX_VK_FORMAT_B8G8R8_SNORM = 31,
    TCTEX_VK_FORMAT_B8G8R8_USCALED = 32,
    TCTEX_VK_FORMAT_B8G8R8_SSCALED = 33,
    TCTEX_VK_FORMAT_B8G8R8_UINT = 34,
    TCTEX_VK_FORMAT_B8G8R8_SINT = 35,
    TCTEX_VK_FORMAT_B8G8R8_SRGB = 36,
    TCTEX_VK_FORMAT_R8G8B8A8_UNORM = 37,
    TCTEX_VK_FORMAT_R8G8B8A8_SNORM = 38,
    TCTEX_VK_FORMAT_R8G8B8A8_USCALED = 39,
    TCTEX_VK_FORMAT_R8G8B8A8_SSCALED = 40,
    TCTEX_VK_FORMAT_R8G8B8A8_UINT = 41,
    TCTEX_VK_FORMAT_R8G8B8A8_SINT = 42,
    TCTEX_VK_FORMAT_R8G8B8A8_SRGB = 43,
    TCTEX_VK_FORMAT_B8G8R8A8_UNORM = 44,
    TCTEX_VK_FORMAT_B8G8R8A8_SNORM = 45,
    TCTEX_VK_FORMAT_B8G8R8A8_USCALED = 46,
    TCTEX_VK_FORMAT_B8G8R8A8_SSCALED = 47,
    TCTEX_VK_FORMAT_B8G8R8A8_UINT = 48,
    TCTEX_VK_FORMAT_B8G8R8A8_SINT = 49,
    TCTEX_VK_FORMAT_B8G8R8A8_SRGB = 50,
    TCTEX_VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
    TCTEX_VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
    TCTEX_VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
    TCTEX_VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
    TCTEX_VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
    TCTEX_VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
    TCTEX_VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
    TCTEX_VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
    TCTEX_VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
    TCTEX_VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
    TCTEX_VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
    TCTEX_VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
    TCTEX_VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
    TCTEX_VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
    TCTEX_VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
    TCTEX_VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
    TCTEX_VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
    TCTEX_VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
    TCTEX_VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
    TCTEX_VK_FORMAT_R16_UNORM = 70,
    TCTEX_VK_FORMAT_R16_SNORM = 71,
    TCTEX_VK_FORMAT_R16_USCALED = 72,
    TCTEX_VK_FORMAT_R16_SSCALED = 73,
    TCTEX_VK_FORMAT_R16_UINT = 74,
    TCTEX_VK_FORMAT_R16_SINT = 75,
    TCTEX_VK_FORMAT_R16_SFLOAT = 76,
    TCTEX_VK_FORMAT_R16G16_UNORM = 77,
    TCTEX_VK_FORMAT_R16G16_SNORM = 78,
    TCTEX_VK_FORMAT_R16G16_USCALED = 79,
    TCTEX_VK_FORMAT_R16G16_SSCALED = 80,
    TCTEX_VK_FORMAT_R16G16_UINT = 81,
    TCTEX_VK_FORMAT_R16G16_SINT = 82,
    TCTEX_VK_FORMAT_R16G16_SFLOAT = 83,
    TCTEX_VK_FORMAT_R16G16B16_UNORM = 84,
    TCTEX_VK_FORMAT_R16G16B16_SNORM = 85,
    TCTEX_VK_FORMAT_R16G16B16_USCALED = 86,
    TCTEX_VK_FORMAT_R16G16B16_SSCALED = 87,
    TCTEX_VK_FORMAT_R16G16B16_UINT = 88,
    TCTEX_VK_FORMAT_R16G16B16_SINT = 89,
    TCTEX_VK_FORMAT_R16G16B16_SFLOAT = 90,
    TCTEX_VK_FORMAT_R16G16B16A16_UNORM = 91,
    TCTEX_VK_FORMAT_R16G16B16A16_SNORM = 92,
    TCTEX_VK_FORMAT_R16G16B16A16_USCALED = 93,
    TCTEX_VK_FORMAT_R16G16B16A16_SSCALED = 94,
    TCTEX_VK_FORMAT_R16G16B16A16_UINT = 95,
    TCTEX_VK_FORMAT_R16G16B16A16_SINT = 96,
    TCTEX_VK_FORMAT_R16G16B16A16_SFLOAT = 97,
    TCTEX_VK_FORMAT_R32_UINT = 98,
    TCTEX_VK_FORMAT_R32_SINT = 99,
    TCTEX_VK_FORMAT_R32_SFLOAT = 100,
    TCTEX_VK_FORMAT_R32G32_UINT = 101,
    TCTEX_VK_FORMAT_R32G32_SINT = 102,
    TCTEX_VK_FORMAT_R32G32_SFLOAT = 103,
    TCTEX_VK_FORMAT_R32G32B32_UINT = 104,
    TCTEX_VK_FORMAT_R32G32B32_SINT = 105,
    TCTEX_VK_FORMAT_R32G32B32_SFLOAT = 106,
    TCTEX_VK_FORMAT_R32G32B32A32_UINT = 107,
    TCTEX_VK_FORMAT_R32G32B32A32_SINT = 108,
    TCTEX_VK_FORMAT_R32G32B32A32_SFLOAT = 109,
    TCTEX_VK_FORMAT_R64_UINT = 110,
    TCTEX_VK_FORMAT_R64_SINT = 111,
    TCTEX_VK_FORMAT_R64_SFLOAT = 112,
    TCTEX_VK_FORMAT_R64G64_UINT = 113,
    TCTEX_VK_FORMAT_R64G64_SINT = 114,
    TCTEX_VK_FORMAT_R64G64_SFLOAT = 115,
    TCTEX_VK_FORMAT_R64G64B64_UINT = 116,
    TCTEX_VK_FORMAT_R64G64B64_SINT = 117,
    TCTEX_VK_FORMAT_R64G64B64_SFLOAT = 118,
    TCTEX_VK_FORMAT_R64G64B64A64_UINT = 119,
    TCTEX_VK_FORMAT_R64G64B64A64_SINT = 120,
    TCTEX_VK_FORMAT_R64G64B64A64_SFLOAT = 121,
    TCTEX_VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
    TCTEX_VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,
    TCTEX_VK_FORMAT_D16_UNORM = 124,
    TCTEX_VK_FORMAT_X8_D24_UNORM_PACK32 = 125,
    TCTEX_VK_FORMAT_D32_SFLOAT = 126,
    TCTEX_VK_FORMAT_S8_UINT = 127,
    TCTEX_VK_FORMAT_D16_UNORM_S8_UINT = 128,
    TCTEX_VK_FORMAT_D24_UNORM_S8_UINT = 129,
    TCTEX_VK_FORMAT_D32_SFLOAT_S8_UINT = 130,
    TCTEX_VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131,
    TCTEX_VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132,
    TCTEX_VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
    TCTEX_VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134,
    TCTEX_VK_FORMAT_BC2_UNORM_BLOCK = 135,
    TCTEX_VK_FORMAT_BC2_SRGB_BLOCK = 136,
    TCTEX_VK_FORMAT_BC3_UNORM_BLOCK = 137,
    TCTEX_VK_FORMAT_BC3_SRGB_BLOCK = 138,
    TCTEX_VK_FORMAT_BC4_UNORM_BLOCK = 139,
    TCTEX_VK_FORMAT_BC4_SNORM_BLOCK = 140,
    TCTEX_VK_FORMAT_BC5_UNORM_BLOCK = 141,
    TCTEX_VK_FORMAT_BC5_SNORM_BLOCK = 142,
    TCTEX_VK_FORMAT_BC6H_UFLOAT_BLOCK = 143,
    TCTEX_VK_FORMAT_BC6H_SFLOAT_BLOCK = 144,
    TCTEX_VK_FORMAT_BC7_UNORM_BLOCK = 145,
    TCTEX_VK_FORMAT_BC7_SRGB_BLOCK = 146,
    TCTEX_VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147,
    TCTEX_VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148,
    TCTEX_VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
    TCTEX_VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
    TCTEX_VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
    TCTEX_VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152,
    TCTEX_VK_FORMAT_EAC_R11_UNORM_BLOCK = 153,
    TCTEX_VK_FORMAT_EAC_R11_SNORM_BLOCK = 154,
    TCTEX_VK_FORMAT_EAC_R11G11_UNORM_BLOCK = 155,
    TCTEX_VK_FORMAT_EAC_R11G11_SNORM_BLOCK = 156,
    TCTEX_VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 157,
    TCTEX_VK_FORMAT_ASTC_4x4_SRGB_BLOCK = 158,
    TCTEX_VK_FORMAT_ASTC_5x4_UNORM_BLOCK = 159,
    TCTEX_VK_FORMAT_ASTC_5x4_SRGB_BLOCK = 160,
    TCTEX_VK_FORMAT_ASTC_5x5_UNORM_BLOCK = 161,
    TCTEX_VK_FORMAT_ASTC_5x5_SRGB_BLOCK = 162,
    TCTEX_VK_FORMAT_ASTC_6x5_UNORM_BLOCK = 163,
    TCTEX_VK_FORMAT_ASTC_6x5_SRGB_BLOCK = 164,
    TCTEX_VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 165,
    TCTEX_VK_FORMAT_ASTC_6x6_SRGB_BLOCK = 166,
    TCTEX_VK_FORMAT_ASTC_8x5_UNORM_BLOCK = 167,
    TCTEX_VK_FORMAT_ASTC_8x5_SRGB_BLOCK = 168,
    TCTEX_VK_FORMAT_ASTC_8x6_UNORM_BLOCK = 169,
    TCTEX_VK_FORMAT_ASTC_8x6_SRGB_BLOCK = 170,
    TCTEX_VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 171,
    TCTEX_VK_FORMAT_ASTC_8x8_SRGB_BLOCK = 172,
    TCTEX_VK_FORMAT_ASTC_10x5_UNORM_BLOCK = 173,
    TCTEX_VK_FORMAT_ASTC_10x5_SRGB_BLOCK = 174,
    TCTEX_VK_FORMAT_ASTC_10x6_UNORM_BLOCK = 175,
    TCTEX_VK_FORMAT_ASTC_10x6_SRGB_BLOCK = 176,
    TCTEX_VK_FORMAT_ASTC_10x8_UNORM_BLOCK = 177,
    TCTEX_VK_FORMAT_ASTC_10x8_SRGB_BLOCK = 178,
    TCTEX_VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 179,
    TCTEX_VK_FORMAT_ASTC_10x10_SRGB_BLOCK = 180,
    TCTEX_VK_FORMAT_ASTC_12x10_UNORM_BLOCK = 181,
    TCTEX_VK_FORMAT_ASTC_12x10_SRGB_BLOCK = 182,
    TCTEX_VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 183,
    TCTEX_VK_FORMAT_ASTC_12x12_SRGB_BLOCK = 184,
} TCTex_VkFormat;
static const TCTex_VK_FormatInfo tctex_i_vk_formats[] = {
    {TCTEX_VK_FORMAT_UNDEFINED,0},
    {TCTEX_VK_FORMAT_R32G32B32A32_UINT,1},
    {TCTEX_VK_FORMAT_R32G32B32A32_SFLOAT,0},
    {TCTEX_VK_FORMAT_R32G32B32A32_UINT,0},
    {TCTEX_VK_FORMAT_R32G32B32A32_SINT,0},
    {TCTEX_VK_FORMAT_R32G32B32_UINT,1},
    {TCTEX_VK_FORMAT_R32G32B32_SFLOAT,0},
    {TCTEX_VK_FORMAT_R32G32B32_UINT,0},
    {TCTEX_VK_FORMAT_R32G32B32_SINT,0},
    {TCTEX_VK_FORMAT_R16G16B16A16_UINT,1},
    {TCTEX_VK_FORMAT_R16G16B16A16_SFLOAT,0},
    {TCTEX_VK_FORMAT_R16G16B16A16_UNORM,0},
    {TCTEX_VK_FORMAT_R16G16B16A16_UINT,0},
    {TCTEX_VK_FORMAT_R16G16B16A16_SNORM,0},
    {TCTEX_VK_FORMAT_R16G16B16A16_SINT,0},
    {TCTEX_VK_FORMAT_R32G32_UINT,1},
    {TCTEX_VK_FORMAT_R32G32_SFLOAT,0},
    {TCTEX_VK_FORMAT_R32G32_UINT,0},
    {TCTEX_VK_FORMAT_R32G32_SINT,0},
    {TCTEX_VK_FORMAT_D32_SFLOAT_S8_UINT,1},//TODO:verify
    {TCTEX_VK_FORMAT_D32_SFLOAT_S8_UINT,0},
    {TCTEX_VK_FORMAT_D32_SFLOAT_S8_UINT,1},
    {TCTEX_VK_FORMAT_D32_SFLOAT_S8_UINT,1},
    {},//TCTEX_DXGI_FORMAT_R10G10B10A2_TYPELESS
    {},//TCTEX_DXGI_FORMAT_R10G10B10A2_UNORM
    {},//TCTEX_DXGI_FORMAT_R10G10B10A2_UINT
    {},//TCTEX_DXGI_FORMAT_R11G11B10_FLOAT
    {TCTEX_VK_FORMAT_R8G8B8A8_UINT,1},
    {TCTEX_VK_FORMAT_R8G8B8A8_UNORM,0},
    {TCTEX_VK_FORMAT_R8G8B8A8_SRGB,0},
    {TCTEX_VK_FORMAT_R8G8B8A8_UINT,0},
    {TCTEX_VK_FORMAT_R8G8B8A8_SNORM,0},
    {TCTEX_VK_FORMAT_R8G8B8A8_SINT,0},
    {TCTEX_VK_FORMAT_R16G16_UINT,1},
    {TCTEX_VK_FORMAT_R16G16_SFLOAT,0},
    {TCTEX_VK_FORMAT_R16G16_UNORM,0},
    {TCTEX_VK_FORMAT_R16G16_UINT,0},
    {TCTEX_VK_FORMAT_R16G16_SNORM,0},
    {TCTEX_VK_FORMAT_R16G16_SINT,0},
    {TCTEX_VK_FORMAT_R32_UINT,1},
    {TCTEX_VK_FORMAT_D32_SFLOAT,0},
    {TCTEX_VK_FORMAT_R32_SFLOAT,0},
    {TCTEX_VK_FORMAT_R32_UINT,0},
    {TCTEX_VK_FORMAT_R32_SINT,0},
    {TCTEX_VK_FORMAT_D24_UNORM_S8_UINT,1},
    {TCTEX_VK_FORMAT_D24_UNORM_S8_UINT,0},
    {TCTEX_VK_FORMAT_D24_UNORM_S8_UINT,1},
    {TCTEX_VK_FORMAT_D24_UNORM_S8_UINT,1},
    {TCTEX_VK_FORMAT_R8G8_UINT,1},
    {TCTEX_VK_FORMAT_R8G8_UNORM,0},
    {TCTEX_VK_FORMAT_R8G8_UINT,0},
    {TCTEX_VK_FORMAT_R8G8_SNORM,0},
    {TCTEX_VK_FORMAT_R8G8_UINT,0},
    {TCTEX_VK_FORMAT_R16_UINT,1},
    {TCTEX_VK_FORMAT_R16_SFLOAT,0},
    {TCTEX_VK_FORMAT_D16_UNORM,0},
    {TCTEX_VK_FORMAT_R16_UNORM,0},
    {TCTEX_VK_FORMAT_R16_UINT,0},
    {TCTEX_VK_FORMAT_R16_SNORM,0},
    {TCTEX_VK_FORMAT_R16_SINT,0},
    {TCTEX_VK_FORMAT_R8_UINT,1},
    {TCTEX_VK_FORMAT_R8_UNORM,0},
    {TCTEX_VK_FORMAT_R8_UINT,0},
    {TCTEX_VK_FORMAT_R8_SNORM,0},
    {TCTEX_VK_FORMAT_R8_SINT,0},
    {TCTEX_VK_FORMAT_R8_UNORM,1},//TCTEX_DXGI_FORMAT_A8_UNORM
    {},//TCTEX_DXGI_FORMAT_R1_UNORM
    {},//TCTEX_DXGI_FORMAT_R9G9B9E5_SHAREDEXP
    {},//TCTEX_DXGI_FORMAT_R8G8_B8G8_UNORM
    {},//TCTEX_DXGI_FORMAT_G8R8_G8B8_UNORM
    {TCTEX_VK_FORMAT_BC1_RGBA_UNORM_BLOCK,1},
    {TCTEX_VK_FORMAT_BC1_RGBA_UNORM_BLOCK,0},
    {TCTEX_VK_FORMAT_BC1_RGBA_SRGB_BLOCK,0},
    {TCTEX_VK_FORMAT_BC2_UNORM_BLOCK,1},
    {TCTEX_VK_FORMAT_BC2_UNORM_BLOCK,0},
    {TCTEX_VK_FORMAT_BC2_SRGB_BLOCK,0},
    {TCTEX_VK_FORMAT_BC3_UNORM_BLOCK,1},
    {TCTEX_VK_FORMAT_BC3_UNORM_BLOCK,0},
    {TCTEX_VK_FORMAT_BC3_SRGB_BLOCK,0},
    {TCTEX_VK_FORMAT_BC4_UNORM_BLOCK,1},
    {TCTEX_VK_FORMAT_BC4_UNORM_BLOCK,0},
    {TCTEX_VK_FORMAT_BC4_SNORM_BLOCK,0},
    {TCTEX_VK_FORMAT_BC5_UNORM_BLOCK,1},
    {TCTEX_VK_FORMAT_BC5_UNORM_BLOCK,0},
    {TCTEX_VK_FORMAT_BC5_SNORM_BLOCK,0},
    {TCTEX_VK_FORMAT_B5G6R5_UNORM_PACK16,0},
    {TCTEX_VK_FORMAT_B5G5R5A1_UNORM_PACK16,0},
    {TCTEX_VK_FORMAT_B8G8R8A8_UNORM,0},
    {},//TCTEX_DXGI_FORMAT_B8G8R8X8_UNORM
    {},//TCTEX_DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM
    {TCTEX_VK_FORMAT_B8G8R8A8_UINT,1},
    {TCTEX_VK_FORMAT_B8G8R8A8_SRGB,0},
    {},//TCTEX_DXGI_FORMAT_B8G8R8X8_TYPELESS
    {},//TCTEX_DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
    {TCTEX_VK_FORMAT_BC6H_UFLOAT_BLOCK,1},
    {TCTEX_VK_FORMAT_BC6H_UFLOAT_BLOCK,0},
    {TCTEX_VK_FORMAT_BC6H_SFLOAT_BLOCK,0},
    {TCTEX_VK_FORMAT_BC7_UNORM_BLOCK,1},
    {TCTEX_VK_FORMAT_BC7_UNORM_BLOCK,0},
    {TCTEX_VK_FORMAT_BC7_SRGB_BLOCK,0},
    {},//TCTEX_DXGI_FORMAT_AYUV,
    {},//TCTEX_DXGI_FORMAT_Y410,
    {},//TCTEX_DXGI_FORMAT_Y416,
    {},//TCTEX_DXGI_FORMAT_NV12,
    {},//TCTEX_DXGI_FORMAT_P010,
    {},//TCTEX_DXGI_FORMAT_P016,
    {},//TCTEX_DXGI_FORMAT_420_OPAQUE,
    {},//TCTEX_DXGI_FORMAT_YUY2,
    {},//TCTEX_DXGI_FORMAT_Y210,
    {},//TCTEX_DXGI_FORMAT_Y216,
    {},//TCTEX_DXGI_FORMAT_NV11,
    {},//TCTEX_DXGI_FORMAT_AI44,
    {},//TCTEX_DXGI_FORMAT_IA44,
    {},//TCTEX_DXGI_FORMAT_P8,
    {},//TCTEX_DXGI_FORMAT_A8P8,
    {TCTEX_VK_FORMAT_B4G4R4A4_UNORM_PACK16},
    {},//TCTEX_DXGI_FORMAT_P208,
    {},//TCTEX_DXGI_FORMAT_V208,
    {},//TCTEX_DXGI_FORMAT_V408,
};
TCTex_VK_FormatInfo tctex_vk_get_formatinfo(const TCTex_Texture* tex)
{
    static const TCTex_VK_FormatInfo NoAlphaDXT1_TYPELESS = {TCTEX_VK_FORMAT_BC1_RGB_UNORM_BLOCK,1};
    static const TCTex_VK_FormatInfo NoAlphaDXT1_UNORM = {TCTEX_VK_FORMAT_BC1_RGB_UNORM_BLOCK,0};
    static const TCTex_VK_FormatInfo NoAlphaDXT1_SRGB = {TCTEX_VK_FORMAT_BC1_RGB_SRGB_BLOCK,0};

    TCTex_InternalFormat iformat = tex->iformat;
    if(iformat == TCTEX_IFORMAT_COMPRESSED_BC1_TYPELESS && tex->alphamode == TCTEX_ALPHA_MODE_OPAQUE) return NoAlphaDXT1_TYPELESS;
    if(iformat == TCTEX_IFORMAT_COMPRESSED_BC1_UNORM && tex->alphamode == TCTEX_ALPHA_MODE_OPAQUE) return NoAlphaDXT1_UNORM;
    if(iformat == TCTEX_IFORMAT_COMPRESSED_BC1_SRGB && tex->alphamode == TCTEX_ALPHA_MODE_OPAQUE) return NoAlphaDXT1_SRGB;
    return iformat < sizeof(tctex_i_vk_formats) / sizeof(*tctex_i_vk_formats) ? tctex_i_vk_formats[iformat] : tctex_i_vk_formats[0];
}
#endif /* TC_TEXTURE_LOAD_VULKAN_FORMATS */
#ifdef TC_TEXTURE_LOAD_OPENGL_FORMATS
typedef enum TCTex_GL_IFormat {
    TCTEX_GL_R8 = 0x8229, // RED
    TCTEX_GL_R8_SNORM = 0x8F94, // RED
    TCTEX_GL_R16 = 0x822A, // RED
    TCTEX_GL_R16_SNORM = 0x8F98, // RED
    TCTEX_GL_RG8 = 0x822B, // RG
    TCTEX_GL_RG8_SNORM = 0x8F95, // RG
    TCTEX_GL_RG16 = 0x822C, // RG
    TCTEX_GL_RG16_SNORM = 0x8F99, // RG
    TCTEX_GL_R3_G3_B2 = 0x2A10, // RGB
    TCTEX_GL_RGB4 = 0x804F, // RGB
    TCTEX_GL_RGB5 = 0x8050, // RGB
    TCTEX_GL_RGB565 = 0x8D62, // RGB
    TCTEX_GL_RGB8 = 0x8051, // RGB
    TCTEX_GL_RGB8_SNORM = 0x8F96, // RGB
    TCTEX_GL_RGB10 = 0x8052, // RGB
    TCTEX_GL_RGB12 = 0x8053, // RGB
    TCTEX_GL_RGB16 = 0x8054, // RGB
    TCTEX_GL_RGB16_SNORM = 0x8F9A, // RGB
    TCTEX_GL_RGBA2 = 0x8055, // RGBA
    TCTEX_GL_RGBA4 = 0x8056, // RGBA
    TCTEX_GL_RGB5_A1 = 0x8057, // RGBA
    TCTEX_GL_RGBA8 = 0x8058, // RGBA
    TCTEX_GL_RGBA8_SNORM = 0x8F97, // RGBA
    TCTEX_GL_RGB10_A2 = 0x8059, // RGBA
    TCTEX_GL_RGB10_A2UI = 0x906F, // RGBA
    TCTEX_GL_RGBA12 = 0x805A, // RGBA
    TCTEX_GL_RGBA16 = 0x805B, // RGBA
    TCTEX_GL_RGBA16_SNORM = 0x8F9B, // RGBA
    TCTEX_GL_SRGB8 = 0x8C41, // RGB
    TCTEX_GL_SRGB8_ALPHA8 = 0x8C43, // RGBA
    TCTEX_GL_R16F = 0x822D, // RED
    TCTEX_GL_RG16F = 0x822F, // RG
    TCTEX_GL_RGB16F = 0x881B, // RGB
    TCTEX_GL_RGBA16F = 0x881A, // RGBA
    TCTEX_GL_R32F = 0x822E, // RED
    TCTEX_GL_RG32F = 0x8230, // RG
    TCTEX_GL_RGB32F = 0x8815, // RGB
    TCTEX_GL_RGBA32F = 0x8814, // RGBA
    TCTEX_GL_R11F_G11F_B10F = 0x8C3A, // RGB
    TCTEX_GL_RGB9_E5 = 0x8C3D, // RGB
    TCTEX_GL_R8I = 0x8231, // RED
    TCTEX_GL_R8UI = 0x8232, // RED
    TCTEX_GL_R16I = 0x8233, // RED
    TCTEX_GL_R16UI = 0x8234, // RED
    TCTEX_GL_R32I = 0x8235, // RED
    TCTEX_GL_R32UI = 0x8236, // RED
    TCTEX_GL_RG8I = 0x8237, // RG
    TCTEX_GL_RG8UI = 0x8238, // RG
    TCTEX_GL_RG16I = 0x8239, // RG
    TCTEX_GL_RG16UI = 0x823A, // RG
    TCTEX_GL_RG32I = 0x823B, // RG
    TCTEX_GL_RG32UI = 0x823C, // RG
    TCTEX_GL_RGB8I = 0x8D8F, // RGB
    TCTEX_GL_RGB8UI = 0x8D7D, // RGB
    TCTEX_GL_RGB16I = 0x8D89, // RGB
    TCTEX_GL_RGB16UI = 0x8D77, // RGB
    TCTEX_GL_RGB32I = 0x8D83, // RGB
    TCTEX_GL_RGB32UI = 0x8D71, // RGB
    TCTEX_GL_RGBA8I = 0x8D8E, // RGBA
    TCTEX_GL_RGBA8UI = 0x8D7C, // RGBA
    TCTEX_GL_RGBA16I = 0x8D88, // RGBA
    TCTEX_GL_RGBA16UI = 0x8D76, // RGBA
    TCTEX_GL_RGBA32I = 0x8D82, // RGBA
    TCTEX_GL_RGBA32UI = 0x8D70, // RGBA

    TCTEX_GL_DEPTH_COMPONENT16 = 0x81A5, // DEPTH_COMPONENT
    TCTEX_GL_DEPTH_COMPONENT24 = 0x81A6, // DEPTH_COMPONENT
    TCTEX_GL_DEPTH_COMPONENT32 = 0x81A7, // DEPTH_COMPONENT
    TCTEX_GL_DEPTH_COMPONENT32F = 0x8CAC, // DEPTH_COMPONENT
    TCTEX_GL_DEPTH24_STENCIL8 = 0x88F0, // DEPTH_STENCIL
    TCTEX_GL_DEPTH32F_STENCIL8 = 0x8CAD, // DEPTH_STENCIL
    TCTEX_GL_STENCIL_INDEX1 = 0x8D46, // STENCIL_INDEX
    TCTEX_GL_STENCIL_INDEX4 = 0x8D47, // STENCIL_INDEX
    TCTEX_GL_STENCIL_INDEX8 = 0x8D48, // STENCIL_INDEX
    TCTEX_GL_STENCIL_INDEX16 = 0x8D49, // STENCIL_INDEX

    //COMPRESSED_RED                  RED
    //COMPRESSED_RG                   RG
    //COMPRESSED_RGB                  RGB
    //COMPRESSED_RGBA                 RGBA
    //COMPRESSED_SRGB                 RGB
    //COMPRESSED_SRGB_ALPHA           RGBA
    TCTEX_GL_COMPRESSED_RED_RGTC1 = 0x8DBB, // RED
    TCTEX_GL_COMPRESSED_SIGNED_RED_RGTC1 = 0x8DBC, // RED
    TCTEX_GL_COMPRESSED_RG_RGTC2 = 0x8DBD, // RG
    TCTEX_GL_COMPRESSED_SIGNED_RG_RGTC2 = 0x8DBE, // RG
    TCTEX_GL_COMPRESSED_RGBA_BPTC_UNORM = 0x8E8C, // RGBA
    TCTEX_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM = 0x8E8D, // RGBA
    TCTEX_GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT = 0x8E8E, // RGB
    TCTEX_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = 0x8E8F, // RGB
    TCTEX_GL_COMPRESSED_RGB8_ETC2 = 0x9274, // RGB
    TCTEX_GL_COMPRESSED_SRGB8_ETC2 = 0x9275, // RGB
    TCTEX_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9276, // RGB(A)
    TCTEX_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9277, // RGB(A)
    TCTEX_GL_COMPRESSED_RGBA8_ETC2_EAC = 0x9278, // RGBA
    TCTEX_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC = 0x9279, // RGBA
    TCTEX_GL_COMPRESSED_R11_EAC = 0x9270, // RED
    TCTEX_GL_COMPRESSED_SIGNED_R11_EAC = 0x9271, // RED
    TCTEX_GL_COMPRESSED_RG11_EAC = 0x9272, // RG
    TCTEX_GL_COMPRESSED_SIGNED_RG11_EAC = 0x9273, // RG

    // extensions
    TCTEX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT = 0x83F0,
    TCTEX_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT = 0x83F1,
    TCTEX_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT = 0x83F2,
    TCTEX_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT = 0x83F3,
    TCTEX_GL_COMPRESSED_SRGB_S3TC_DXT1_EXT = 0x8C4C,
    TCTEX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT = 0x8C4D,
    TCTEX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT = 0x8C4E,
    TCTEX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT = 0x8C4F,
} TCTex_GL_IFormat;
typedef enum TCTex_GL_Format {
    TCTEX_GL_DEPTH_COMPONENT = 0x1902, // D
    TCTEX_GL_RED = 0x1903, // RED
    TCTEX_GL_RG = 0x8227, // RG
    TCTEX_GL_RGB = 0x1907, // RGB
    TCTEX_GL_RGBA = 0x1908, // RGBA
    TCTEX_GL_STENCIL_INDEX = 0x1901, // S
    // base internal format only
    TCTEX_GL_DEPTH_STENCIL = 0x84F9, // DS
    // pixel format only
    TCTEX_GL_BGR = 0x80E0, // RGB
    TCTEX_GL_BGRA = 0x80E1, // RGBA
} TCTex_GL_Format;
typedef enum TCTex_GL_Type
{
    TCTEX_GL_UNSIGNED_BYTE = 0x1401,
    TCTEX_GL_BYTE = 0x1400,
    TCTEX_GL_UNSIGNED_SHORT = 0x1403,
    TCTEX_GL_SHORT = 0x1402,
    TCTEX_GL_UNSIGNED_INT = 0x1405,
    TCTEX_GL_INT = 0x1404,
    TCTEX_GL_HALF_FLOAT = 0x140B,
    TCTEX_GL_FLOAT = 0x1406,
    TCTEX_GL_UNSIGNED_BYTE_3_3_2 = 0x8032,
    TCTEX_GL_UNSIGNED_BYTE_2_3_3_REV = 0x8362,
    TCTEX_GL_UNSIGNED_SHORT_5_6_5 = 0x8363,
    TCTEX_GL_UNSIGNED_SHORT_5_6_5_REV = 0x8364,
    TCTEX_GL_UNSIGNED_SHORT_4_4_4_4 = 0x8033,
    TCTEX_GL_UNSIGNED_SHORT_4_4_4_4_REV = 0x8365,
    TCTEX_GL_UNSIGNED_SHORT_5_5_5_1 = 0x8034,
    TCTEX_GL_UNSIGNED_SHORT_1_5_5_5_REV = 0x8366,
    TCTEX_GL_UNSIGNED_INT_8_8_8_8 = 0x8035,
    TCTEX_GL_UNSIGNED_INT_8_8_8_8_REV = 0x8367,
    TCTEX_GL_UNSIGNED_INT_10_10_10_2 = 0x8036,
    TCTEX_GL_UNSIGNED_INT_2_10_10_10_REV = 0x8368,
    TCTEX_GL_UNSIGNED_INT_24_8 = 0x84FA,
    TCTEX_GL_UNSIGNED_INT_10F_11F_11F_REV = 0x8C3B,
    TCTEX_GL_UNSIGNED_INT_5_9_9_9_REV = 0x8C3E,
    TCTEX_GL_FLOAT_32_UNSIGNED_INT_24_8_REV = 0x8DAD,
} TCTex_GL_Type;
typedef enum TCTex_GL_Extension
{
    TCTEX_GL_EXT_texture_compression_s3tc = 1,
    TCTEX_GL_EXT_texture_sRGB = 2,
} TCTex_GL_Extension;

static const TCTex_GL_FormatInfo tctex_i_gl_formats[] = {
    {0,0,0,0,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA32UI,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT,0,1},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA32F,TCTEX_GL_RGBA,TCTEX_GL_FLOAT,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA32UI,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA32I,TCTEX_GL_RGBA,TCTEX_GL_INT,0,0},
    {TCTEX_GL_RGB,TCTEX_GL_RGB32UI,TCTEX_GL_RGB,TCTEX_GL_UNSIGNED_INT,0,1},
    {TCTEX_GL_RGB,TCTEX_GL_RGB32F,TCTEX_GL_RGB,TCTEX_GL_FLOAT,0,0},
    {TCTEX_GL_RGB,TCTEX_GL_RGB32UI,TCTEX_GL_RGB,TCTEX_GL_UNSIGNED_INT,0,0},
    {TCTEX_GL_RGB,TCTEX_GL_RGB32I,TCTEX_GL_RGB,TCTEX_GL_INT,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA16UI,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT,0,1},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA16F,TCTEX_GL_RGBA,TCTEX_GL_HALF_FLOAT,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA16,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_SHORT,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA16UI,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_SHORT,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA16_SNORM,TCTEX_GL_RGBA,TCTEX_GL_SHORT,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA16I,TCTEX_GL_RGBA,TCTEX_GL_SHORT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG32UI,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_INT,0,1},
    {TCTEX_GL_RG,TCTEX_GL_RG32F,TCTEX_GL_RG,TCTEX_GL_FLOAT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG32UI,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_INT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG32I,TCTEX_GL_RG,TCTEX_GL_INT,0,0},
    {TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_DEPTH32F_STENCIL8,TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_FLOAT_32_UNSIGNED_INT_24_8_REV,0,1},//TODO:verify
    {TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_DEPTH32F_STENCIL8,TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_FLOAT_32_UNSIGNED_INT_24_8_REV,0,0},
    {TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_DEPTH32F_STENCIL8,TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_FLOAT_32_UNSIGNED_INT_24_8_REV,0,1},
    {TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_DEPTH32F_STENCIL8,TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_FLOAT_32_UNSIGNED_INT_24_8_REV,0,1},
    {TCTEX_GL_RGBA,TCTEX_GL_RGB10_A2UI,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_10_10_10_2,0,1},
    {TCTEX_GL_RGBA,TCTEX_GL_RGB10_A2,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_10_10_10_2,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGB10_A2UI,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_10_10_10_2,0,0},
    {TCTEX_GL_RGB,TCTEX_GL_R11F_G11F_B10F,TCTEX_GL_RGB,TCTEX_GL_UNSIGNED_INT_10F_11F_11F_REV,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA8UI,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_8_8_8_8,0,1},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA8,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_8_8_8_8,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_SRGB8_ALPHA8,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_8_8_8_8,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA8UI,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_8_8_8_8,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA8_SNORM,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_8_8_8_8,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA8I,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_8_8_8_8,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16UI,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_SHORT,0,1},
    {TCTEX_GL_RG,TCTEX_GL_RG16F,TCTEX_GL_RG,TCTEX_GL_HALF_FLOAT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_SHORT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16UI,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_SHORT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16_SNORM,TCTEX_GL_RG,TCTEX_GL_SHORT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16I,TCTEX_GL_RG,TCTEX_GL_SHORT,0,0},
    {TCTEX_GL_RED,TCTEX_GL_R32UI,TCTEX_GL_RED,TCTEX_GL_UNSIGNED_INT,0,1},
    {TCTEX_GL_DEPTH_COMPONENT,TCTEX_GL_DEPTH_COMPONENT32F,TCTEX_GL_DEPTH_COMPONENT,TCTEX_GL_FLOAT,0,0},
    {TCTEX_GL_RED,TCTEX_GL_R32F,TCTEX_GL_RED,TCTEX_GL_FLOAT,0,0},
    {TCTEX_GL_RED,TCTEX_GL_R32UI,TCTEX_GL_RED,TCTEX_GL_UNSIGNED_INT,0,0},
    {TCTEX_GL_RED,TCTEX_GL_R32I,TCTEX_GL_RED,TCTEX_GL_INT,0,0},
    {TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_DEPTH24_STENCIL8,TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_UNSIGNED_INT_24_8,0,1},//TODO: verify
    {TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_DEPTH24_STENCIL8,TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_UNSIGNED_INT_24_8,0,0},
    {TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_DEPTH24_STENCIL8,TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_UNSIGNED_INT_24_8,0,1},
    {TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_DEPTH24_STENCIL8,TCTEX_GL_DEPTH_STENCIL,TCTEX_GL_UNSIGNED_INT_24_8,0,1},
    {TCTEX_GL_RG,TCTEX_GL_RG8UI,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_BYTE,0,1},
    {TCTEX_GL_RG,TCTEX_GL_RG8,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_BYTE,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG8UI,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_BYTE,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG8_SNORM,TCTEX_GL_RG,TCTEX_GL_BYTE,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG8I,TCTEX_GL_RG,TCTEX_GL_BYTE,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16UI,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_SHORT,0,1},
    {TCTEX_GL_RG,TCTEX_GL_RG16F,TCTEX_GL_RG,TCTEX_GL_HALF_FLOAT,0,0},
    {TCTEX_GL_DEPTH_COMPONENT,TCTEX_GL_DEPTH_COMPONENT16,TCTEX_GL_DEPTH_COMPONENT,TCTEX_GL_UNSIGNED_SHORT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_SHORT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16UI,TCTEX_GL_RG,TCTEX_GL_UNSIGNED_SHORT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16_SNORM,TCTEX_GL_RG,TCTEX_GL_SHORT,0,0},
    {TCTEX_GL_RG,TCTEX_GL_RG16I,TCTEX_GL_RG,TCTEX_GL_SHORT,0,0},
    {TCTEX_GL_RED,TCTEX_GL_R8UI,TCTEX_GL_RED,TCTEX_GL_UNSIGNED_BYTE,0,1},
    {TCTEX_GL_RED,TCTEX_GL_R8,TCTEX_GL_RED,TCTEX_GL_UNSIGNED_BYTE,0,0},
    {TCTEX_GL_RED,TCTEX_GL_R8UI,TCTEX_GL_RED,TCTEX_GL_UNSIGNED_BYTE,0,0},
    {TCTEX_GL_RED,TCTEX_GL_R8_SNORM,TCTEX_GL_RED,TCTEX_GL_BYTE,0,0},
    {TCTEX_GL_RED,TCTEX_GL_R8I,TCTEX_GL_RED,TCTEX_GL_BYTE,0,0},
    {TCTEX_GL_RED,TCTEX_GL_R8,TCTEX_GL_RED,TCTEX_GL_UNSIGNED_BYTE,0,1},//TCTEX_IFORMAT_A8_UNORM
    {},//TCTEX_IFORMAT_R1_UNORM
    {TCTEX_GL_RGB,TCTEX_GL_RGB9_E5,TCTEX_GL_RGB,TCTEX_GL_UNSIGNED_INT_5_9_9_9_REV,0,0},
    {},//TCTEX_IFORMAT_R8G8_B8G8_UNORM
    {},//TCTEX_IFORMAT_G8R8_G8B8_UNORM
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc,1},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc,0},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc|TCTEX_GL_EXT_texture_sRGB,0},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc,1},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc,0},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc|TCTEX_GL_EXT_texture_sRGB,0},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc,1},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc,0},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc|TCTEX_GL_EXT_texture_sRGB,0},
    {TCTEX_GL_RED,TCTEX_GL_COMPRESSED_RED_RGTC1,0,0,0,1},
    {TCTEX_GL_RED,TCTEX_GL_COMPRESSED_RED_RGTC1,0,0,0,0},
    {TCTEX_GL_RED,TCTEX_GL_COMPRESSED_SIGNED_RED_RGTC1,0,0,0,0},
    {TCTEX_GL_RG,TCTEX_GL_COMPRESSED_RG_RGTC2,0,0,0,1},
    {TCTEX_GL_RG,TCTEX_GL_COMPRESSED_RG_RGTC2,0,0,0,0},
    {TCTEX_GL_RG,TCTEX_GL_COMPRESSED_SIGNED_RG_RGTC2,0,0,0,0},
    {TCTEX_GL_RGB,TCTEX_GL_RGB565,TCTEX_GL_RGB,TCTEX_GL_UNSIGNED_SHORT_5_6_5_REV,0,0},//TODO: GL_BGR format?
    {TCTEX_GL_RGBA,TCTEX_GL_RGB5_A1,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_SHORT_1_5_5_5_REV,0,0},//TODO: GL_BGRA format?
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA8,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_8_8_8_8_REV,0,0},//TODO: GL_BGRA format?
    {TCTEX_GL_RGB,TCTEX_GL_RGB8,TCTEX_GL_RGB,TCTEX_GL_UNSIGNED_INT_8_8_8_8_REV,0,0},//TODO: GL_BGR format? GL_RGBA/GL_BGRA type?
    {},//TCTEX_IFORMAT_R10G10B10_XR_BIAS_A2_UNORM; TODO: what to do with the bias here?
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA8UI,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_8_8_8_8_REV,0,1},//TODO: GL_BGRA format?
    {TCTEX_GL_RGBA,TCTEX_GL_SRGB8_ALPHA8,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_INT_8_8_8_8_REV,0,0},//TODO: GL_BGRA format?
    {TCTEX_GL_RGB,TCTEX_GL_RGB8UI,TCTEX_GL_RGB,TCTEX_GL_UNSIGNED_INT_8_8_8_8_REV,0,1},//TODO: GL_BGR format? GL_RGBA/GL_BGRA type?
    {TCTEX_GL_RGB,TCTEX_GL_SRGB8,TCTEX_GL_RGB,TCTEX_GL_UNSIGNED_INT_8_8_8_8_REV,0,0},//TODO: GL_BGR format? GL_RGBA/GL_BGRA type?
    {TCTEX_GL_RGB,TCTEX_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,0,0,0,1},
    {TCTEX_GL_RGB,TCTEX_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,0,0,0,0},
    {TCTEX_GL_RGB,TCTEX_GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,0,0,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_RGBA_BPTC_UNORM,0,0,0,1},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_RGBA_BPTC_UNORM,0,0,0,0},
    {TCTEX_GL_RGBA,TCTEX_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,0,0,0,0},
    {},//TCTEX_IFORMAT_AYUV,
    {},//TCTEX_IFORMAT_Y410,
    {},//TCTEX_IFORMAT_Y416,
    {},//TCTEX_IFORMAT_NV12,
    {},//TCTEX_IFORMAT_P010,
    {},//TCTEX_IFORMAT_P016,
    {},//TCTEX_IFORMAT_420_OPAQUE,
    {},//TCTEX_IFORMAT_YUY2,
    {},//TCTEX_IFORMAT_Y210,
    {},//TCTEX_IFORMAT_Y216,
    {},//TCTEX_IFORMAT_NV11,
    {},//TCTEX_IFORMAT_AI44,
    {},//TCTEX_IFORMAT_IA44,
    {},//TCTEX_IFORMAT_P8,
    {},//TCTEX_IFORMAT_A8P8,
    {TCTEX_GL_RGBA,TCTEX_GL_RGBA4,TCTEX_GL_RGBA,TCTEX_GL_UNSIGNED_SHORT_4_4_4_4_REV,0,0},
    {},//TCTEX_IFORMAT_P208,
    {},//TCTEX_IFORMAT_V208,
    {},//TCTEX_IFORMAT_V408,
};
TCTex_GL_FormatInfo tctex_gl_get_formatinfo(const TCTex_Texture* tex)
{
    static const TCTex_GL_FormatInfo NoAlphaDXT1_TYPELESS = {TCTEX_GL_RGB,TCTEX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc,1};
    static const TCTex_GL_FormatInfo NoAlphaDXT1_UNORM = {TCTEX_GL_RGB,TCTEX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc,0};
    static const TCTex_GL_FormatInfo NoAlphaDXT1_SRGB = {TCTEX_GL_RGB,TCTEX_GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,0,0,TCTEX_GL_EXT_texture_compression_s3tc|TCTEX_GL_EXT_texture_sRGB,0};

    TCTex_InternalFormat iformat = tex->iformat;
    if(iformat == TCTEX_IFORMAT_COMPRESSED_BC1_TYPELESS && tex->alphamode == TCTEX_ALPHA_MODE_OPAQUE) return NoAlphaDXT1_TYPELESS;
    if(iformat == TCTEX_IFORMAT_COMPRESSED_BC1_UNORM && tex->alphamode == TCTEX_ALPHA_MODE_OPAQUE) return NoAlphaDXT1_UNORM;
    if(iformat == TCTEX_IFORMAT_COMPRESSED_BC1_SRGB && tex->alphamode == TCTEX_ALPHA_MODE_OPAQUE) return NoAlphaDXT1_SRGB;
    return iformat < sizeof(tctex_i_gl_formats) / sizeof(*tctex_i_gl_formats) ? tctex_i_gl_formats[iformat] : tctex_i_gl_formats[0];
}
#endif /* TC_TEXTURE_LOAD_OPENGL_FORMATS */
#ifdef TC_TEXTURE_LOAD_DIRECT3D_FORMATS
TCTex_D3D_FormatInfo tctex_d3d_get_formatinfo(const TCTex_Texture* tex)
{
    TCTex_InternalFormat iformat = tex->iformat;
    TCTex_D3D_FormatInfo finfo;
    finfo.dxgiFormat = TC__STATIC_CAST(uint32_t,iformat) <= TC__STATIC_CAST(uint32_t,TCTEX_DXGI_FORMAT_MAX_) ? iformat : TCTEX_DXGI_FORMAT_UNKNOWN;
    return finfo;
}
#endif /* TC_TEXTURE_LOAD_D3D_FORMATS */

#endif /* TC_TEXTURE_LOAD_IMPLEMENTATION */
