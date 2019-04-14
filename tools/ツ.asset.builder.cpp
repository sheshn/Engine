#include <stdio.h>
#include <stdlib.h>

#include "../ツ/ツ.h"
#include "../ツ/ツ.math.h"
#include "../ツ/ツ.renderer.h"
#include "../ツ/ツ.asset.h"
#include "ツ.tokenizer.h"

#define JSON_STRING(text) "\""#text"\""

struct String
{
    char* text;
    u64   length;
};

internal u8* read_file(wchar_t* filepath)
{
    FILE* file = _wfopen(filepath, L"rb");
    if (!file)
    {
        fprintf(stderr, "%ws not found :(\n", filepath);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    u8* result = (u8*)malloc(size + 1);
    fread(result, size, 1, file);
    fclose(file);
    result[size] = 0;

    return result;
}

internal void write_file(wchar_t* filepath, u8* contents, u32 size)
{
    FILE* file = _wfopen(filepath, L"wb");
    if (!file)
    {
        fprintf(stderr, "Unable to create %ws :(\n", filepath);
        exit(EXIT_FAILURE);
    }

    fwrite(contents, sizeof(u8), size, file);
    fclose(file);
}

internal u8* read_file(char* filepath)
{
    FILE* file = fopen(filepath, "rb");
    if (!file)
    {
        fprintf(stderr, "%s not found :(\n", filepath);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    u8* result = (u8*)malloc(size + 1);
    fread(result, size, 1, file);
    fclose(file);
    result[size] = 0;

    return result;
}

internal void write_file(char* filepath, u8* contents, u32 size)
{
    FILE* file = fopen(filepath, "wb");
    if (!file)
    {
        fprintf(stderr, "Unable to create %s :(\n", filepath);
        exit(EXIT_FAILURE);
    }

    fwrite(contents, sizeof(u8), size, file);
    fclose(file);
}

internal u64 file_size(char* filepath)
{
    FILE* file = fopen(filepath, "rb");
    if (!file)
    {
        fprintf(stderr, "ERROR: %s not found :(\n", filepath);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    fclose(file);

    return size;
}

internal b32 string_equals(char* s1, char* s2)
{
    while (*s1 && *s2)
    {
        if (*s1++ != *s2++)
        {
            return false;
        }
    }
    return *s1 == *s2;
}

internal b32 string_starts_with(char* s, char* prefix, u64 prefix_length)
{
    for (u64 i = 0; i < prefix_length; ++i)
    {
        if (s[i] != prefix[i])
        {
            return false;
        }
    }
    return true;
}

#define MAX_MIPMAP_LEVELS 16

#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((u32)(u8)(ch0) | ((u32)(u8)(ch1) << 8) | ((u32)(u8)(ch2) << 16) | ((u32)(u8)(ch3) << 24))

internal u32 DDS_MAGIC      = 0x20534444;
internal u32 DDS_FOURCC     = 0x00000004;
internal u32 DDS_RGB        = 0x00000040;
internal u32 DDS_RGBA       = 0x00000041;
internal u32 DDS_LUMINANCE  = 0x00020000;
internal u32 DDS_LUMINANCEA = 0x00020001;
internal u32 DDS_ALPHA      = 0x00000002;
internal u32 DDS_PAL8       = 0x00000020;
internal u32 DDS_BUMPDUDV   = 0x00080000;

internal u32 DDS_HEADER_FLAGS_TEXTURE    = 0x00001007;
internal u32 DDS_HEADER_FLAGS_MIPMAP     = 0x00020000;
internal u32 DDS_HEADER_FLAGS_VOLUME     = 0x00800000;
internal u32 DDS_HEADER_FLAGS_PITCH      = 0x00000008;
internal u32 DDS_HEADER_FLAGS_LINEARSIZE = 0x00080000;

internal u32 DDS_HEIGHT = 0x00000002;
internal u32 DDS_WIDTH  = 0x00000004;

internal u32 DDS_SURFACE_FLAGS_TEXTURE = 0x00001000;
internal u32 DDS_SURFACE_FLAGS_MIPMAP  = 0x00400008;
internal u32 DDS_SURFACE_FLAGS_CUBEMAP = 0x00000008;

internal u32 DDS_CUBEMAP_POSITIVEX = 0x00000600;
internal u32 DDS_CUBEMAP_NEGATIVEX = 0x00000a00;
internal u32 DDS_CUBEMAP_POSITIVEY = 0x00001200;
internal u32 DDS_CUBEMAP_NEGATIVEY = 0x00002200;
internal u32 DDS_CUBEMAP_POSITIVEZ = 0x00004200;
internal u32 DDS_CUBEMAP_NEGATIVEZ = 0x00008200;

internal u32 DDS_CUBEMAP          = 0x00000200;
internal u32 DDS_CUBEMAP_ALLFACES = DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX | DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY | DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ;
internal u32 DDS_FLAGS_VOLUME     = 0x00200000;

internal u32 DDS_RESOURCE_MISC_TEXTURECUBE   = 0x4L;
internal u32 DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L;

enum Image_Type
{
    IMAGE_TYPE_1D = 0,
    IMAGE_TYPE_2D = 1,
    IMAGE_TYPE_3D = 2
};

enum Image_View_Type
{
    IMAGE_VIEW_TYPE_1D         = 0,
    IMAGE_VIEW_TYPE_2D         = 1,
    IMAGE_VIEW_TYPE_3D         = 2,
    IMAGE_VIEW_TYPE_CUBE       = 3,
    IMAGE_VIEW_TYPE_1D_ARRAY   = 4,
    IMAGE_VIEW_TYPE_2D_ARRAY   = 5,
    IMAGE_VIEW_TYPE_CUBE_ARRAY = 6,
};

enum DXGI_Format
{
    DXGI_FORMAT_UNKNOWN                    = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS      = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT         = 2,
    DXGI_FORMAT_R32G32B32A32_UINT          = 3,
    DXGI_FORMAT_R32G32B32A32_SINT          = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS         = 5,
    DXGI_FORMAT_R32G32B32_FLOAT            = 6,
    DXGI_FORMAT_R32G32B32_UINT             = 7,
    DXGI_FORMAT_R32G32B32_SINT             = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS      = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT         = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM         = 11,
    DXGI_FORMAT_R16G16B16A16_UINT          = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM         = 13,
    DXGI_FORMAT_R16G16B16A16_SINT          = 14,
    DXGI_FORMAT_R32G32_TYPELESS            = 15,
    DXGI_FORMAT_R32G32_FLOAT               = 16,
    DXGI_FORMAT_R32G32_UINT                = 17,
    DXGI_FORMAT_R32G32_SINT                = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS          = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT       = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS   = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT    = 22,
    DXGI_FORMAT_R10G10B10A2_TYPELESS       = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM          = 24,
    DXGI_FORMAT_R10G10B10A2_UINT           = 25,
    DXGI_FORMAT_R11G11B10_FLOAT            = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS          = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM             = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB        = 29,
    DXGI_FORMAT_R8G8B8A8_UINT              = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM             = 31,
    DXGI_FORMAT_R8G8B8A8_SINT              = 32,
    DXGI_FORMAT_R16G16_TYPELESS            = 33,
    DXGI_FORMAT_R16G16_FLOAT               = 34,
    DXGI_FORMAT_R16G16_UNORM               = 35,
    DXGI_FORMAT_R16G16_UINT                = 36,
    DXGI_FORMAT_R16G16_SNORM               = 37,
    DXGI_FORMAT_R16G16_SINT                = 38,
    DXGI_FORMAT_R32_TYPELESS               = 39,
    DXGI_FORMAT_D32_FLOAT                  = 40,
    DXGI_FORMAT_R32_FLOAT                  = 41,
    DXGI_FORMAT_R32_UINT                   = 42,
    DXGI_FORMAT_R32_SINT                   = 43,
    DXGI_FORMAT_R24G8_TYPELESS             = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT          = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS      = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT       = 47,
    DXGI_FORMAT_R8G8_TYPELESS              = 48,
    DXGI_FORMAT_R8G8_UNORM                 = 49,
    DXGI_FORMAT_R8G8_UINT                  = 50,
    DXGI_FORMAT_R8G8_SNORM                 = 51,
    DXGI_FORMAT_R8G8_SINT                  = 52,
    DXGI_FORMAT_R16_TYPELESS               = 53,
    DXGI_FORMAT_R16_FLOAT                  = 54,
    DXGI_FORMAT_D16_UNORM                  = 55,
    DXGI_FORMAT_R16_UNORM                  = 56,
    DXGI_FORMAT_R16_UINT                   = 57,
    DXGI_FORMAT_R16_SNORM                  = 58,
    DXGI_FORMAT_R16_SINT                   = 59,
    DXGI_FORMAT_R8_TYPELESS                = 60,
    DXGI_FORMAT_R8_UNORM                   = 61,
    DXGI_FORMAT_R8_UINT                    = 62,
    DXGI_FORMAT_R8_SNORM                   = 63,
    DXGI_FORMAT_R8_SINT                    = 64,
    DXGI_FORMAT_A8_UNORM                   = 65,
    DXGI_FORMAT_R1_UNORM                   = 66,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP         = 67,
    DXGI_FORMAT_R8G8_B8G8_UNORM            = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM            = 69,
    DXGI_FORMAT_BC1_TYPELESS               = 70,
    DXGI_FORMAT_BC1_UNORM                  = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB             = 72,
    DXGI_FORMAT_BC2_TYPELESS               = 73,
    DXGI_FORMAT_BC2_UNORM                  = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB             = 75,
    DXGI_FORMAT_BC3_TYPELESS               = 76,
    DXGI_FORMAT_BC3_UNORM                  = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB             = 78,
    DXGI_FORMAT_BC4_TYPELESS               = 79,
    DXGI_FORMAT_BC4_UNORM                  = 80,
    DXGI_FORMAT_BC4_SNORM                  = 81,
    DXGI_FORMAT_BC5_TYPELESS               = 82,
    DXGI_FORMAT_BC5_UNORM                  = 83,
    DXGI_FORMAT_BC5_SNORM                  = 84,
    DXGI_FORMAT_B5G6R5_UNORM               = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM             = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM             = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM             = 88,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    DXGI_FORMAT_B8G8R8A8_TYPELESS          = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB        = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS          = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB        = 93,
    DXGI_FORMAT_BC6H_TYPELESS              = 94,
    DXGI_FORMAT_BC6H_UF16                  = 95,
    DXGI_FORMAT_BC6H_SF16                  = 96,
    DXGI_FORMAT_BC7_TYPELESS               = 97,
    DXGI_FORMAT_BC7_UNORM                  = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB             = 99,
    DXGI_FORMAT_AYUV                       = 100,
    DXGI_FORMAT_Y410                       = 101,
    DXGI_FORMAT_Y416                       = 102,
    DXGI_FORMAT_NV12                       = 103,
    DXGI_FORMAT_P010                       = 104,
    DXGI_FORMAT_P016                       = 105,
    DXGI_FORMAT_420_OPAQUE                 = 106,
    DXGI_FORMAT_YUY2                       = 107,
    DXGI_FORMAT_Y210                       = 108,
    DXGI_FORMAT_Y216                       = 109,
    DXGI_FORMAT_NV11                       = 110,
    DXGI_FORMAT_AI44                       = 111,
    DXGI_FORMAT_IA44                       = 112,
    DXGI_FORMAT_P8                         = 113,
    DXGI_FORMAT_A8P8                       = 114,
    DXGI_FORMAT_B4G4R4A4_UNORM             = 115,
    DXGI_FORMAT_FORCE_UINT                 = 0xffffffffUL
};

enum DDS_Resource_Dimension
{
    D3D11_RESOURCE_DIMENSION_UNKNOWN   = 0,
    D3D11_RESOURCE_DIMENSION_BUFFER    = 1,
    D3D11_RESOURCE_DIMENSION_TEXTURE1D = 2,
    D3D11_RESOURCE_DIMENSION_TEXTURE2D = 3,
    D3D11_RESOURCE_DIMENSION_TEXTURE3D = 4
};

#pragma pack(push, 1)
struct DDS_Pixelformat
{
    u32 size;
    u32 flags;
    u32 four_cc;
    u32 rgb_bit_count;
    u32 r_bit_mask;
    u32 g_bit_mask;
    u32 b_bit_mask;
    u32 a_bit_mask;
};

struct DDS_Header
{
    u32             size;
    u32             flags;
    u32             height;
    u32             width;
    u32             pitch_or_linear_size;
    u32             depth;
    u32             mip_map_count;
    u32             reserved1[11];
    DDS_Pixelformat ddspf;
    u32             caps;
    u32             caps2;
    u32             caps3;
    u32             caps4;
    u32             reserved2;
};

struct DDS_Header_DXT10
{
    DXGI_Format            format;
    DDS_Resource_Dimension resource_dimension;
    u32                    misc_flag;
    u32                    array_size;
    u32                    misc_flags2;
};
#pragma pack(pop)

struct DDS_Image_Surface
{
    u8* data;
    u32 size;
    u32 width;
    u32 height;
    u32 depth;
};

struct DDS_Image
{
    u8*               data;
    u32               size;
    DDS_Image_Surface levels[MAX_MIPMAP_LEVELS];
    u32               mipmap_count;
    DXGI_Format       format;
    Image_Type        type;
    Image_View_Type   view_type;
};

inline u32 dds_get_bits_per_pixel(DXGI_Format format)
{
    switch (format)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

    default:
        return 0;
    }
}

inline DXGI_Format dds_get_dxgi_format(DDS_Pixelformat& pixel_format)
{
    #define IS_BIT_MASK(r,g,b,a) (pixel_format.r_bit_mask == r && pixel_format.g_bit_mask == g && pixel_format.b_bit_mask == b && pixel_format.a_bit_mask == a)

    if (pixel_format.flags & DDS_RGB)
    {
        switch (pixel_format.rgb_bit_count)
        {
        case 32:
            if (IS_BIT_MASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000))
            {
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            }
            if (IS_BIT_MASK(0x00ff0000,0x0000ff00,0x000000ff,0xff000000))
            {
                return DXGI_FORMAT_B8G8R8A8_UNORM;
            }
            if (IS_BIT_MASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000))
            {
                return DXGI_FORMAT_B8G8R8X8_UNORM;
            }
            if (IS_BIT_MASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000))
            {
                return DXGI_FORMAT_R10G10B10A2_UNORM;
            }
            if (IS_BIT_MASK(0x0000ffff,0xffff0000,0x00000000,0x00000000))
            {
                return DXGI_FORMAT_R16G16_UNORM;
            }
            if (IS_BIT_MASK(0xffffffff,0x00000000,0x00000000,0x00000000))
            {
                return DXGI_FORMAT_R32_FLOAT;
            }
            break;

        case 24:
            break;

        case 16:
            if (IS_BIT_MASK(0x7c00,0x03e0,0x001f,0x8000))
            {
                return DXGI_FORMAT_B5G5R5A1_UNORM;
            }
            if (IS_BIT_MASK(0xf800,0x07e0,0x001f,0x0000))
            {
                return DXGI_FORMAT_B5G6R5_UNORM;
            }
            if (IS_BIT_MASK(0x0f00,0x00f0,0x000f,0xf000))
            {
                return DXGI_FORMAT_B4G4R4A4_UNORM;
            }
            break;
        }
    }
    else if (pixel_format.flags & DDS_LUMINANCE)
    {
        if (pixel_format.rgb_bit_count == 8 && IS_BIT_MASK(0x000000ff,0x00000000,0x00000000,0x00000000))
        {
            return DXGI_FORMAT_R8_UNORM;
        }
        if (pixel_format.rgb_bit_count == 16)
        {
            if (IS_BIT_MASK(0x0000ffff,0x00000000,0x00000000,0x00000000))
            {
                return DXGI_FORMAT_R16_UNORM;
            }
            if (IS_BIT_MASK(0x000000ff,0x00000000,0x00000000,0x0000ff00))
            {
                return DXGI_FORMAT_R8G8_UNORM;
            }
        }
    }
    else if ((pixel_format.flags & DDS_ALPHA) && pixel_format.rgb_bit_count == 8)
    {
        return DXGI_FORMAT_A8_UNORM;
    }
    else if (pixel_format.flags & DDS_BUMPDUDV)
    {
        if (pixel_format.rgb_bit_count == 16 && IS_BIT_MASK(0x00ff, 0xff00, 0x0000, 0x0000))
        {
            return DXGI_FORMAT_R8G8_SNORM;
        }

        if (pixel_format.rgb_bit_count == 32)
        {
            if (IS_BIT_MASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
            {
                return DXGI_FORMAT_R8G8B8A8_SNORM;
            }
            if (IS_BIT_MASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
            {
                return DXGI_FORMAT_R16G16_SNORM;
            }
        }
    }
    else if (pixel_format.flags & DDS_FOURCC)
    {
        if (MAKEFOURCC('D', 'X', 'T', '1') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC1_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '3') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC2_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '5') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC3_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '2') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC2_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '4') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC3_UNORM;
        }
        if (MAKEFOURCC('A', 'T', 'I', '1') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC4_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '4', 'U') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC4_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '4', 'S') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC4_SNORM;
        }
        if (MAKEFOURCC('A', 'T', 'I', '2') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC5_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '5', 'U') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC5_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '5', 'S') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_BC5_SNORM;
        }
        if (MAKEFOURCC('R', 'G', 'B', 'G') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_R8G8_B8G8_UNORM;
        }
        if (MAKEFOURCC('G', 'R', 'G', 'B') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_G8R8_G8B8_UNORM;
        }
        if (MAKEFOURCC('Y','U','Y','2') == pixel_format.four_cc)
        {
            return DXGI_FORMAT_YUY2;
        }

        switch(pixel_format.four_cc)
        {
        case 36:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case 110:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case 111:
            return DXGI_FORMAT_R16_FLOAT;
        case 112:
            return DXGI_FORMAT_R16G16_FLOAT;
        case 113:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case 114:
            return DXGI_FORMAT_R32_FLOAT;
        case 115:
            return DXGI_FORMAT_R32G32_FLOAT;
        case 116:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
    }

    #undef IS_BIT_MASK
    return DXGI_FORMAT_UNKNOWN;
}

