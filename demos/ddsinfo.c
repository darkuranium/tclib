#define TC_TEXTURE_LOAD_IMPLEMENTATION
#define TC_TEXTURE_LOAD_VULKAN_FORMATS
#define TC_TEXTURE_LOAD_OPENGL_FORMATS
#define TC_TEXTURE_LOAD_DIRECT3D_FORMATS
#include "../tc_texture_load.h"

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

void printUsage(const char* argv0, int ecode)
{
    printf("Usage: %s <files...>\n", argv0 ? argv0 : "ddsinfo");
    exit(ecode);
}
int main(int argc, char** argv)
{
    static const char* AlphaModes[] = {
        "Unknown",
        "Straight",
        "Premultiplied",
        "Opaque",
        "Custom"
    };

    if(argc == 1) printUsage(argv[0], 1);
    int i;
    for(i = 1; i < argc; i++)
    {
        TCTex_Texture tex;
        if(!tctex_load_fname(&tex, argv[i]))
        {
            fprintf(stderr, "Unable to load \"%s\": %s\n", argv[i], tex.errmsg);
            continue;
        }

        int needsep;

        printf("========== %s ==========\n", argv[i]);
        printf("Dimension: %uD", tex.dimension);
        if(tex.isvolume) printf(" (Volumetric)");
        printf("\n");
        printf("Size: %ux%ux%u", tex.size.x, tex.size.y, tex.size.z);
        if(tex.arraylen != 1) printf(" [x%u array]", tex.arraylen);
        printf("\n");
        printf("MipMap Levels: %u\n", tex.nmiplevels);
        printf("CubeMap Faces: %u", tex.cubefaces.num);
        if(tex.cubefaces.num)
        {
            needsep = 0;
            printf("(");
            if(tex.cubefaces.mask & TCTEX_CUBE_FACE_POSX) { printf("%s+x", needsep ? "," : ""); needsep = true; }
            if(tex.cubefaces.mask & TCTEX_CUBE_FACE_NEGX) { printf("%s-x", needsep ? "," : ""); needsep = true; }
            if(tex.cubefaces.mask & TCTEX_CUBE_FACE_POSY) { printf("%s+y", needsep ? "," : ""); needsep = true; }
            if(tex.cubefaces.mask & TCTEX_CUBE_FACE_NEGY) { printf("%s-y", needsep ? "," : ""); needsep = true; }
            if(tex.cubefaces.mask & TCTEX_CUBE_FACE_POSZ) { printf("%s+z", needsep ? "," : ""); needsep = true; }
            if(tex.cubefaces.mask & TCTEX_CUBE_FACE_NEGZ) { printf("%s-z", needsep ? "," : ""); needsep = true; }
            printf(")");
        }
        printf("\n");
        printf("Alpha mode: %s (%u)\n", tex.alphamode < sizeof(AlphaModes)/sizeof(*AlphaModes) ? AlphaModes[tex.alphamode] : "???", tex.alphamode);
        printf("Data:\n");
        printf("\tStarting Offset: %u\n", tex.offset0);
        printf("\tPitch: Y=%u,Z=%u\n", tex.pitch.y, tex.pitch.z);
        printf("\tNumber of Bytes: %u\n", tex.nbytes);
        printf("\tInternal Format: %s (%u)\n", tex.iformat < sizeof(InternalFormatStrings)/sizeof(*InternalFormatStrings) ? InternalFormatStrings[tex.iformat] : "???", tex.iformat);

        tctex_close(&tex);
    }
    return 0;
}
