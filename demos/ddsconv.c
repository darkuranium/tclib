#define TC_TEXTURE_LOAD_IMPLEMENTATION
#include "../tc_texture_load.h"

#define TC_TEXTURE_CODEC_IMPLEMENTATION
#include "../tc_texture_codec.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "util/stb_image_write.h"

#include <stdlib.h>

static const char* InternalFormatStrings[] = {
    "UNDEFINED",
    "R32G32B32A32_TYPELESS",
    "R32G32B32A32_SFLOAT",
    "R32G32B32A32_UINT",
    "R32G32B32A32_SINT",
    "R32G32B32_TYPELESS",
    "R32G32B32_SFLOAT",
    "R32G32B32_UINT",
    "R32G32B32_SINT",
    "R16G16B16A16_TYPELESS",
    "R16G16B16A16_SFLOAT",
    "R16G16B16A16_UNORM",
    "R16G16B16A16_UINT",
    "R16G16B16A16_SNORM",
    "R16G16B16A16_SINT",
    "R32G32_TYPELESS",
    "R32G32_SFLOAT",
    "R32G32_UINT",
    "R32G32_SINT",
    "R32G8X24_TYPELESS",
    "D32_SFLOAT_S8X24_UINT",
    "R32_SFLOAT_X8X24_TYPELESS",
    "X32_TYPELESS_G8X24_UINT",
    "R10G10B10A2_TYPELESS",
    "R10G10B10A2_UNORM",
    "R10G10B10A2_UINT",
    "R11G11B10_SFLOAT",
    "R8G8B8A8_TYPELESS",
    "R8G8B8A8_UNORM",
    "R8G8B8A8_SRGB",
    "R8G8B8A8_UINT",
    "R8G8B8A8_SNORM",
    "R8G8B8A8_SINT",
    "R16G16_TYPELESS",
    "R16G16_SFLOAT",
    "R16G16_UNORM",
    "R16G16_UINT",
    "R16G16_SNORM",
    "R16G16_SINT",
    "R32_TYPELESS",
    "D32_SFLOAT",
    "R32_SFLOAT",
    "R32_UINT",
    "R32_SINT",
    "R24G8_TYPELESS",
    "D24_UNORM_S8_UINT",
    "R24_UNORM_X8_TYPELESS",
    "X24_TYPELESS_G8_UINT",
    "R8G8_TYPELESS",
    "R8G8_UNORM",
    "R8G8_UINT",
    "R8G8_SNORM",
    "R8G8_SINT",
    "R16_TYPELESS",
    "R16_SFLOAT",
    "D16_UNORM",
    "R16_UNORM",
    "R16_UINT",
    "R16_SNORM",
    "R16_SINT",
    "R8_TYPELESS",
    "R8_UNORM",
    "R8_UINT",
    "R8_SNORM",
    "R8_SINT",
    "A8_UNORM",
    "R1_UNORM",
    "R9G9B9E5_UFLOAT",
    "R8G8_B8G8_UNORM",
    "G8R8_G8B8_UNORM",
    "COMPRESSED_BC1_TYPELESS",
    "COMPRESSED_BC1_UNORM",
    "COMPRESSED_BC1_SRGB",
    "COMPRESSED_BC2_TYPELESS",
    "COMPRESSED_BC2_UNORM",
    "COMPRESSED_BC2_SRGB",
    "COMPRESSED_BC3_TYPELESS",
    "COMPRESSED_BC3_UNORM",
    "COMPRESSED_BC3_SRGB",
    "COMPRESSED_BC4_TYPELESS",
    "COMPRESSED_BC4_UNORM",
    "COMPRESSED_BC4_SNORM",
    "COMPRESSED_BC5_TYPELESS",
    "COMPRESSED_BC5_UNORM",
    "COMPRESSED_BC5_SNORM",
    "B5G6R5_UNORM",
    "B5G5R5A1_UNORM",
    "B8G8R8A8_UNORM",
    "B8G8R8X8_UNORM",
    "R10G10B10_XR_BIAS_A2_UNORM",
    "B8G8R8A8_TYPELESS",
    "B8G8R8A8_SRGB",
    "B8G8R8X8_TYPELESS",
    "B8G8R8X8_SRGB",
    "COMPRESSED_BC6H_TYPELESS",
    "COMPRESSED_BC6H_UFLOAT",
    "COMPRESSED_BC6H_SFLOAT",
    "COMPRESSED_BC7_TYPELESS",
    "COMPRESSED_BC7_UNORM",
    "COMPRESSED_BC7_SRGB",
    "AYUV",
    "Y410",
    "Y416",
    "NV12",
    "P010",
    "P016",
    "420_OPAQUE",
    "YUY2",
    "Y210",
    "Y216",
    "NV11",
    "AI44",
    "IA44",
    "P8",
    "A8P8",
    "B4G4R4A4_UNORM",
    "P208",
    "V208",
    "V408",
};

