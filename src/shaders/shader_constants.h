#pragma once

// Standalone constants for JIT-compiled shaders. No system header dependencies.
namespace d3d11sw {

static constexpr unsigned SW_MAX_VS_INPUTS  = 32;
static constexpr unsigned SW_MAX_VARYINGS   = 32;
static constexpr unsigned SW_MAX_PS_OUTPUTS = 8;
static constexpr unsigned SW_MAX_CBUFS      = 14;
static constexpr unsigned SW_MAX_TEXTURES   = 128;
static constexpr unsigned SW_MAX_SAMPLERS   = 16;
static constexpr unsigned SW_MAX_UAVS       = 64;
static constexpr unsigned SW_MAX_TGSM       = 8;
static constexpr unsigned SW_MAX_MIP_LEVELS = 15;
static constexpr unsigned SW_MAX_GS_INPUT_VERTS  = 6;
static constexpr unsigned SW_MAX_GS_OUTPUT_VERTS = 1024;
static constexpr unsigned SW_MAX_PATCH_CP        = 32;
static constexpr unsigned SW_MAX_PATCH_CONSTANTS = 32;

static constexpr unsigned SW_FORMAT_R32G32B32A32_FLOAT    = 2;
static constexpr unsigned SW_FORMAT_R32G32B32A32_UINT     = 3;
static constexpr unsigned SW_FORMAT_R32G32B32A32_SINT     = 4;
static constexpr unsigned SW_FORMAT_R32G32B32_FLOAT       = 6;
static constexpr unsigned SW_FORMAT_R16G16B16A16_FLOAT    = 10;
static constexpr unsigned SW_FORMAT_R16G16B16A16_UNORM    = 11;
static constexpr unsigned SW_FORMAT_R16G16B16A16_UINT     = 12;
static constexpr unsigned SW_FORMAT_R16G16B16A16_SNORM    = 13;
static constexpr unsigned SW_FORMAT_R16G16B16A16_SINT     = 14;
static constexpr unsigned SW_FORMAT_R32G32_FLOAT          = 16;
static constexpr unsigned SW_FORMAT_R32G32_UINT           = 17;
static constexpr unsigned SW_FORMAT_R32G32_SINT           = 18;
static constexpr unsigned SW_FORMAT_R10G10B10A2_UNORM     = 24;
static constexpr unsigned SW_FORMAT_R10G10B10A2_UINT      = 25;
static constexpr unsigned SW_FORMAT_R11G11B10_FLOAT       = 26;
static constexpr unsigned SW_FORMAT_R8G8B8A8_UNORM        = 28;
static constexpr unsigned SW_FORMAT_R8G8B8A8_UNORM_SRGB   = 29;
static constexpr unsigned SW_FORMAT_R8G8B8A8_UINT         = 30;
static constexpr unsigned SW_FORMAT_R8G8B8A8_SNORM        = 31;
static constexpr unsigned SW_FORMAT_R8G8B8A8_SINT         = 32;
static constexpr unsigned SW_FORMAT_R16G16_FLOAT          = 34;
static constexpr unsigned SW_FORMAT_R16G16_UNORM          = 35;
static constexpr unsigned SW_FORMAT_R16G16_UINT           = 36;
static constexpr unsigned SW_FORMAT_R16G16_SNORM          = 37;
static constexpr unsigned SW_FORMAT_R16G16_SINT           = 38;
static constexpr unsigned SW_FORMAT_R32_FLOAT             = 41;
static constexpr unsigned SW_FORMAT_R32_UINT              = 42;
static constexpr unsigned SW_FORMAT_R32_SINT              = 43;
static constexpr unsigned SW_FORMAT_R8G8_UNORM            = 49;
static constexpr unsigned SW_FORMAT_R8G8_UINT             = 50;
static constexpr unsigned SW_FORMAT_R8G8_SNORM            = 51;
static constexpr unsigned SW_FORMAT_R8G8_SINT             = 52;
static constexpr unsigned SW_FORMAT_R16_FLOAT             = 54;
static constexpr unsigned SW_FORMAT_R16_UNORM             = 56;
static constexpr unsigned SW_FORMAT_R16_UINT              = 57;
static constexpr unsigned SW_FORMAT_R16_SNORM             = 58;
static constexpr unsigned SW_FORMAT_R16_SINT              = 59;
static constexpr unsigned SW_FORMAT_R8_UNORM              = 61;
static constexpr unsigned SW_FORMAT_R8_UINT               = 62;
static constexpr unsigned SW_FORMAT_R8_SNORM              = 63;
static constexpr unsigned SW_FORMAT_R8_SINT               = 64;
static constexpr unsigned SW_FORMAT_B8G8R8A8_UNORM        = 87;
static constexpr unsigned SW_FORMAT_B8G8R8X8_UNORM        = 88;
static constexpr unsigned SW_FORMAT_B8G8R8A8_UNORM_SRGB   = 91;
static constexpr unsigned SW_FORMAT_B8G8R8X8_UNORM_SRGB   = 93;

static constexpr unsigned SW_FORMAT_BC1_UNORM             = 71;
static constexpr unsigned SW_FORMAT_BC1_UNORM_SRGB        = 72;
static constexpr unsigned SW_FORMAT_BC2_UNORM             = 74;
static constexpr unsigned SW_FORMAT_BC2_UNORM_SRGB        = 75;
static constexpr unsigned SW_FORMAT_BC3_UNORM             = 77;
static constexpr unsigned SW_FORMAT_BC3_UNORM_SRGB        = 78;
static constexpr unsigned SW_FORMAT_BC4_UNORM             = 80;
static constexpr unsigned SW_FORMAT_BC4_SNORM             = 81;
static constexpr unsigned SW_FORMAT_BC5_UNORM             = 83;
static constexpr unsigned SW_FORMAT_BC5_SNORM             = 84;
static constexpr unsigned SW_FORMAT_BC7_UNORM             = 98;
static constexpr unsigned SW_FORMAT_BC7_UNORM_SRGB        = 99;

static constexpr unsigned SW_FORMAT_D32_FLOAT             = 40;
static constexpr unsigned SW_FORMAT_D16_UNORM             = 55;
static constexpr unsigned SW_FORMAT_D24_UNORM_S8_UINT     = 45;
static constexpr unsigned SW_FORMAT_D32_FLOAT_S8X24_UINT  = 20;

static constexpr unsigned SW_TEXTURE_ADDRESS_WRAP        = 1;
static constexpr unsigned SW_TEXTURE_ADDRESS_MIRROR      = 2;
static constexpr unsigned SW_TEXTURE_ADDRESS_CLAMP       = 3;
static constexpr unsigned SW_TEXTURE_ADDRESS_BORDER      = 4;
static constexpr unsigned SW_TEXTURE_ADDRESS_MIRROR_ONCE = 5;

static constexpr unsigned SW_FILTER_MIN_MAG_MIP_POINT        = 0;
static constexpr unsigned SW_FILTER_MIN_MAG_POINT_MIP_LINEAR = 1;
static constexpr unsigned SW_FILTER_MIN_MAG_MIP_LINEAR       = 0x15;

static constexpr unsigned SW_COMPARISON_NEVER         = 1;
static constexpr unsigned SW_COMPARISON_LESS          = 2;
static constexpr unsigned SW_COMPARISON_EQUAL         = 3;
static constexpr unsigned SW_COMPARISON_LESS_EQUAL    = 4;
static constexpr unsigned SW_COMPARISON_GREATER       = 5;
static constexpr unsigned SW_COMPARISON_NOT_EQUAL     = 6;
static constexpr unsigned SW_COMPARISON_GREATER_EQUAL = 7;
static constexpr unsigned SW_COMPARISON_ALWAYS        = 8;

static constexpr unsigned SW_UAV_DIMENSION_BUFFER    = 1;
static constexpr unsigned SW_UAV_DIMENSION_TEXTURE2D = 4;

}