inline u32 dds_get_surface_size(u32 width, u32 height, DXGI_Format format)
{
    u32  size = 0;
    u32  bpe = 0;
    bool bc = false;
    bool packed = false;
    bool planar = false;

    switch (format)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        bc = true;
        bpe = 8;
        break;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        bc = true;
        bpe = 16;
        break;

    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_YUY2:
        packed = true;
        bpe = 4;
        break;

    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        packed = true;
        bpe = 8;
        break;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
        planar = true;
        bpe = 2;
        break;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        planar = true;
        bpe = 4;
        break;
    }

    if (bc)
    {
        u32 block_width = width > 0 ? max(1, (width + 3) / 4) : 0;
        u32 block_height = height > 0 ? max(1, (height + 3) / 4) : 0;
        size = block_width * bpe * block_height;
    }
    else if (packed)
    {
        size = ((width + 1) >> 1) * bpe * height;
    }
    else if (format == DXGI_FORMAT_NV11)
    {
        size = ((width + 3) >> 2) * 4 * height * 2;
    }
    else if (planar)
    {
        u32 row = ((width + 1) >> 1) * bpe;
        size = (row * height) + ((row * height + 1) >> 1);
    }
    else
    {
        u32 bpp = dds_get_bits_per_pixel(format);
        size = (width * bpp + 7) / 8 * height;
    }

    return size;
}

