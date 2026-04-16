#pragma once
#include "common/common.h"
#include <dxgiformat.h>
#include <d3d11.h>
#include <algorithm>
#include "util/align.h"

namespace d3d11sw
{

    inline constexpr Uint32 GetFormatStride(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 8u;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 16u;
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 12u;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return 8u;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
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
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return 4u;
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
            return 2u;
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
            return 1u;
        default:
            return 16u;
        }
    }

    inline constexpr Uint32 GetFormatBlockSize(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 4u;
        default:
            return 1u;
        }
    }

    inline constexpr Bool IsDepthFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return true;
        default:
            return false;
        }
    }

    inline constexpr Bool IsDepthCompatible(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return true;
        default:
            return false;
        }
    }

    inline constexpr Bool IsBlockCompressedFormat(DXGI_FORMAT format)
    {
        return GetFormatBlockSize(format) > 1u;
    }

    inline Uint64 GetRowPitch(DXGI_FORMAT format, Uint32 width, Uint32 mip = 0)
    {
        Uint32 block = GetFormatBlockSize(format);
        Uint64 num_blocks = std::max(1u, DivideAndRoundUp(width >> mip, block));
        return num_blocks * GetFormatStride(format);
    }

    inline Uint64 GetSlicePitch(DXGI_FORMAT format, Uint32 width, Uint32 height, Uint32 mip = 0)
    {
        Uint32 block = GetFormatBlockSize(format);
        Uint64 num_blocks_x = std::max(1u, DivideAndRoundUp(width  >> mip, block));
        Uint64 num_blocks_y = std::max(1u, DivideAndRoundUp(height >> mip, block));
        return num_blocks_x * num_blocks_y * GetFormatStride(format);
    }

    inline Uint64 GetTextureMipByteSize(DXGI_FORMAT format, Uint32 width, Uint32 height, Uint32 depth, Uint32 mip)
    {
        return GetSlicePitch(format, width, height, mip) * std::max(1u, depth >> mip);
    }

    inline Uint64 GetTextureByteSize(DXGI_FORMAT format, Uint32 width, Uint32 height, Uint32 depth = 1, Uint32 mip_count = 1)
    {
        Uint64 size = 0;
        for (Uint32 mip = 0; mip < mip_count; ++mip)
            size += GetTextureMipByteSize(format, width, height, depth, mip);
        return size;
    }

    inline constexpr Char const* FormatToString(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_UNKNOWN:                  return "UNKNOWN";
        case DXGI_FORMAT_R32G32B32A32_FLOAT:       return "R32G32B32A32_FLOAT";
        case DXGI_FORMAT_R32G32B32A32_UINT:        return "R32G32B32A32_UINT";
        case DXGI_FORMAT_R32G32B32A32_SINT:        return "R32G32B32A32_SINT";
        case DXGI_FORMAT_R32G32B32_FLOAT:          return "R32G32B32_FLOAT";
        case DXGI_FORMAT_R32G32B32_UINT:           return "R32G32B32_UINT";
        case DXGI_FORMAT_R32G32B32_SINT:           return "R32G32B32_SINT";
        case DXGI_FORMAT_R16G16B16A16_FLOAT:       return "R16G16B16A16_FLOAT";
        case DXGI_FORMAT_R16G16B16A16_UNORM:       return "R16G16B16A16_UNORM";
        case DXGI_FORMAT_R16G16B16A16_UINT:        return "R16G16B16A16_UINT";
        case DXGI_FORMAT_R16G16B16A16_SNORM:       return "R16G16B16A16_SNORM";
        case DXGI_FORMAT_R16G16B16A16_SINT:        return "R16G16B16A16_SINT";
        case DXGI_FORMAT_R32G32_FLOAT:             return "R32G32_FLOAT";
        case DXGI_FORMAT_R32G32_UINT:              return "R32G32_UINT";
        case DXGI_FORMAT_R32G32_SINT:              return "R32G32_SINT";
        case DXGI_FORMAT_R32G8X24_TYPELESS:        return "R32G8X24_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:     return "D32_FLOAT_S8X24_UINT";
        case DXGI_FORMAT_R10G10B10A2_UNORM:        return "R10G10B10A2_UNORM";
        case DXGI_FORMAT_R10G10B10A2_UINT:         return "R10G10B10A2_UINT";
        case DXGI_FORMAT_R11G11B10_FLOAT:          return "R11G11B10_FLOAT";
        case DXGI_FORMAT_R8G8B8A8_UNORM:           return "R8G8B8A8_UNORM";
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:      return "R8G8B8A8_UNORM_SRGB";
        case DXGI_FORMAT_R8G8B8A8_UINT:            return "R8G8B8A8_UINT";
        case DXGI_FORMAT_R8G8B8A8_SNORM:           return "R8G8B8A8_SNORM";
        case DXGI_FORMAT_R8G8B8A8_SINT:            return "R8G8B8A8_SINT";
        case DXGI_FORMAT_B8G8R8A8_UNORM:           return "B8G8R8A8_UNORM";
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:      return "B8G8R8A8_UNORM_SRGB";
        case DXGI_FORMAT_R16G16_FLOAT:             return "R16G16_FLOAT";
        case DXGI_FORMAT_R16G16_UNORM:             return "R16G16_UNORM";
        case DXGI_FORMAT_R16G16_UINT:              return "R16G16_UINT";
        case DXGI_FORMAT_R16G16_SNORM:             return "R16G16_SNORM";
        case DXGI_FORMAT_R16G16_SINT:              return "R16G16_SINT";
        case DXGI_FORMAT_R32_TYPELESS:             return "R32_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT:                return "D32_FLOAT";
        case DXGI_FORMAT_R32_FLOAT:                return "R32_FLOAT";
        case DXGI_FORMAT_R32_UINT:                 return "R32_UINT";
        case DXGI_FORMAT_R32_SINT:                 return "R32_SINT";
        case DXGI_FORMAT_R24G8_TYPELESS:           return "R24G8_TYPELESS";
        case DXGI_FORMAT_D24_UNORM_S8_UINT:        return "D24_UNORM_S8_UINT";
        case DXGI_FORMAT_R8G8_UNORM:               return "R8G8_UNORM";
        case DXGI_FORMAT_R8G8_UINT:                return "R8G8_UINT";
        case DXGI_FORMAT_R8G8_SNORM:               return "R8G8_SNORM";
        case DXGI_FORMAT_R8G8_SINT:                return "R8G8_SINT";
        case DXGI_FORMAT_R16_TYPELESS:             return "R16_TYPELESS";
        case DXGI_FORMAT_R16_FLOAT:                return "R16_FLOAT";
        case DXGI_FORMAT_D16_UNORM:                return "D16_UNORM";
        case DXGI_FORMAT_R16_UNORM:                return "R16_UNORM";
        case DXGI_FORMAT_R16_UINT:                 return "R16_UINT";
        case DXGI_FORMAT_R16_SNORM:                return "R16_SNORM";
        case DXGI_FORMAT_R16_SINT:                 return "R16_SINT";
        case DXGI_FORMAT_R8_UNORM:                 return "R8_UNORM";
        case DXGI_FORMAT_R8_UINT:                  return "R8_UINT";
        case DXGI_FORMAT_R8_SNORM:                 return "R8_SNORM";
        case DXGI_FORMAT_R8_SINT:                  return "R8_SINT";
        case DXGI_FORMAT_BC1_UNORM:                return "BC1_UNORM";
        case DXGI_FORMAT_BC1_UNORM_SRGB:           return "BC1_UNORM_SRGB";
        case DXGI_FORMAT_BC2_UNORM:                return "BC2_UNORM";
        case DXGI_FORMAT_BC2_UNORM_SRGB:           return "BC2_UNORM_SRGB";
        case DXGI_FORMAT_BC3_UNORM:                return "BC3_UNORM";
        case DXGI_FORMAT_BC3_UNORM_SRGB:           return "BC3_UNORM_SRGB";
        case DXGI_FORMAT_BC4_UNORM:                return "BC4_UNORM";
        case DXGI_FORMAT_BC4_SNORM:                return "BC4_SNORM";
        case DXGI_FORMAT_BC5_UNORM:                return "BC5_UNORM";
        case DXGI_FORMAT_BC5_SNORM:                return "BC5_SNORM";
        case DXGI_FORMAT_BC6H_UF16:                return "BC6H_UF16";
        case DXGI_FORMAT_BC6H_SF16:                return "BC6H_SF16";
        case DXGI_FORMAT_BC7_UNORM:                return "BC7_UNORM";
        case DXGI_FORMAT_BC7_UNORM_SRGB:           return "BC7_UNORM_SRGB";
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:       return "R9G9B9E5_SHAREDEXP";
        default:                                   return "INVALID";
        }
    }

    UINT GetFormatSupport(DXGI_FORMAT format);
    HRESULT ValidateTextureBindFlags(DXGI_FORMAT format, UINT bindFlags, Bool depthStencilAllowed);
}