static uint8_t conv_clamp_u8f32(float f)
{
    int i = (int)f;
    return i < 0 ? 0 : i > 255 ? 255 : i;
}
void conv_float_from_half(float* dst, const uint16_t* src, size_t sizex, size_t sizey, size_t sizez, size_t comp)
{
    // we go in reverse so that dst can equal src!
    size_t i = sizex * sizey * sizez * comp;
    while(i--)
        dst[i] = tctex_util_float_from_half(src[i]);
}
void conv_srgb_from_float(uint8_t* dst, const float* src, size_t sizex, size_t sizey, size_t sizez, size_t comp)
{
    // we go in forward so that dst can equal src!
    size_t i;
    for(i = 0; i < sizex * sizey * sizez * comp; i++)
    {
        if(comp == 4 && (i & 3) == 3) // if it's alpha, convert it linearly
            dst[i] = conv_clamp_u8f32(src[i] * 255.0f);
        else
            dst[i] = tctex_util_srgb_from_linear(src[i]);
    }
}
void conv_float_from_srgb(float* dst, const uint8_t* src, size_t sizex, size_t sizey, size_t sizez, size_t comp)
{
    // we go in reverse so that dst can equal src!
    size_t i = sizex * sizey * sizez * comp;
    while(i--)
    {
        if(comp == 4 && (i & 3) == 3) // if it's alpha, convert it linearly
            dst[i] = src[i] / 255.0f;
        else
            dst[i] = tctex_util_linear_from_srgb(src[i]);
    }
}