internal b32 dds_load_image(DDS_Image* image)
{
    u8* data = image->data;
    DDS_Header* header = (DDS_Header*)(data + sizeof(u32));
    if (*((u32*)data) != DDS_MAGIC || header->size != sizeof(DDS_Header) || header->ddspf.size != sizeof(DDS_Pixelformat))
    {
        return false;
    }

    u32 levels = max(1, header->mip_map_count);
    u32 layers = 1;

    image->format = DXGI_FORMAT_UNKNOWN;
    u32 width = header->width;
    u32 height = header->height;
    u32 depth = header->depth;

    data += sizeof(u32) + sizeof(DDS_Header);
    if ((header->ddspf.flags & DDS_FOURCC) && (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.four_cc))
    {
        DDS_Header_DXT10* header_dxt10 = (DDS_Header_DXT10*)data;
        data += sizeof(DDS_Header_DXT10);

        layers = header_dxt10->array_size;
        if (header_dxt10->array_size == 0)
        {
            return false;
        }

        switch (header_dxt10->format)
        {
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
        case DXGI_FORMAT_A8P8:
            return false;

        default:
            if (dds_get_bits_per_pixel(header_dxt10->format) == 0)
            {
                return false;
            }
        }

        image->format = header_dxt10->format;
        switch (header_dxt10->resource_dimension)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            if ((header->flags & DDS_HEIGHT) && header->height != 1)
            {
                return false;
            }
            image->type = IMAGE_TYPE_1D;
            image->view_type = IMAGE_VIEW_TYPE_1D_ARRAY;
            height = 1;
            depth = 1;
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            if (header_dxt10->misc_flag & DDS_RESOURCE_MISC_TEXTURECUBE)
            {
                layers *= 6;
                image->view_type = IMAGE_VIEW_TYPE_CUBE_ARRAY;
            }
            else
            {
                image->view_type = IMAGE_VIEW_TYPE_2D_ARRAY;
            }
            image->type = IMAGE_TYPE_2D;
            depth = 1;
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            if (!(header->flags & DDS_FLAGS_VOLUME) || layers > 1)
            {
                return false;
            }
            image->type = IMAGE_TYPE_3D;
            image->view_type = IMAGE_VIEW_TYPE_3D;
            break;

        default:
            return false;
        }
    }
    else
    {
        image->format = dds_get_dxgi_format(header->ddspf);
        if (image->format == DXGI_FORMAT_UNKNOWN)
        {
            return false;
        }

        if (header->flags & DDS_FLAGS_VOLUME)
        {
            image->type = IMAGE_TYPE_3D;
            image->view_type = IMAGE_VIEW_TYPE_3D;
        }
        else
        {
            if (header->caps2 & DDS_CUBEMAP)
            {
                if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
                {
                    return false;
                }

                layers = 6;
                image->view_type = IMAGE_VIEW_TYPE_CUBE;
            }
            else
            {
                image->view_type = IMAGE_VIEW_TYPE_2D;
            }

            image->type = IMAGE_TYPE_2D;
            depth = 1;
        }
    }

    // NOTE: Only doing regular 2D textures for now.
    assert(layers == 1 && levels < MAX_MIPMAP_LEVELS);

    image->size = 0;
    image->mipmap_count = levels;
    for (u32 i = 0; i < layers; ++i)
    {
        u32 w = width;
        u32 h = height;
        u32 d = depth;

        for (u32 j = 0; j < levels; ++j)
        {
            u32 size = dds_get_surface_size(w, h, image->format);
            image->levels[j] = {data, size * d, w, h, d};

            data += size;
            image->size += size;
            w = max(1, w >> 1);
            h = max(1, h >> 1);
            d = max(1, d >> 1);
        }
    }

    return true;
}

#undef MAKEFOURCC

#define GLTF_INIT_OBJECT(name) internal void name(void* data)
typedef void (*GLTF_Init_Object_Function)(void* data);

struct GLTF_Buffer
{
    String name;
    String uri;
    u64    size;
};

enum GLTF_Buffer_View_Target
{
    GLTF_BUFFER_VIEW_TARGET_VERTEX_ARRAY = 34962,
    GLTF_BUFFER_VIEW_TARGET_INDEX_ARRAY  = 34963
};

struct GLTF_Buffer_View
{
    String name;

    u32 buffer_index;
    u64 size;
    u64 offset;
    u32 stride;

    GLTF_Buffer_View_Target target;
};

enum GLTF_Component_Type
{
    GLTF_COMPONENT_TYPE_BYTE   = 5120,
    GLTF_COMPONENT_TYPE_UBYTE  = 5121,
    GLTF_COMPONENT_TYPE_SHORT  = 5122,
    GLTF_COMPONENT_TYPE_USHORT = 5123,
    GLTF_COMPONENT_TYPE_UINT   = 5125,
    GLTF_COMPONENT_TYPE_FLOAT  = 5126,
};

u32 gltf_sizeof_component_type(GLTF_Component_Type component_type)
{
    switch (component_type)
    {
    case GLTF_COMPONENT_TYPE_BYTE:
    case GLTF_COMPONENT_TYPE_UBYTE:
        return 1;
    case GLTF_COMPONENT_TYPE_SHORT:
    case GLTF_COMPONENT_TYPE_USHORT:
        return 2;
    case GLTF_COMPONENT_TYPE_UINT:
    case GLTF_COMPONENT_TYPE_FLOAT:
        return 4;
    }
    return 0;
}

enum GLTF_Data_Type
{
    GLTF_DATA_TYPE_SCALAR,
    GLTF_DATA_TYPE_VEC2,
    GLTF_DATA_TYPE_VEC3,
    GLTF_DATA_TYPE_VEC4,
    GLTF_DATA_TYPE_MAT2,
    GLTF_DATA_TYPE_MAT3,
    GLTF_DATA_TYPE_MAT4
};

u32 gltf_element_count_of_data_type(GLTF_Data_Type data_type)
{
    switch (data_type)
    {
    case GLTF_DATA_TYPE_SCALAR:
        return 1;
    case GLTF_DATA_TYPE_VEC2:
        return 2;
    case GLTF_DATA_TYPE_VEC3:
        return 3;
    case GLTF_DATA_TYPE_VEC4:
        return 4;
    case GLTF_DATA_TYPE_MAT2:
        return 4;
    case GLTF_DATA_TYPE_MAT3:
        return 9;
    case GLTF_DATA_TYPE_MAT4:
        return 16;
    }
    return 0;
}

u32 gltf_sizeof_data_type_with_component_type(GLTF_Data_Type data_type, GLTF_Component_Type component_type)
{
    u32 component_size = gltf_sizeof_component_type(component_type);
    u32 element_count = gltf_element_count_of_data_type(data_type);
    return element_count * component_size;
}

// NOTE: Ignoring sparse accessor for now
struct GLTF_Accessor
{
    u32 buffer_view_index; // +1 to index (0 means shouldn't use this property)
    u64 offset;
    u64 element_count;

    GLTF_Component_Type component_type;
    GLTF_Data_Type      data_type;

    m4x4 min;
    m4x4 max;
    b32  normalized;
};

enum GLTF_Primitive_Mode
{
    GLTF_PRIMITIVE_TYPE_POINT_LIST     = 0,
    GLTF_PRIMITIVE_TYPE_LINE_LIST      = 1,
    GLTF_PRIMITIVE_TYPE_LINE_LOOP      = 2,
    GLTF_PRIMITIVE_TYPE_LINE_STRIP     = 3,
    GLTF_PRIMITIVE_TYPE_TRIANGLE_LIST  = 4,
    GLTF_PRIMITIVE_TYPE_TRIANGLE_STRIP = 5,
    GLTF_PRIMITIVE_TYPE_TRIANGLE_FAN   = 6
};

enum GLTF_Attribute_Type
{
    GLTF_ATTRIBUTE_TYPE_POSITION,
    GLTF_ATTRIBUTE_TYPE_NORMAL,
    GLTF_ATTRIBUTE_TYPE_TANGENT,
    GLTF_ATTRIBUTE_TYPE_TEXCOORD,
    GLTF_ATTRIBUTE_TYPE_COLOR,
    GLTF_ATTRIBUTE_TYPE_JOINTS,
    GLTF_ATTRIBUTE_TYPE_WEIGHTS
};

struct GLTF_Attribute
{
    GLTF_Attribute_Type type;
    u32                 set_index;
    u32                 accessor_index;
};

// NOTE: Ignoring morph targets for now
struct GLTF_Primitive
{
    GLTF_Primitive_Mode mode;

    GLTF_Attribute attributes[8];
    u32            attribute_count;

    u32 indices_accessor_index; // +1 to index (0 means shouldn't use this property)
    u32 material_index;         // +1 to index (0 means shouldn't use this property)
};

GLTF_INIT_OBJECT(init_gltf_primitive)
{
    GLTF_Primitive* o = (GLTF_Primitive*)data;
    *o = {};
    o->mode = GLTF_PRIMITIVE_TYPE_TRIANGLE_LIST;
}

// NOTE: Ignoring weights for now
struct GLTF_Mesh
{
    String name;

    GLTF_Primitive* primitives;
    u32             primitive_count;
};

struct GLTF_Image
{
    String uri;
    String mime_type;
    u32    buffer_view_index;
};

