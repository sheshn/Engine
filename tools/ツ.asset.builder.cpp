#include <stdio.h>
#include <stdlib.h>

#include "../ツ/ツ.common.h"
#include "../ツ/ツ.math.h"

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


u8* read_file(wchar_t* filepath)
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

void write_file(wchar_t* filepath, u8* contents, u32 size)
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

int main()
{
    DDS_Image image;
    image.data = read_file(L"../data/image1.dds");

    if (!dds_load_image(&image))
    {
        fprintf(stderr, "Unable to load DDS image!\n");
        return 1;
    }

    // TODO: Actual asset file format!
    write_file(L"../data/image1.tsu", image.levels[0].data, image.size);
    return 0;
}