void printUsage(const char* argv0, int ecode)
{
    printf("Usage: %s <input> <basename> <format:png|hdr>\n", argv0 ? argv0 : "ddsinfo");
    exit(ecode);
}
#define MAX(x,y) ((x)>(y)?(x):(y))
int main(int argc, char** argv)
{
    int ecode = 1;
    if(argc <= 3) printUsage(argv[0], 1);

    TCTex_Texture tex;
    if(!tctex_load_fname(&tex, argv[1]))
    {
        fprintf(stderr, "Unable to load \"%s\": %s\n", argv[1], tex.errmsg);
        return 1;
    }
    // this is enough space for the entire thing!
    void* obufA = malloc(tex.size.x * tex.size.y * tex.size.z * 4 * sizeof(uint32_t));
    void* obufB = malloc(tex.size.x * tex.size.y * tex.size.z * 4 * sizeof(float));
    uint8_t* obuf8 = obufA;
    uint16_t* obuf16 = obufA;
    float* obuff = obufB;

    char fnbuf[256];

    /*typedef struct TCTex_MipMapInfo
{
    uint64_t offset;
    uint32_t nbytes;
    struct { uint32_t x, y, z; } size;
    struct { uint32_t y, z; } pitch;
} TCTex_MipMapInfo;*/

    TCTex_MipMapInfo mminfo[32];
    uint32_t mmnum, mlevel;
    uint32_t x, y, z;
    uint8_t ochannels;

    printf("%s: %ux%ux%u (%u levels), format: %s\n", argv[1], tex.size.x, tex.size.y, tex.size.z, tex.nmiplevels, tex.iformat < sizeof(InternalFormatStrings)/sizeof(*InternalFormatStrings) ? InternalFormatStrings[tex.iformat] : "???");
    switch(tex.iformat)
    {
/*
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
    TCTEX_IFORMAT_R16_SINT = 59,*/
        case TCTEX_IFORMAT_R8_TYPELESS: // output: RGB8
        case TCTEX_IFORMAT_R8_UNORM:
        case TCTEX_IFORMAT_R8_UINT:
        case TCTEX_IFORMAT_R8_SNORM:
        case TCTEX_IFORMAT_R8_SINT:
        case TCTEX_IFORMAT_A8_UNORM:
            mmnum = tctex_get_mipmaps(&tex, mminfo, sizeof(mminfo) / sizeof(*mminfo), 0);
            ochannels = 3;

            for(mlevel = 0; mlevel < mmnum; mlevel++)
            {
                const struct { uint8_t r; }* data = (const void*)(tex.memory + mminfo[mlevel].offset);
                for(z = 0; z < mminfo[mlevel].size.z; z++)
                {
                    for(y = 0; y < mminfo[mlevel].size.y; y++)
                    {
                        for(x = 0; x < mminfo[mlevel].size.x; x++)
                        {
                            uint32_t doffset = ((z * mminfo[mlevel].size.y + y) * mminfo[mlevel].size.x + x) * ochannels;
                            uint32_t soffset = ((z * mminfo[mlevel].size.y + y) * mminfo[mlevel].size.x + x) * 1;
                            obuf8[doffset+0] = data[soffset].r;
                            obuf8[doffset+1] = 0x00;
                            obuf8[doffset+2] = 0x00;
                            if(tex.iformat == TCTEX_IFORMAT_R8_SNORM
                            || tex.iformat == TCTEX_IFORMAT_R8_SINT) // if is signed, adjust output (via bias)!
                                obuf8[doffset+0] += 128;
                        }
                    }
                }
                snprintf(fnbuf, sizeof(fnbuf), "%s-%umip%u.%s", argv[2], 0, mlevel, argv[3]);
                stbi_write_png(fnbuf, mminfo[mlevel].size.x, mminfo[mlevel].size.y * mminfo[mlevel].size.z, ochannels, obuf8, 0);
            }
            break;
/*
    TCTEX_IFORMAT_R1_UNORM = 66,
    TCTEX_IFORMAT_R9G9B9E5_UFLOAT = 67,
    TCTEX_IFORMAT_R8G8_B8G8_UNORM = 68,
    TCTEX_IFORMAT_G8R8_G8B8_UNORM = 69,
*/
        case TCTEX_IFORMAT_COMPRESSED_BC1_TYPELESS: // output: RGBA8
        case TCTEX_IFORMAT_COMPRESSED_BC1_UNORM:
        case TCTEX_IFORMAT_COMPRESSED_BC1_SRGB:
            mmnum = tctex_get_mipmaps(&tex, mminfo, sizeof(mminfo) / sizeof(*mminfo), 0);
            ochannels = 4;

            for(mlevel = 0; mlevel < mmnum; mlevel++)
            {
                const uint8_t* data = (const void*)(tex.memory + mminfo[mlevel].offset);
                for(z = 0; z < mminfo[mlevel].size.z; z++)
                {
                    uint32_t doffset = ((z * mminfo[mlevel].size.y + 0) * mminfo[mlevel].size.x + 0) * ochannels;
                    tctex_decompress_bc1(&obuf8[doffset], ochannels, mminfo[mlevel].size.x*ochannels, data, mminfo[mlevel].size.x, mminfo[mlevel].size.y, true, true);
                }
                snprintf(fnbuf, sizeof(fnbuf), "%s-%umip%u.%s", argv[2], 0, mlevel, argv[3]);
                stbi_write_png(fnbuf, mminfo[mlevel].size.x, mminfo[mlevel].size.y * mminfo[mlevel].size.z, ochannels, obuf8, 0);
            }
            break;
        case TCTEX_IFORMAT_COMPRESSED_BC2_TYPELESS: // output: RGBA8
        case TCTEX_IFORMAT_COMPRESSED_BC2_UNORM:
        case TCTEX_IFORMAT_COMPRESSED_BC2_SRGB:
            mmnum = tctex_get_mipmaps(&tex, mminfo, sizeof(mminfo) / sizeof(*mminfo), 0);
            ochannels = 4;

            for(mlevel = 0; mlevel < mmnum; mlevel++)
            {
                const uint8_t* data = (const void*)(tex.memory + mminfo[mlevel].offset);
                for(z = 0; z < mminfo[mlevel].size.z; z++)
                {
                    uint32_t doffset = ((z * mminfo[mlevel].size.y + 0) * mminfo[mlevel].size.x + 0) * ochannels;
                    tctex_decompress_bc2(&obuf8[doffset], ochannels, mminfo[mlevel].size.x*ochannels, data, mminfo[mlevel].size.x, mminfo[mlevel].size.y);
                }
                snprintf(fnbuf, sizeof(fnbuf), "%s-%umip%u.%s", argv[2], 0, mlevel, argv[3]);
                stbi_write_png(fnbuf, mminfo[mlevel].size.x, mminfo[mlevel].size.y * mminfo[mlevel].size.z, ochannels, obuf8, 0);
            }
            break;
        case TCTEX_IFORMAT_COMPRESSED_BC3_TYPELESS: // output: RGBA8
        case TCTEX_IFORMAT_COMPRESSED_BC3_UNORM:
        case TCTEX_IFORMAT_COMPRESSED_BC3_SRGB:
            mmnum = tctex_get_mipmaps(&tex, mminfo, sizeof(mminfo) / sizeof(*mminfo), 0);
            ochannels = 4;

            for(mlevel = 0; mlevel < mmnum; mlevel++)
            {
                const uint8_t* data = (const void*)(tex.memory + mminfo[mlevel].offset);
                for(z = 0; z < mminfo[mlevel].size.z; z++)
                {
                    uint32_t doffset = ((z * mminfo[mlevel].size.y + 0) * mminfo[mlevel].size.x + 0) * ochannels;
                    tctex_decompress_bc3(&obuf8[doffset], ochannels, mminfo[mlevel].size.x*ochannels, data, mminfo[mlevel].size.x, mminfo[mlevel].size.y);
                }
                snprintf(fnbuf, sizeof(fnbuf), "%s-%umip%u.%s", argv[2], 0, mlevel, argv[3]);
                stbi_write_png(fnbuf, mminfo[mlevel].size.x, mminfo[mlevel].size.y * mminfo[mlevel].size.z, ochannels, obuf8, 0);
            }
            break;
/*
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
*/
        case TCTEX_IFORMAT_COMPRESSED_BC6H_TYPELESS: // output: RGBf
        case TCTEX_IFORMAT_COMPRESSED_BC6H_UFLOAT:
        case TCTEX_IFORMAT_COMPRESSED_BC6H_SFLOAT:
            mmnum = tctex_get_mipmaps(&tex, mminfo, sizeof(mminfo) / sizeof(*mminfo), 0);
            ochannels = 3;

            for(mlevel = 0; mlevel < mmnum; mlevel++)
            {
                const uint8_t* data = (const void*)(tex.memory + mminfo[mlevel].offset);
                for(z = 0; z < mminfo[mlevel].size.z; z++)
                {
                    uint32_t doffset = ((z * mminfo[mlevel].size.y + 0) * mminfo[mlevel].size.x + 0) * ochannels;
                    tctex_decompress_bc6h(&obuf16[doffset], ochannels*sizeof(*obuf16), mminfo[mlevel].size.x*ochannels*sizeof(*obuf16), data, mminfo[mlevel].size.x, mminfo[mlevel].size.y, tex.iformat == TCTEX_IFORMAT_COMPRESSED_BC6H_SFLOAT);
                }
                conv_float_from_half(obuff, obuf16, mminfo[mlevel].size.x, mminfo[mlevel].size.y, mminfo[mlevel].size.z, ochannels);

                conv_srgb_from_float(obuf8, obuff, mminfo[mlevel].size.x, mminfo[mlevel].size.y, mminfo[mlevel].size.z, ochannels);
                snprintf(fnbuf, sizeof(fnbuf), "%s-%umip%u.%s", argv[2], 0, mlevel, argv[3]);
                stbi_write_png(fnbuf, mminfo[mlevel].size.x, mminfo[mlevel].size.y * mminfo[mlevel].size.z, ochannels, obuf8, 0);
            }
            break;
        case TCTEX_IFORMAT_COMPRESSED_BC7_TYPELESS: // output: RGBA8
        case TCTEX_IFORMAT_COMPRESSED_BC7_UNORM:
        case TCTEX_IFORMAT_COMPRESSED_BC7_SRGB:
            mmnum = tctex_get_mipmaps(&tex, mminfo, sizeof(mminfo) / sizeof(*mminfo), 0);
            ochannels = 4;

            for(mlevel = 0; mlevel < mmnum; mlevel++)
            {
                const uint8_t* data = (const void*)(tex.memory + mminfo[mlevel].offset);
                for(z = 0; z < mminfo[mlevel].size.z; z++)
                {
                    uint32_t doffset = ((z * mminfo[mlevel].size.y + 0) * mminfo[mlevel].size.x + 0) * ochannels;
                    tctex_decompress_bc7(&obuf8[doffset], ochannels, mminfo[mlevel].size.x*ochannels, data, mminfo[mlevel].size.x, mminfo[mlevel].size.y);
                }
                snprintf(fnbuf, sizeof(fnbuf), "%s-%umip%u.%s", argv[2], 0, mlevel, argv[3]);
                stbi_write_png(fnbuf, mminfo[mlevel].size.x, mminfo[mlevel].size.y * mminfo[mlevel].size.z, ochannels, obuf8, 0);
            }
            break;
/*
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
*/
        default: fprintf(stderr, "Error: Format not yet handled.\n"); goto exit;
    }

    ecode = 0;
exit:
    free(obufA);
    free(obufB);
    tctex_close(&tex);
    return ecode;
}