struct GLTF_MSFT_Texture_DDS_Extension
{
    u32 dds_image_source_index; // +1 to index (0 means shouldn't use this property)
};

// NOTE: Ignoring sampler for now
struct GLTF_Texture
{
    u32 sampler_index;
    u32 image_source_index;

    GLTF_MSFT_Texture_DDS_Extension dds_extension;
};

struct GLTF_Material_Texture
{
    u32 texture_index; // +1 to index (0 means shouldn't use this property)
    u32 texcoord_set_index;

    union
    {
        f32 scale;
        f32 strength;
    };
};

struct GLTF_PBR_Material_Metallic_Roughness
{
    v4  base_color_factor;
    f32 metallic_factor;
    f32 roughness_factor;

    GLTF_Material_Texture base_color_texture;
    GLTF_Material_Texture metallic_roughness_texture;
};

enum GLTF_Alpha_Mode
{
    GLTF_ALPHA_MODE_OPAQUE,
    GLTF_ALPHA_MODE_MASK,
    GLTF_ALPHA_MODE_BLEND
};

struct GLTF_MSFT_Packing_Occlusion_Roughness_Metallic_Extension
{
    GLTF_Material_Texture occlusion_roughness_metallic_texture;
    GLTF_Material_Texture normal_texture;
};

struct GLTF_Material
{
    String name;

    GLTF_PBR_Material_Metallic_Roughness pbr_metallic_roughness;
    GLTF_Material_Texture                normal_texture;
    GLTF_Material_Texture                occlusion_texture;
    GLTF_Material_Texture                emissive_texture;
    v3                                   emissive_factor;

    GLTF_Alpha_Mode alpha_mode;
    f32             alpha_cutoff;
    b32             double_sided;

    GLTF_MSFT_Packing_Occlusion_Roughness_Metallic_Extension packing_occlusion_roughness_metallic_extension;
};

GLTF_INIT_OBJECT(init_gltf_material)
{
    GLTF_Material* o = (GLTF_Material*)data;
    *o = {};
    o->pbr_metallic_roughness.base_color_factor = {1, 1, 1, 1};
    o->pbr_metallic_roughness.metallic_factor = 1.0f;
    o->pbr_metallic_roughness.roughness_factor = 1.0f;
    o->pbr_metallic_roughness.base_color_texture.scale = 1.0f;
    o->pbr_metallic_roughness.metallic_roughness_texture.scale = 1.0f;
    o->normal_texture.scale = 1.0f;
    o->occlusion_texture.strength = 1.0f;
    o->emissive_texture.scale = 1.0f;
    o->alpha_mode = GLTF_ALPHA_MODE_OPAQUE;
    o->alpha_cutoff = 0.5f;
    o->packing_occlusion_roughness_metallic_extension.occlusion_roughness_metallic_texture.scale = 1.0f;
    o->packing_occlusion_roughness_metallic_extension.normal_texture.scale = 1.0f;
}

struct GLTF_File
{
    GLTF_Buffer* buffers;
    u32          buffer_count;

    GLTF_Buffer_View* buffer_views;
    u32               buffer_view_count;

    GLTF_Accessor* accessors;
    u32            accessor_count;

    GLTF_Mesh*     meshes;
    u32            mesh_count;

    GLTF_Image* images;
    u32         image_count;

    GLTF_Texture* textures;
    u32           texture_count;

    GLTF_Material* materials;
    u32            material_count;

    char* json;
    u8*   bin;
};

internal u32 GLTF_BINARY_MAGIC = 0x46546C67;

enum GLTF_Binary_Chunk_Type
{
    GLTF_BINARY_CHUNK_TYPE_JSON = 0x4E4F534A,
    GLTF_BINARY_CHUNK_TYPE_BIN  = 0x004E4942
};

#pragma pack(push, 1)
struct GLTF_Binary_File_Header
{
    u32 magic;
    u32 version;
    u32 length;
};

struct GLTF_Binary_Chunk_Header
{
    u32 length;
    u32 type;
};
#pragma pack(pop)

internal u32 json_array_count(Tokenizer* tokenizer)
{
    char* at = tokenizer->at;
    u32 count = 0;

    if (eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACKET))
    {
        u32 bracket_count = 1;
        Token last = get_token(tokenizer);
        if (last.type == TOKEN_TYPE_OPEN_BRACE)
        {
            u32 brace_count = 1;
            while (true)
            {
                Token current = get_token(tokenizer);
                if (current.type == TOKEN_TYPE_OPEN_BRACE)
                {
                    brace_count++;
                }
                else if (current.type == TOKEN_TYPE_CLOSE_BRACE)
                {
                    brace_count--;
                }
                else if (current.type == TOKEN_TYPE_COMMA && last.type == TOKEN_TYPE_CLOSE_BRACE && brace_count == 0)
                {
                    count++;
                }
                else if (current.type == TOKEN_TYPE_OPEN_BRACKET)
                {
                    bracket_count++;
                }
                else if (current.type == TOKEN_TYPE_CLOSE_BRACKET)
                {
                    bracket_count--;
                    if (bracket_count == 0)
                    {
                        break;
                    }
                }
                last = current;
            }
        }
    }
    tokenizer->at = at;
    return count + 1;
}

#define GLTF_CHECK_PARSE(found, key, value) if (!found) { fprintf(stderr, "ERROR(%s): unsupported key: %.*s, with value: %.*s\n", __FUNCTION__, (u32)key.length, key.text, (u32)value.length, value.text); }
#define GLTF_PARSE_OBJECT(name) internal void name(Tokenizer* tokenizer, Token key, Token value, void* data)
typedef void (*GLTF_Parse_Object_Function)(Tokenizer* tokenizer, Token key, Token value, void* data);

internal void parse_gltf_object(Tokenizer* tokenizer, GLTF_Parse_Object_Function parse_function, void* data, u32* count = NULL, u32 size = 0, GLTF_Init_Object_Function init_function = NULL)
{
    if (init_function)
    {
        init_function(data);
    }
    if (eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACE))
    {
        while (true)
        {
            Token key = get_token(tokenizer);
            if (key.type == TOKEN_TYPE_COMMA)
            {
                continue;
            }
            else if (key.type == TOKEN_TYPE_CLOSE_BRACE)
            {
                break;
            }
            else if (eat_token(tokenizer, TOKEN_TYPE_COLON))
            {
                parse_function(tokenizer, key, get_token(tokenizer), count ? (void*)((u8*)data + (*count)++ * size) : data);
            }
        }
    }
}

internal void parse_gltf_array(Tokenizer* tokenizer, void** array, u32* count, u32 element_size, GLTF_Parse_Object_Function parse_function, GLTF_Init_Object_Function init_function = NULL)
{
    *count = json_array_count(tokenizer);
    *array = calloc(*count, element_size);

    if (*count > 0 && eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACKET))
    {
        for (u32 i = 0; i < *count; ++i)
        {
            parse_gltf_object(tokenizer, parse_function, (void*)((u8*)*array + i * element_size), NULL, 0, init_function);
            if (!eat_token(tokenizer, TOKEN_TYPE_COMMA))
            {
                break;
            }
        }
    }
}

internal void json_value_parse_gltf_object(Tokenizer* tokenizer, char* key, void* data, GLTF_Parse_Object_Function parse_function, Token key_token, Token value_token, b32* found, u32* count = NULL, u32 size = 0, GLTF_Init_Object_Function init_function = NULL)
{
    if (token_equals(key_token, key))
    {
        tokenizer->at = value_token.text;
        parse_gltf_object(tokenizer, parse_function, data, count, size, init_function);
        *found = true;
    }
}

internal void json_value_parse_gltf_array(Tokenizer* tokenizer, char* key, void** data, u32* count, u32 element_size, GLTF_Parse_Object_Function parse_function, Token key_token, Token value_token, b32* found, GLTF_Init_Object_Function init_function = NULL)
{
    if (token_equals(key_token, key))
    {
        tokenizer->at = value_token.text;
        parse_gltf_array(tokenizer, data, count, element_size, parse_function, init_function);
        *found = true;
    }
}

internal void json_value_as_u32(Tokenizer* tokenizer, char* key, u32* value, Token key_token, Token value_token, b32* found, u32 array_size = 0, u32 increment = 0)
{
    if (token_equals(key_token, key) && value_token.type == TOKEN_TYPE_NUMBER)
    {
        *value = (u32)value_token.u64 + increment;
        *found = true;
    }
}

#define JSON_VALUE_AS(name, type) internal void name(Tokenizer* tokenizer, char* key, type* value, Token key_token, Token value_token, b32* found, u32 array_size = 0)
JSON_VALUE_AS(json_value_as_string, String) { if (token_equals(key_token, key) && value_token.type == TOKEN_TYPE_STRING) { *value = {value_token.text, value_token.length}; *found = true; } }
JSON_VALUE_AS(json_value_as_u64, u64)       { if (token_equals(key_token, key) && value_token.type == TOKEN_TYPE_NUMBER) { *value = value_token.u64; *found = true; } }
JSON_VALUE_AS(json_value_as_f32, f32)       { if (token_equals(key_token, key) && value_token.type == TOKEN_TYPE_NUMBER) { *value = (f32)value_token.f64; *found = true; } }

JSON_VALUE_AS(json_value_as_f32_array, float)
{
    if (token_equals(key_token, key) && value_token.type == TOKEN_TYPE_OPEN_BRACKET)
    {
        *found = true;
        u64 count = 0;
        while (true)
        {
            Token token = get_token(tokenizer);
            if (token.type == TOKEN_TYPE_CLOSE_BRACKET)
            {
                break;
            }
            else if (token.type == TOKEN_TYPE_NUMBER)
            {
                if (count >= array_size)
                {
                    *found = false;
                    break;
                }
                value[count++] = token.is_float ? (f32)token.f64 : (f32)token.s64;
            }
        }
    }
}

