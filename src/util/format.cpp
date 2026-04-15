#include "util/format.h"

namespace d3d11sw {

UINT GetFormatSupport(DXGI_FORMAT format)
{
    static constexpr UINT kColor =
        D3D11_FORMAT_SUPPORT_BUFFER            |
        D3D11_FORMAT_SUPPORT_TEXTURE1D         |
        D3D11_FORMAT_SUPPORT_TEXTURE2D         |
        D3D11_FORMAT_SUPPORT_TEXTURE3D         |
        D3D11_FORMAT_SUPPORT_TEXTURECUBE       |
        D3D11_FORMAT_SUPPORT_SHADER_LOAD       |
        D3D11_FORMAT_SUPPORT_SHADER_SAMPLE     |
        D3D11_FORMAT_SUPPORT_SHADER_GATHER     |
        D3D11_FORMAT_SUPPORT_MIP               |
        D3D11_FORMAT_SUPPORT_MIP_AUTOGEN       |
        D3D11_FORMAT_SUPPORT_RENDER_TARGET     |
        D3D11_FORMAT_SUPPORT_BLENDABLE         |
        D3D11_FORMAT_SUPPORT_CPU_LOCKABLE;

    static constexpr UINT kDepth =
        D3D11_FORMAT_SUPPORT_TEXTURE2D     |
        D3D11_FORMAT_SUPPORT_DEPTH_STENCIL |
        D3D11_FORMAT_SUPPORT_CPU_LOCKABLE;

    static constexpr UINT kBC =
        D3D11_FORMAT_SUPPORT_TEXTURE1D     |
        D3D11_FORMAT_SUPPORT_TEXTURE2D     |
        D3D11_FORMAT_SUPPORT_TEXTURE3D     |
        D3D11_FORMAT_SUPPORT_TEXTURECUBE   |
        D3D11_FORMAT_SUPPORT_SHADER_LOAD   |
        D3D11_FORMAT_SUPPORT_SHADER_SAMPLE |
        D3D11_FORMAT_SUPPORT_SHADER_GATHER |
        D3D11_FORMAT_SUPPORT_MIP           |
        D3D11_FORMAT_SUPPORT_CPU_LOCKABLE;

    static constexpr UINT kTypeless =
        D3D11_FORMAT_SUPPORT_TEXTURE1D              |
        D3D11_FORMAT_SUPPORT_TEXTURE2D              |
        D3D11_FORMAT_SUPPORT_TEXTURE3D              |
        D3D11_FORMAT_SUPPORT_CPU_LOCKABLE           |
        D3D11_FORMAT_SUPPORT_CAST_WITHIN_BIT_LAYOUT;

    switch (format)
    {
    case DXGI_FORMAT_UNKNOWN: return 0;

    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return kDepth;

    case DXGI_FORMAT_BC1_UNORM:     case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM:     case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM:     case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_UNORM:     case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_UNORM:     case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_UF16:     case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_UNORM:     case DXGI_FORMAT_BC7_UNORM_SRGB:
        return kBC;
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_R16_TYPELESS:
        return kTypeless;
    default:
        return kColor;
    }
}

HRESULT ValidateTextureBindFlags(DXGI_FORMAT format, UINT bindFlags, Bool depthStencilAllowed)
{
    const Bool isDepth = IsDepthFormat(format);
    const Bool isBC    = IsBlockCompressedFormat(format);

    if (!depthStencilAllowed && (bindFlags & D3D11_BIND_DEPTH_STENCIL))
    {
        return E_INVALIDARG;
    }

    if (isDepth && (bindFlags & D3D11_BIND_RENDER_TARGET))
    {
        return E_INVALIDARG;
    }

    if (!isDepth && !IsDepthCompatible(format) && (bindFlags & D3D11_BIND_DEPTH_STENCIL))
    {
        return E_INVALIDARG;
    }

    if (isBC && (bindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_UNORDERED_ACCESS)))
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

}