JSON_VALUE_AS(json_value_as_b32, b32)
{
    if (token_equals(key_token, key) && value_token.type == TOKEN_TYPE_IDENTIFIER)
    {
        *found = true;
        if (token_equals(value_token, "true"))
        {
            *value = true;
        }
        else if (token_equals(value_token, "false"))
        {
            *value = false;
        }
        else
        {
            *found = false;
        }
    }
}

JSON_VALUE_AS(json_value_as_gltf_data_type, GLTF_Data_Type)
{
    if (token_equals(key_token, key) && value_token.type == TOKEN_TYPE_STRING)
    {
        *found = true;
        if (token_equals(value_token, JSON_STRING(SCALAR)))    { *value = GLTF_DATA_TYPE_SCALAR; }
        else if (token_equals(value_token, JSON_STRING(VEC2))) { *value = GLTF_DATA_TYPE_VEC2; }
        else if (token_equals(value_token, JSON_STRING(VEC3))) { *value = GLTF_DATA_TYPE_VEC3; }
        else if (token_equals(value_token, JSON_STRING(VEC4))) { *value = GLTF_DATA_TYPE_VEC4; }
        else if (token_equals(value_token, JSON_STRING(MAT2))) { *value = GLTF_DATA_TYPE_MAT2; }
        else if (token_equals(value_token, JSON_STRING(MAT3))) { *value = GLTF_DATA_TYPE_MAT3; }
        else if (token_equals(value_token, JSON_STRING(MAT4))) { *value = GLTF_DATA_TYPE_MAT4; }
        else                                                   { *found = false; }
    }
}

JSON_VALUE_AS(json_value_as_gltf_alpha_mode, GLTF_Alpha_Mode)
{
    if (token_equals(key_token, key) && value_token.type == TOKEN_TYPE_STRING)
    {
        *found = true;
        if (token_equals(value_token, JSON_STRING(OPAQUE)))     { *value = GLTF_ALPHA_MODE_OPAQUE; }
        else if (token_equals(value_token, JSON_STRING(MASK)))  { *value = GLTF_ALPHA_MODE_MASK; }
        else if (token_equals(value_token, JSON_STRING(BLEND))) { *value = GLTF_ALPHA_MODE_BLEND; }
        else                                                    { *found = false; }
    }
}

GLTF_PARSE_OBJECT(parse_gltf_buffer)
{
    GLTF_Buffer* buffer = (GLTF_Buffer*)data;

    b32 found = false;
    json_value_as_string(tokenizer, JSON_STRING(name), &buffer->name, key, value, &found);
    json_value_as_string(tokenizer, JSON_STRING(uri), &buffer->uri, key, value, &found);
    json_value_as_u64(tokenizer, JSON_STRING(byteLength), &buffer->size, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_buffer_view)
{
    GLTF_Buffer_View* buffer_view = (GLTF_Buffer_View*)data;

    b32 found = false;
    json_value_as_string(tokenizer, JSON_STRING(name), &buffer_view->name, key, value, &found);
    json_value_as_u32(tokenizer, JSON_STRING(buffer), &buffer_view->buffer_index, key, value, &found);
    json_value_as_u64(tokenizer, JSON_STRING(byteLength), &buffer_view->size, key, value, &found);
    json_value_as_u64(tokenizer, JSON_STRING(byteOffset), &buffer_view->offset, key, value, &found);
    json_value_as_u32(tokenizer, JSON_STRING(byteStride), &buffer_view->stride, key, value, &found);
    json_value_as_u32(tokenizer, JSON_STRING(target), (u32*)&buffer_view->target, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_accessor)
{
    GLTF_Accessor* accessor = (GLTF_Accessor*)data;

    b32 found = false;
    json_value_as_u32(tokenizer, JSON_STRING(bufferView), &accessor->buffer_view_index, key, value, &found, 0, 1);
    json_value_as_u64(tokenizer, JSON_STRING(byteOffset), &accessor->offset, key, value, &found);
    json_value_as_u64(tokenizer, JSON_STRING(count), &accessor->element_count, key, value, &found);
    json_value_as_u64(tokenizer, JSON_STRING(componentType), (u64*)&accessor->component_type, key, value, &found);
    json_value_as_gltf_data_type(tokenizer, JSON_STRING(type), &accessor->data_type, key, value, &found);
    json_value_as_f32_array(tokenizer, JSON_STRING(min), accessor->min.data, key, value, &found, array_count(accessor->min.data));
    json_value_as_f32_array(tokenizer, JSON_STRING(max), accessor->max.data, key, value, &found, array_count(accessor->max.data));
    json_value_as_b32(tokenizer, JSON_STRING(normalized), &accessor->normalized, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_attribute)
{
    GLTF_Attribute* attribute = (GLTF_Attribute*)data;

    b32 found = false;
    if (key.type == TOKEN_TYPE_STRING && value.type == TOKEN_TYPE_NUMBER)
    {
        found = true;
        if (token_starts_with(key, JSON_STRING(POSITION), sizeof(JSON_STRING(POSITION)) - 1))      { attribute->type = GLTF_ATTRIBUTE_TYPE_POSITION; }
        else if (token_starts_with(key, JSON_STRING(NORMAL), sizeof(JSON_STRING(NORMAL)) - 1))     { attribute->type = GLTF_ATTRIBUTE_TYPE_NORMAL; }
        else if (token_starts_with(key, JSON_STRING(TANGENT), sizeof(JSON_STRING(TANGENT)) - 1))   { attribute->type = GLTF_ATTRIBUTE_TYPE_TANGENT; }
        else if (token_starts_with(key, JSON_STRING(TEXCOORD), sizeof(JSON_STRING(TEXCOORD)) - 3)) { attribute->type = GLTF_ATTRIBUTE_TYPE_TEXCOORD; }
        else if (token_starts_with(key, JSON_STRING(COLOR), sizeof(JSON_STRING(COLOR)) - 3))       { attribute->type = GLTF_ATTRIBUTE_TYPE_COLOR; }
        else if (token_starts_with(key, JSON_STRING(JOINTS), sizeof(JSON_STRING(JOINTS)) - 3))     { attribute->type = GLTF_ATTRIBUTE_TYPE_JOINTS; }
        else if (token_starts_with(key, JSON_STRING(WEIGHTS), sizeof(JSON_STRING(WEIGHTS)) - 3))   { attribute->type = GLTF_ATTRIBUTE_TYPE_WEIGHTS; }
        else                                                                                       { found = false; }

        if (found)
        {
            attribute->set_index = 0;
            for (u64 i = 0; i < key.length; ++i)
            {
                if (key.text[i] == '_')
                {
                    attribute->set_index = strtoul(key.text + 1, NULL, 0);
                    break;
                }
            }
            attribute->accessor_index = (u32)value.u64;
        }
    }
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_primitive)
{
    GLTF_Primitive* primitive = (GLTF_Primitive*)data;

    b32 found = false;
    json_value_parse_gltf_object(tokenizer, JSON_STRING(attributes), primitive->attributes, parse_gltf_attribute, key, value, &found, &primitive->attribute_count, sizeof(GLTF_Attribute));
    json_value_as_u32(tokenizer, JSON_STRING(mode), (u32*)&primitive->mode, key, value, &found);
    json_value_as_u32(tokenizer, JSON_STRING(indices), &primitive->indices_accessor_index, key, value, &found, 0, 1);
    json_value_as_u32(tokenizer, JSON_STRING(material), &primitive->material_index, key, value, &found, 0, 1);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_mesh)
{
    GLTF_Mesh* mesh = (GLTF_Mesh*)data;

    b32 found = false;
    json_value_parse_gltf_array(tokenizer, JSON_STRING(primitives), (void**)&mesh->primitives, &mesh->primitive_count, sizeof(GLTF_Primitive), parse_gltf_primitive, key, value, &found, init_gltf_primitive);
    json_value_as_string(tokenizer, JSON_STRING(name), &mesh->name, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_image)
{
    GLTF_Image* image = (GLTF_Image*)data;

    b32 found = false;
    json_value_as_string(tokenizer, JSON_STRING(uri), &image->uri, key, value, &found);
    json_value_as_string(tokenizer, JSON_STRING(mimeType), &image->mime_type, key, value, &found);
    json_value_as_u32(tokenizer, JSON_STRING(bufferView), &image->buffer_view_index, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_material_texture)
{
    GLTF_Material_Texture* texture = (GLTF_Material_Texture*)data;

    b32 found = false;
    json_value_as_u32(tokenizer, JSON_STRING(index), &texture->texture_index, key, value, &found, 0, 1);
    json_value_as_u32(tokenizer, JSON_STRING(texCoord), &texture->texcoord_set_index, key, value, &found);
    json_value_as_f32(tokenizer, JSON_STRING(scale), &texture->scale, key, value, &found);
    json_value_as_f32(tokenizer, JSON_STRING(strength), &texture->strength, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_pbr_metallic_roughness)
{
    GLTF_PBR_Material_Metallic_Roughness* pbr = (GLTF_PBR_Material_Metallic_Roughness*)data;

    b32 found = false;
    json_value_parse_gltf_object(tokenizer, JSON_STRING(baseColorTexture), &pbr->base_color_texture, parse_gltf_material_texture, key, value, &found);
    json_value_parse_gltf_object(tokenizer, JSON_STRING(metallicRoughnessTexture), &pbr->metallic_roughness_texture, parse_gltf_material_texture, key, value, &found);
    json_value_as_f32_array(tokenizer, JSON_STRING(baseColorFactor), pbr->base_color_factor.data, key, value, &found, array_count(pbr->base_color_factor.data));
    json_value_as_f32(tokenizer, JSON_STRING(metallicFactor), &pbr->metallic_factor, key, value, &found);
    json_value_as_f32(tokenizer, JSON_STRING(roughnessFactor), &pbr->roughness_factor, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_msft_texture_dds)
{
    GLTF_MSFT_Texture_DDS_Extension* dds = (GLTF_MSFT_Texture_DDS_Extension*)data;

    b32 found = false;
    json_value_as_u32(tokenizer, JSON_STRING(source), &dds->dds_image_source_index, key, value, &found, 0, 1);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_msft_packing_occlusion_roughness_metallic)
{
    GLTF_MSFT_Packing_Occlusion_Roughness_Metallic_Extension* packing = (GLTF_MSFT_Packing_Occlusion_Roughness_Metallic_Extension*)data;

    b32 found = false;
    json_value_parse_gltf_object(tokenizer, JSON_STRING(occlusionRoughnessMetallicTexture), &packing->occlusion_roughness_metallic_texture, parse_gltf_material_texture, key, value, &found);
    json_value_parse_gltf_object(tokenizer, JSON_STRING(normalTexture), &packing->normal_texture, parse_gltf_material_texture, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_extension)
{
    b32 found = false;
    json_value_parse_gltf_object(tokenizer, JSON_STRING(MSFT_texture_dds), data, parse_gltf_msft_texture_dds, key, value, &found);
    json_value_parse_gltf_object(tokenizer, JSON_STRING(MSFT_packing_occlusionRoughnessMetallic), data, parse_gltf_msft_packing_occlusion_roughness_metallic, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_texture)
{
    GLTF_Texture* texture = (GLTF_Texture*)data;

    b32 found = false;
    json_value_parse_gltf_object(tokenizer, JSON_STRING(extensions), &texture->dds_extension, parse_gltf_extension, key, value, &found);
    json_value_as_u32(tokenizer, JSON_STRING(sampler), &texture->sampler_index, key, value, &found);
    json_value_as_u32(tokenizer, JSON_STRING(source), &texture->image_source_index, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

GLTF_PARSE_OBJECT(parse_gltf_material)
{
    GLTF_Material* material = (GLTF_Material*)data;

    b32 found = false;
    json_value_parse_gltf_object(tokenizer, JSON_STRING(extensions), &material->packing_occlusion_roughness_metallic_extension, parse_gltf_extension, key, value, &found);
    json_value_parse_gltf_object(tokenizer, JSON_STRING(pbrMetallicRoughness), &material->pbr_metallic_roughness, parse_gltf_pbr_metallic_roughness, key, value, &found);
    json_value_parse_gltf_object(tokenizer, JSON_STRING(normalTexture), &material->normal_texture, parse_gltf_material_texture, key, value, &found);
    json_value_parse_gltf_object(tokenizer, JSON_STRING(occlusionTexture), &material->occlusion_texture, parse_gltf_material_texture, key, value, &found);
    json_value_parse_gltf_object(tokenizer, JSON_STRING(emissiveTexture), &material->emissive_texture, parse_gltf_material_texture, key, value, &found);
    json_value_as_string(tokenizer, JSON_STRING(name), &material->name, key, value, &found);
    json_value_as_f32_array(tokenizer, JSON_STRING(emissiveFactor), material->emissive_factor.data, key, value, &found, array_count(material->emissive_factor.data));
    json_value_as_gltf_alpha_mode(tokenizer, JSON_STRING(alphaMode), &material->alpha_mode, key, value, &found);
    json_value_as_f32(tokenizer, JSON_STRING(alphaCutoff), &material->alpha_cutoff, key, value, &found);
    json_value_as_b32(tokenizer, JSON_STRING(doubleSided), &material->double_sided, key, value, &found);
    GLTF_CHECK_PARSE(found, key, value);
}

internal b32 parse_gltf(GLTF_File* gltf)
{
    Tokenizer tokenizer = {gltf->json};

    b32 parsing = true;
    while (parsing)
    {
        Token token = get_token(&tokenizer);
        switch (token.type)
        {
        case TOKEN_TYPE_END_OF_STREAM:
            parsing = false;
            break;
        case TOKEN_TYPE_STRING:
        {
            if (eat_token(&tokenizer, TOKEN_TYPE_COLON))
            {
                if (token_equals(token, JSON_STRING(buffers)))
                {
                    parse_gltf_array(&tokenizer, (void**)&gltf->buffers, &gltf->buffer_count, sizeof(GLTF_Buffer), parse_gltf_buffer);
                }
                else if (token_equals(token, JSON_STRING(bufferViews)))
                {
                    parse_gltf_array(&tokenizer, (void**)&gltf->buffer_views, &gltf->buffer_view_count, sizeof(GLTF_Buffer_View), parse_gltf_buffer_view);
                }
                else if (token_equals(token, JSON_STRING(accessors)))
                {
                    parse_gltf_array(&tokenizer, (void**)&gltf->accessors, &gltf->accessor_count, sizeof(GLTF_Accessor), parse_gltf_accessor);
                }
                else if (token_equals(token, JSON_STRING(meshes)))
                {
                    parse_gltf_array(&tokenizer, (void**)&gltf->meshes, &gltf->mesh_count, sizeof(GLTF_Mesh), parse_gltf_mesh);
                }
                else if (token_equals(token, JSON_STRING(images)))
                {
                    parse_gltf_array(&tokenizer, (void**)&gltf->images, &gltf->image_count, sizeof(GLTF_Image), parse_gltf_image);
                }
                else if (token_equals(token, JSON_STRING(textures)))
                {
                    parse_gltf_array(&tokenizer, (void**)&gltf->textures, &gltf->texture_count, sizeof(GLTF_Texture), parse_gltf_texture);
                }
                else if (token_equals(token, JSON_STRING(materials)))
                {
                    parse_gltf_array(&tokenizer, (void**)&gltf->materials, &gltf->material_count, sizeof(GLTF_Material), parse_gltf_material, init_gltf_material);
                }
            }
        } break;
        default:
            break;
        }
    }
    return true;
}

struct TSU_File
{
    TSU_Header header;

    Asset_Info* assets;
    u64         asset_count;
    u64         current_asset_index;

    u64 current_texture_base_index;
    u64 current_material_base_index;

    u8* asset_data;
    u64 asset_data_size;
    u64 asset_data_offset;
    u64 current_asset_data_offset;

    u32 texture_count;
    u32 material_count;
    u32 mesh_count;
    u32 sub_mesh_count;
};

/*
       TSU File Format
    |-------------------|
    |       magic       |
    |-------------------|
    |      version      |
    |-------------------|
    |    asset_count    |
    |-------------------|
    |    reserved[5]    |
    |-------------------|<---|
    |   Texture_Info1   |    |
    |   Texture_Info2   |    |
    |   Texture_Info3   |    |
    |        ...        |    |
    |-------------------|    |
    |   Material_Info1  |    |
    |   Material_Info2  |    |
    |   Material_Info3  |    |
    |        ...        |    |
    |-------------------|    |---- Asset Infos
    |     Mesh_Info1    |    |
    |-------------------|    |
    |  Sub_Mesh_Info11  |    |
    |  Sub_Mesh_Info12  |    |
    |-------------------|    |
    |     Mesh_Info2    |    |
    |-------------------|    |
    |  Sub_Mesh_Info21  |    |
    |-------------------|    |
    |     Mesh_Info3    |    |
    |-------------------|    |
    |  Sub_Mesh_Info31  |    |
    |        ...        |    |
    |-------------------|<---|
    |        Data       |
    |-------------------|

     NOTE: Order of Asset Infos will always be textures, materials, mesh then sub_mesh
*/

internal u64 gltf_file_get_total_texture_data_size(GLTF_File* gltf)
{
    u64 texture_data_size = 0;
    for (u32 i = 0; i < gltf->texture_count; ++i)
    {
        GLTF_Texture* t = gltf->textures + i;
        if (t->dds_extension.dds_image_source_index == 0)
        {
            fprintf(stderr, "ERROR: texture %u is NOT supported (only support DDS textures)!\n", i);
        }
        else
        {
            GLTF_Image* image = gltf->images + t->dds_extension.dds_image_source_index - 1;
            if (image->uri.text)
            {
                // TODO: This filepath needs to be relative to the GLTF file
                // texture_data_size = file_size(image->uri.text);
            }
            else if (image->mime_type.text)
            {
                assert(string_starts_with(image->mime_type.text, JSON_STRING(image/vnd-ms.dds), sizeof(JSON_STRING(image/vnd-ms.dds)) - 1));

                DDS_Image dds = {gltf->bin + gltf->buffer_views[image->buffer_view_index].offset};
                if (dds_load_image(&dds) && (dds.format == DXGI_FORMAT_BC5_UNORM || dds.format == DXGI_FORMAT_BC7_UNORM || dds.format == DXGI_FORMAT_BC7_UNORM_SRGB))
                {
                    texture_data_size += dds.size;
                }
                else
                {
                    fprintf(stderr, "ERROR: unable to load dds image (texture: %u, format: %u)\n", i, (u32)dds.format);
                }
            }
            else
            {
                fprintf(stderr, "ERROR: unable to find source data for texture %u (source: %u)!\n", i, t->dds_extension.dds_image_source_index - 1);
            }
        }
    }

    return texture_data_size;
}

internal u64 gltf_file_get_total_mesh_data_size(GLTF_File* gltf)
{
    u64 mesh_data_size = 0;
    for (u32 i = 0; i < gltf->mesh_count; ++i)
    {
        GLTF_Mesh* gltf_mesh = gltf->meshes + i;

        u32 mesh_size = 0;
        for (u32 j = 0; j < gltf_mesh->primitive_count; ++j)
        {
            GLTF_Primitive* primitive = gltf_mesh->primitives + j;
            if (primitive->indices_accessor_index == 0 || primitive->mode != GLTF_PRIMITIVE_TYPE_TRIANGLE_LIST || primitive->attribute_count == 0)
            {
                fprintf(stderr, "ERROR: mesh %u, primitive %u is NOT supported (only support indexed triangled meshes)!\n", i, j);
            }
            else
            {
                GLTF_Accessor* index_buffer_accessor = gltf->accessors + primitive->indices_accessor_index - 1;
                if (index_buffer_accessor->buffer_view_index == 0)
                {
                    fprintf(stderr, "ERROR: unable to find source index data for mesh %u, primitive %u (source: %u)!\n", i, j, primitive->indices_accessor_index - 1);
                }
                else
                {
                    assert(index_buffer_accessor->data_type == GLTF_DATA_TYPE_SCALAR);

                    u32 supported_attributes = 0;
                    for (u32 k = 0; k < primitive->attribute_count; ++k)
                    {
                        GLTF_Attribute a = primitive->attributes[k];
                        if ((a.type != GLTF_ATTRIBUTE_TYPE_POSITION || (a.type == GLTF_ATTRIBUTE_TYPE_POSITION && a.set_index != 0)) &&
                            (a.type != GLTF_ATTRIBUTE_TYPE_NORMAL || (a.type == GLTF_ATTRIBUTE_TYPE_NORMAL && a.set_index != 0)) &&
                            // (a.type != GLTF_ATTRIBUTE_TYPE_TANGENT || (a.type == GLTF_ATTRIBUTE_TYPE_TANGENT && a.set_index != 0)) ||
                            (a.type != GLTF_ATTRIBUTE_TYPE_TEXCOORD || (a.type == GLTF_ATTRIBUTE_TYPE_TEXCOORD && a.set_index != 0)))
                        {
                            fprintf(stderr, "ERROR: mesh %u, primitive %u has unsupported vertex attribute type: %d, index: %u!\n", i, j, a.type, a.set_index);
                        }
                        else
                        {
                            supported_attributes++;
                        }
                    }
                    if (supported_attributes > 0)
                    {
                        GLTF_Accessor* attribute0_accessor = gltf->accessors + primitive->attributes[0].accessor_index;
                        if (attribute0_accessor->buffer_view_index == 0)
                        {
                            fprintf(stderr, "ERROR: unable to find source vertex data for mesh %u, primitive %u (source: %u)!\n", i, j, primitive->attributes[0].accessor_index);
                        }
                        else
                        {
                            mesh_size += sizeof(Vertex) * attribute0_accessor->element_count + sizeof(u32) * index_buffer_accessor->element_count;
                        }
                    }
                }
            }
        }
        mesh_data_size += ((mesh_size + sizeof(Vertex) - 1) / sizeof(Vertex)) * sizeof(Vertex);
    }

    return mesh_data_size;
}

internal void gltf_to_tsu_meshes(GLTF_File* gltf, TSU_File* tsu)
{
    for (u32 i = 0; i < gltf->mesh_count; ++i)
    {
        GLTF_Mesh* gltf_mesh = gltf->meshes + i;

        Asset_Info* asset = tsu->assets + tsu->current_asset_index;
        asset->type = ASSET_TYPE_MESH;
        asset->data_offset = tsu->current_asset_data_offset + tsu->asset_data_offset;
        asset->data_size = 0;
        tsu->current_asset_index++;

        // NOTE: The mesh's first sub mesh index will also take the 'null' asset into consideration even though it doesn't need to since a sub-mesh will always be after a mesh asset
        asset->mesh_info.first_sub_mesh_index = tsu->current_asset_index + 1;
        asset->mesh_info.sub_mesh_count = gltf_mesh->primitive_count;

        u64 first_vertex_offset = tsu->current_asset_data_offset;
        for (u32 j = 0; j < gltf_mesh->primitive_count; ++j)
        {
            GLTF_Primitive* primitive = gltf_mesh->primitives + j;

            if (primitive->indices_accessor_index != 0 && primitive->mode == GLTF_PRIMITIVE_TYPE_TRIANGLE_LIST && primitive->attribute_count > 0)
            {
                GLTF_Accessor* index_buffer_accessor = gltf->accessors + primitive->indices_accessor_index - 1;
                if (index_buffer_accessor->buffer_view_index != 0)
                {
                    u64 vertex_count = gltf->accessors[primitive->attributes[0].accessor_index].element_count;

                    for (u32 k = 0; k < primitive->attribute_count; ++k)
                    {
                        GLTF_Attribute a = primitive->attributes[k];
                        if ((a.type == GLTF_ATTRIBUTE_TYPE_POSITION && a.set_index == 0) ||
                            (a.type == GLTF_ATTRIBUTE_TYPE_NORMAL && a.set_index == 0) ||
                            // (a.type == GLTF_ATTRIBUTE_TYPE_TANGENT && a.set_index == 0) ||
                            (a.type == GLTF_ATTRIBUTE_TYPE_TEXCOORD && a.set_index == 0))
                        {
                            GLTF_Accessor* vertex_accessor = gltf->accessors + a.accessor_index;
                            if (vertex_accessor->buffer_view_index != 0)
                            {
                                assert(vertex_accessor->element_count == vertex_count);

                                GLTF_Buffer_View* buffer_view = gltf->buffer_views + vertex_accessor->buffer_view_index - 1;
                                GLTF_Buffer* buffer = gltf->buffers + buffer_view->buffer_index;
                                if (buffer->uri.text)
                                {
                                    // TODO: Read buffer file
                                    // TODO: This filepath needs to be relative to the GLTF file
                                }
                                else
                                {
                                    u8* gltf_buffer_data = gltf->bin + buffer_view->offset + vertex_accessor->offset;
                                    u32 stride = buffer_view->stride == 0 ? gltf_sizeof_data_type_with_component_type(vertex_accessor->data_type, vertex_accessor->component_type) : buffer_view->stride;
                                    for (u32 vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
                                    {
                                        u8* element = (u8*)(gltf_buffer_data + stride * vertex_index);
                                        Vertex* v = (Vertex*)(tsu->asset_data + tsu->current_asset_data_offset) + vertex_index;

                                        if (a.type == GLTF_ATTRIBUTE_TYPE_POSITION)
                                        {
                                            v->position = *(v3*)element;
                                        }
                                        else if (a.type == GLTF_ATTRIBUTE_TYPE_NORMAL)
                                        {
                                            v->normal = *(v3*)element;
                                        }
                                        else if (a.type == GLTF_ATTRIBUTE_TYPE_TEXCOORD && a.set_index == 0)
                                        {
                                            v->uv0 = *(v2*)element;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    u32 vertices_size = sizeof(Vertex) * vertex_count;
                    asset->data_size += vertices_size;
                    tsu->current_asset_data_offset += vertices_size;
                }
            }
        }

        u32 index_offset = 0;
        for (u32 j = 0; j < gltf_mesh->primitive_count; ++j)
        {
            GLTF_Primitive* primitive = gltf_mesh->primitives + j;

            Asset_Info* sub_mesh_asset = tsu->assets + tsu->current_asset_index + j;
            sub_mesh_asset->type = ASSET_TYPE_SUB_MESH;
            sub_mesh_asset->data_offset = 0;
            sub_mesh_asset->data_size = 0;
            sub_mesh_asset->sub_mesh_info.material_index = primitive->material_index + (primitive->material_index == 0 ? 0 : tsu->current_material_base_index);

            if (primitive->indices_accessor_index != 0 && primitive->mode == GLTF_PRIMITIVE_TYPE_TRIANGLE_LIST && primitive->attribute_count > 0)
            {
                GLTF_Accessor* index_buffer_accessor = gltf->accessors + primitive->indices_accessor_index - 1;
                if (index_buffer_accessor->buffer_view_index != 0)
                {
                    sub_mesh_asset->sub_mesh_info.index_offset = tsu->current_asset_data_offset - first_vertex_offset;
                    sub_mesh_asset->sub_mesh_info.index_count = index_buffer_accessor->element_count;

                    GLTF_Buffer_View* index_buffer_view = gltf->buffer_views + index_buffer_accessor->buffer_view_index - 1;
                    GLTF_Buffer* index_buffer = gltf->buffers + index_buffer_view->buffer_index;
                    if (index_buffer->uri.text)
                    {
                        // TODO: Read buffer file
                        // TODO: This filepath needs to be relative to the GLTF file
                    }
                    else
                    {
                        u8* gltf_buffer_data = gltf->bin + index_buffer_view->offset + index_buffer_accessor->offset;
                        u32 stride = index_buffer_view->stride == 0 ? gltf_sizeof_data_type_with_component_type(index_buffer_accessor->data_type, index_buffer_accessor->component_type) : index_buffer_view->stride;
                        assert(stride == sizeof(u16) || stride == sizeof(u32));

                        for (u32 element_index = 0; element_index < index_buffer_accessor->element_count; ++element_index)
                        {
                            u8* element = (gltf_buffer_data + stride * element_index);
                            u32 index = stride == sizeof(u16) ? *(u16*)element : *(u32*)element;
                            *((u32*)(tsu->asset_data + tsu->current_asset_data_offset) + element_index) = index + index_offset;
                        }

                        index_offset += (u32)index_buffer_accessor->max.data[0] + 1;

                        u32 indices_size = sizeof(u32) * index_buffer_accessor->element_count;
                        asset->data_size += indices_size;
                        tsu->current_asset_data_offset += indices_size;
                    }
                }
            }
        }

        u32 padded_mesh_size = ((asset->data_size + sizeof(Vertex) - 1) / sizeof(Vertex)) * sizeof(Vertex);
        tsu->current_asset_data_offset += padded_mesh_size - asset->data_size;
        asset->data_size = padded_mesh_size;

        tsu->current_asset_index += gltf_mesh->primitive_count;
    }
}

internal void gltf_to_tsu_materials(GLTF_File* gltf, TSU_File* tsu)
{
    for (u32 i = 0; i < gltf->material_count; ++i)
    {
        GLTF_Material* gltf_material = gltf->materials + i;

        Asset_Info* asset = tsu->assets + tsu->current_asset_index;
        asset->type = ASSET_TYPE_MATERIAL;
        asset->data_offset = 0;
        asset->data_size = 0;

        asset->material_info.material.albedo_texture_id = gltf_material->pbr_metallic_roughness.base_color_texture.texture_index + (gltf_material->pbr_metallic_roughness.base_color_texture.texture_index == 0 ? 0 : tsu->current_texture_base_index);
        asset->material_info.material.normal_texture_id = gltf_material->packing_occlusion_roughness_metallic_extension.normal_texture.texture_index + (gltf_material->packing_occlusion_roughness_metallic_extension.normal_texture.texture_index == 0 ? 0 : tsu->current_texture_base_index);
        asset->material_info.material.occlusion_roughness_metallic_texture_id = gltf_material->packing_occlusion_roughness_metallic_extension.occlusion_roughness_metallic_texture.texture_index + (gltf_material->packing_occlusion_roughness_metallic_extension.occlusion_roughness_metallic_texture.texture_index == 0 ? 0 : tsu->current_texture_base_index);
        asset->material_info.material.base_color_factor = gltf_material->pbr_metallic_roughness.base_color_factor;
        asset->material_info.material.metallic_factor = gltf_material->pbr_metallic_roughness.metallic_factor;
        asset->material_info.material.roughness_factor = gltf_material->pbr_metallic_roughness.roughness_factor;

        tsu->current_asset_index++;
    }
}

internal void gltf_to_tsu_textures(GLTF_File* gltf, TSU_File* tsu)
{
    for (u32 i = 0; i < gltf->texture_count; ++i)
    {
        GLTF_Texture* gltf_texture = gltf->textures + i;

        Asset_Info* asset = tsu->assets + tsu->current_asset_index;
        asset->type = ASSET_TYPE_TEXTURE;
        asset->data_offset = tsu->current_asset_data_offset + tsu->asset_data_offset;

        // NOTE: If the texture source is not DDS we will still keep the texture info at the index
        // TODO: Remove texture infos that we don't use?
        if (gltf_texture->dds_extension.dds_image_source_index != 0)
        {
            GLTF_Image* image = gltf->images + gltf_texture->dds_extension.dds_image_source_index - 1;
            if (image->uri.text)
            {
                // TODO: This filepath needs to be relative to the GLTF file
                // texture_data_size = file_size(image->uri.text);
            }
            else if (image->mime_type.text)
            {
                DDS_Image dds = {gltf->bin + gltf->buffer_views[image->buffer_view_index].offset};
                if (dds_load_image(&dds) && (dds.format == DXGI_FORMAT_BC5_UNORM || dds.format == DXGI_FORMAT_BC7_UNORM || dds.format == DXGI_FORMAT_BC7_UNORM_SRGB))
                {
                    asset->data_size = dds.size;
                    asset->texture_info.width = dds.levels[0].width;
                    asset->texture_info.height = dds.levels[0].height;
                    asset->texture_info.mipmap_count = dds.mipmap_count;
                    #define VK_FORMAT_BC5_UNORM_BLOCK 141
                    #define VK_FORMAT_BC7_UNORM_BLOCK 145
                    #define VK_FORMAT_BC7_SRGB_BLOCK  146
                    asset->texture_info.format = dds.format == DXGI_FORMAT_BC5_UNORM ? VK_FORMAT_BC5_UNORM_BLOCK : (dds.format == DXGI_FORMAT_BC7_UNORM ? VK_FORMAT_BC7_UNORM_BLOCK : VK_FORMAT_BC7_SRGB_BLOCK);

                    copy_memory(tsu->asset_data + tsu->current_asset_data_offset, dds.levels[0].data, dds.size);
                    tsu->current_asset_data_offset += dds.size;
                }
            }
        }
        tsu->current_asset_index++;
    }

    tsu->current_material_base_index = tsu->current_asset_index;
}

internal TSU_File create_tsu_file(GLTF_File* gltf_files, u32 gltf_file_count)
{
    TSU_File tsu_file = {};
    tsu_file.header.magic = TSU_MAGIC;
    tsu_file.header.version = TSU_VERSION;

    for (u32 i = 0; i < gltf_file_count; ++i)
    {
        GLTF_File* gltf = gltf_files + i;
        assert(gltf->texture_count == gltf->image_count);

        tsu_file.texture_count += gltf->texture_count;
        tsu_file.material_count += gltf->material_count;
        tsu_file.mesh_count += gltf->mesh_count;

        for (u32 j = 0; j < gltf->mesh_count; ++j)
        {
            tsu_file.sub_mesh_count += gltf->meshes[j].primitive_count;
        }

        tsu_file.asset_data_size += gltf_file_get_total_texture_data_size(gltf) + gltf_file_get_total_mesh_data_size(gltf);
        tsu_file.header.asset_count += tsu_file.texture_count + tsu_file.material_count + tsu_file.mesh_count + tsu_file.sub_mesh_count;
    }

    tsu_file.assets = (Asset_Info*)malloc(sizeof(Asset_Info) * tsu_file.header.asset_count);
    tsu_file.asset_data = (u8*)malloc(tsu_file.asset_data_size);
    tsu_file.asset_data_offset = sizeof(TSU_Header) + sizeof(Asset_Info) * tsu_file.header.asset_count;

    // NOTE: The asset indices that each asset can refer to already take the 'null' asset into consideration
    for (u32 i = 0; i < gltf_file_count; ++i)
    {
        GLTF_File* gltf = gltf_files + i;

        gltf_to_tsu_textures(gltf, &tsu_file);
        gltf_to_tsu_materials(gltf, &tsu_file);
        gltf_to_tsu_meshes(gltf, &tsu_file);

        tsu_file.current_texture_base_index = tsu_file.current_asset_index;
    }
    assert(tsu_file.current_asset_data_offset == tsu_file.asset_data_size);

    return tsu_file;
}

internal b32 write_tsu_file(TSU_File* tsu_file, char* filename)
{
    FILE* file = fopen(filename, "wb");
    if (!file)
    {
        return false;
    }

    fwrite(&tsu_file->header, sizeof(TSU_Header), 1, file);
    fwrite(tsu_file->assets, sizeof(Asset_Info) * tsu_file->header.asset_count, 1, file);
    fwrite(tsu_file->asset_data, tsu_file->asset_data_size, 1, file);
    fclose(file);
    return true;
};

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("Usage: asset.builder.exe gltf_or_glb_filename output_filename\n");
        return 1;
    }

    char* intput_filename = argv[1];
    char* output_filename = argv[2];

    u32 filename_length = 0;
    for (char* f = intput_filename; *f; ++f, ++filename_length);

    GLTF_File gltf_file = {};
    if (string_equals(intput_filename + filename_length - 5, ".gltf"))
    {
        gltf_file.json = (char*)read_file(intput_filename);
    }
    else if (string_equals(intput_filename + filename_length - 4, ".glb"))
    {
        u8* glb = read_file(intput_filename);
        GLTF_Binary_File_Header* header = (GLTF_Binary_File_Header*)glb;
        GLTF_Binary_Chunk_Header* json_header = (GLTF_Binary_Chunk_Header*)(glb + sizeof(GLTF_Binary_File_Header));
        GLTF_Binary_Chunk_Header* bin_header = (GLTF_Binary_Chunk_Header*)(glb + sizeof(GLTF_Binary_File_Header) + sizeof(GLTF_Binary_Chunk_Header) + json_header->length);

        if (header->magic != GLTF_BINARY_MAGIC || header->version != 2 || json_header->type != GLTF_BINARY_CHUNK_TYPE_JSON || bin_header->type != GLTF_BINARY_CHUNK_TYPE_BIN)
        {
            fprintf(stderr, "ERROR: input file is not a valid '.glb' (binary gltf v2) file!\n");
            return 1;
        }

        gltf_file.json = (char*)(glb + sizeof(GLTF_Binary_File_Header) + sizeof(GLTF_Binary_Chunk_Header));
        gltf_file.json[json_header->length] = 0;

        gltf_file.bin = (glb + sizeof(GLTF_Binary_File_Header) + sizeof(GLTF_Binary_Chunk_Header) * 2 + json_header->length);
    }
    else
    {
        fprintf(stderr, "ERROR: input file is not a '.gltf' or '.glb' file!\n");
        return 1;
    }

    if (!parse_gltf(&gltf_file))
    {
        fprintf(stderr, "ERROR: unable to parse GLTF file: %s!\n", argv[1]);
        return 1;
    }
    printf("Finished parsing GLTF file: %s\n", argv[1]);

    TSU_File tsu_file = create_tsu_file(&gltf_file, 1);
    if (!write_tsu_file(&tsu_file, output_filename))
    {
        fprintf(stderr, "ERROR: unable to write .tsu file!\n");
        return 1;
    }
    printf("Successfully create .tsu file!\n");

    return 0;
}