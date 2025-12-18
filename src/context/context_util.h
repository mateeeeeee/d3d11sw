#pragma once
#include "common/common.h"
#include "resources/subresource_layout.h"
#include "resources/buffer.h"
#include "resources/texture1d.h"
#include "resources/texture2d.h"
#include "resources/texture3d.h"
#include <algorithm>
#include <cmath>

namespace d3d11sw {

inline UINT16 FloatToHalf(FLOAT f)
{
    UINT u = std::bit_cast<UINT>(f);
    UINT sign = (u >> 16) & 0x8000u;
    Int exp = ((u >> 23) & 0xFF) - 127;
    UINT frac = u & 0x007FFFFFu;
    if (exp > 15)
    {
        return (UINT16)(sign | 0x7C00u);
    }
    if (exp < -14)
    {
        return (UINT16)sign;
    }
    return (UINT16)(sign | (UINT)((exp + 15) << 10) | (frac >> 13));
}

inline FLOAT HalfToFloat(UINT16 h)
{
    UINT sign = ((UINT)h & 0x8000u) << 16;
    UINT exp  = (h >> 10) & 0x1F;
    UINT frac = h & 0x03FFu;
    UINT f;
    if (exp == 0)
    {
        f = sign;
    }
    else if (exp == 31)
    {
        f = sign | 0x7F800000u | (frac << 13);
    }
    else
    {
        f = sign | ((exp + 112) << 23) | (frac << 13);
    }
    return std::bit_cast<FLOAT>(f);
}

inline UINT FloatToR11(FLOAT f)
{
    if (f < 0.f) { f = 0.f; }
    UINT u = std::bit_cast<UINT>(f);
    Int exp = ((u >> 23) & 0xFF) - 127;
    UINT frac = u & 0x007FFFFFu;
    if (exp > 15) { return 0x7C0u; }
    if (exp < -14) { return 0; }
    return (UINT)((exp + 15) << 6) | (frac >> 17);
}

inline UINT FloatToR10(FLOAT f)
{
    if (f < 0.f) { f = 0.f; }
    UINT u = std::bit_cast<UINT>(f);
    Int exp = ((u >> 23) & 0xFF) - 127;
    UINT frac = u & 0x007FFFFFu;
    if (exp > 15) { return 0x3E0u; }
    if (exp < -14) { return 0; }
    return (UINT)((exp + 15) << 5) | (frac >> 18);
}

inline FLOAT R11ToFloat(UINT v)
{
    UINT exp  = (v >> 6) & 0x1F;
    UINT frac = v & 0x3F;
    if (exp == 0) { return 0.f; }
    if (exp == 31) { return std::bit_cast<FLOAT>(0x7F800000u | (frac << 17)); }
    return std::bit_cast<FLOAT>(((exp + 112) << 23) | (frac << 17));
}

inline FLOAT R10ToFloat(UINT v)
{
    UINT exp  = (v >> 5) & 0x1F;
    UINT frac = v & 0x1F;
    if (exp == 0) { return 0.f; }
    if (exp == 31) { return std::bit_cast<FLOAT>(0x7F800000u | (frac << 18)); }
    return std::bit_cast<FLOAT>(((exp + 112) << 23) | (frac << 18));
}

template <typename F>
inline void ForEachPixel(UINT8* data, const D3D11SW_SUBRESOURCE_LAYOUT& layout, F fn)
{
    UINT stride = layout.PixelStride;
    for (UINT slice = 0; slice < layout.NumSlices; ++slice)
    {
        UINT8* slicePtr = data + (UINT64)slice * layout.DepthPitch;
        for (UINT row = 0; row < layout.NumRows; ++row)
        {
            UINT8* rowPtr  = slicePtr + (UINT64)row * layout.RowPitch;
            UINT numPixels = layout.RowPitch / stride;
            for (UINT px = 0; px < numPixels; ++px)
            {
                fn(rowPtr + (UINT64)px * stride);
            }
        }
    }
}

inline void PackRTVColor(DXGI_FORMAT fmt, const FLOAT rgba[4], UINT8 out[16])
{
    std::memset(out, 0, 16);
    auto u8  = [](FLOAT f) -> UINT8  { return (UINT8) (std::clamp(f, 0.f, 1.f) * 255.f   + 0.5f); };
    auto s8  = [](FLOAT f) -> INT8   { return (INT8)  (std::clamp(f,-1.f, 1.f) * 127.f   + 0.5f); };
    auto u16 = [](FLOAT f) -> UINT16 { return (UINT16)(std::clamp(f, 0.f, 1.f) * 65535.f + 0.5f); };
    switch (fmt)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            out[0]=u8(rgba[0]); out[1]=u8(rgba[1]); out[2]=u8(rgba[2]); out[3]=u8(rgba[3]); break;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            out[0]=(UINT8)s8(rgba[0]); out[1]=(UINT8)s8(rgba[1]);
            out[2]=(UINT8)s8(rgba[2]); out[3]=(UINT8)s8(rgba[3]); break;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            out[0]=(UINT8)rgba[0]; out[1]=(UINT8)rgba[1]; out[2]=(UINT8)rgba[2]; out[3]=(UINT8)rgba[3]; break;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            out[0]=(UINT8)(INT8)rgba[0]; out[1]=(UINT8)(INT8)rgba[1];
            out[2]=(UINT8)(INT8)rgba[2]; out[3]=(UINT8)(INT8)rgba[3]; break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            out[0]=u8(rgba[2]); out[1]=u8(rgba[1]); out[2]=u8(rgba[0]); out[3]=u8(rgba[3]); break;
        case DXGI_FORMAT_R8G8_UNORM:
            out[0]=u8(rgba[0]); out[1]=u8(rgba[1]); break;
        case DXGI_FORMAT_R8_UNORM:
            out[0]=u8(rgba[0]); break;
        case DXGI_FORMAT_R16G16B16A16_UNORM: 
        {
            UINT16 v[4]={ u16(rgba[0]), u16(rgba[1]), u16(rgba[2]), u16(rgba[3]) };
            std::memcpy(out, v, 8); break;
        }
        case DXGI_FORMAT_R16G16_UNORM: 
        {
            UINT16 v[2]={ u16(rgba[0]), u16(rgba[1]) };
            std::memcpy(out, v, 4); break;
        }
        case DXGI_FORMAT_R16_UNORM: 
        {
            UINT16 v = u16(rgba[0]);
            std::memcpy(out, &v, 2); break;
        }
        case DXGI_FORMAT_R32G32B32A32_FLOAT: std::memcpy(out, rgba,        16); break;
        case DXGI_FORMAT_R32G32B32_FLOAT:    std::memcpy(out, rgba,        12); break;
        case DXGI_FORMAT_R32G32_FLOAT:       std::memcpy(out, rgba,         8); break;
        case DXGI_FORMAT_R32_FLOAT:          std::memcpy(out, rgba,         4); break;
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            UINT u[4]={ (UINT)rgba[0], (UINT)rgba[1], (UINT)rgba[2], (UINT)rgba[3] };
            std::memcpy(out, u, 16); break;
        }
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        {
            UINT u[2]={ (UINT)rgba[0], (UINT)rgba[1] };
            std::memcpy(out, u, 8); break;
        }
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        {
            UINT u = (UINT)rgba[0];
            std::memcpy(out, &u, 4); break;
        }
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        {
            UINT16 v[4]={ FloatToHalf(rgba[0]), FloatToHalf(rgba[1]), FloatToHalf(rgba[2]), FloatToHalf(rgba[3]) };
            std::memcpy(out, v, 8); break;
        }
        case DXGI_FORMAT_R16G16_FLOAT:
        {
            UINT16 v[2]={ FloatToHalf(rgba[0]), FloatToHalf(rgba[1]) };
            std::memcpy(out, v, 4); break;
        }
        case DXGI_FORMAT_R16_FLOAT:
        {
            UINT16 v = FloatToHalf(rgba[0]);
            std::memcpy(out, &v, 2); break;
        }
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        {
            UINT r = (UINT)(std::clamp(rgba[0], 0.f, 1.f) * 1023.f + 0.5f);
            UINT g = (UINT)(std::clamp(rgba[1], 0.f, 1.f) * 1023.f + 0.5f);
            UINT b = (UINT)(std::clamp(rgba[2], 0.f, 1.f) * 1023.f + 0.5f);
            UINT a = (UINT)(std::clamp(rgba[3], 0.f, 1.f) * 3.f    + 0.5f);
            UINT packed = r | (g << 10) | (b << 20) | (a << 30);
            std::memcpy(out, &packed, 4); break;
        }
        case DXGI_FORMAT_R10G10B10A2_UINT:
        {
            UINT r = (UINT)rgba[0] & 0x3FFu;
            UINT g = (UINT)rgba[1] & 0x3FFu;
            UINT b = (UINT)rgba[2] & 0x3FFu;
            UINT a = (UINT)rgba[3] & 0x3u;
            UINT packed = r | (g << 10) | (b << 20) | (a << 30);
            std::memcpy(out, &packed, 4); break;
        }
        case DXGI_FORMAT_R11G11B10_FLOAT:
        {
            UINT packed = FloatToR11(rgba[0]) | (FloatToR11(rgba[1]) << 11) | (FloatToR10(rgba[2]) << 22);
            std::memcpy(out, &packed, 4); break;
        }
        default: break;
    }
}

inline void UnpackColor(DXGI_FORMAT fmt, const UINT8* src, FLOAT rgba[4])
{
    rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0.f;
    switch (fmt)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            rgba[0] = src[0] / 255.f;
            rgba[1] = src[1] / 255.f;
            rgba[2] = src[2] / 255.f;
            rgba[3] = src[3] / 255.f;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            rgba[0] = src[2] / 255.f;
            rgba[1] = src[1] / 255.f;
            rgba[2] = src[0] / 255.f;
            rgba[3] = src[3] / 255.f;
            break;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        {
            rgba[0] = std::max((INT8)src[0] / 127.f, -1.f);
            rgba[1] = std::max((INT8)src[1] / 127.f, -1.f);
            rgba[2] = std::max((INT8)src[2] / 127.f, -1.f);
            rgba[3] = std::max((INT8)src[3] / 127.f, -1.f);
            break;
        }
        case DXGI_FORMAT_R8G8B8A8_UINT:
            rgba[0] = (FLOAT)src[0];
            rgba[1] = (FLOAT)src[1];
            rgba[2] = (FLOAT)src[2];
            rgba[3] = (FLOAT)src[3];
            break;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            rgba[0] = (FLOAT)(INT8)src[0];
            rgba[1] = (FLOAT)(INT8)src[1];
            rgba[2] = (FLOAT)(INT8)src[2];
            rgba[3] = (FLOAT)(INT8)src[3];
            break;
        case DXGI_FORMAT_R8G8_UNORM:
            rgba[0] = src[0] / 255.f;
            rgba[1] = src[1] / 255.f;
            break;
        case DXGI_FORMAT_R8_UNORM:
            rgba[0] = src[0] / 255.f;
            break;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        {
            UINT16 v[4];
            std::memcpy(v, src, 8);
            rgba[0] = v[0] / 65535.f;
            rgba[1] = v[1] / 65535.f;
            rgba[2] = v[2] / 65535.f;
            rgba[3] = v[3] / 65535.f;
            break;
        }
        case DXGI_FORMAT_R16G16_UNORM:
        {
            UINT16 v[2];
            std::memcpy(v, src, 4);
            rgba[0] = v[0] / 65535.f;
            rgba[1] = v[1] / 65535.f;
            break;
        }
        case DXGI_FORMAT_R16_UNORM:
        {
            UINT16 v;
            std::memcpy(&v, src, 2);
            rgba[0] = v / 65535.f;
            break;
        }
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            std::memcpy(rgba, src, 16);
            break;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            std::memcpy(rgba, src, 12);
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
            std::memcpy(rgba, src, 8);
            break;
        case DXGI_FORMAT_R32_FLOAT:
            std::memcpy(rgba, src, 4);
            break;
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            UINT v[4];
            std::memcpy(v, src, 16);
            rgba[0] = (FLOAT)v[0]; rgba[1] = (FLOAT)v[1];
            rgba[2] = (FLOAT)v[2]; rgba[3] = (FLOAT)v[3];
            break;
        }
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        {
            UINT v[2];
            std::memcpy(v, src, 8);
            rgba[0] = (FLOAT)v[0]; rgba[1] = (FLOAT)v[1];
            break;
        }
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        {
            UINT v;
            std::memcpy(&v, src, 4);
            rgba[0] = (FLOAT)v;
            break;
        }
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        {
            UINT16 v[4];
            std::memcpy(v, src, 8);
            rgba[0] = HalfToFloat(v[0]);
            rgba[1] = HalfToFloat(v[1]);
            rgba[2] = HalfToFloat(v[2]);
            rgba[3] = HalfToFloat(v[3]);
            break;
        }
        case DXGI_FORMAT_R16G16_FLOAT:
        {
            UINT16 v[2];
            std::memcpy(v, src, 4);
            rgba[0] = HalfToFloat(v[0]);
            rgba[1] = HalfToFloat(v[1]);
            break;
        }
        case DXGI_FORMAT_R16_FLOAT:
        {
            UINT16 v;
            std::memcpy(&v, src, 2);
            rgba[0] = HalfToFloat(v);
            break;
        }
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        {
            UINT packed;
            std::memcpy(&packed, src, 4);
            rgba[0] = (packed         & 0x3FFu) / 1023.f;
            rgba[1] = ((packed >> 10) & 0x3FFu) / 1023.f;
            rgba[2] = ((packed >> 20) & 0x3FFu) / 1023.f;
            rgba[3] = ((packed >> 30) & 0x3u)   / 3.f;
            break;
        }
        case DXGI_FORMAT_R10G10B10A2_UINT:
        {
            UINT packed;
            std::memcpy(&packed, src, 4);
            rgba[0] = (FLOAT)(packed         & 0x3FFu);
            rgba[1] = (FLOAT)((packed >> 10) & 0x3FFu);
            rgba[2] = (FLOAT)((packed >> 20) & 0x3FFu);
            rgba[3] = (FLOAT)((packed >> 30) & 0x3u);
            break;
        }
        case DXGI_FORMAT_R11G11B10_FLOAT:
        {
            UINT packed;
            std::memcpy(&packed, src, 4);
            rgba[0] = R11ToFloat(packed & 0x7FFu);
            rgba[1] = R11ToFloat((packed >> 11) & 0x7FFu);
            rgba[2] = R10ToFloat((packed >> 22) & 0x3FFu);
            rgba[3] = 1.f;
            break;
        }
        default:
            break;
    }
}

inline void PackUAVUint(DXGI_FORMAT fmt, const UINT values[4], UINT8 out[16])
{
    std::memset(out, 0, 16);
    switch (fmt)
    {
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT: std::memcpy(out, values, 16); break;
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:       std::memcpy(out, values,  8); break;
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:          std::memcpy(out, values,  4); break;
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT: 
        {
            UINT16 v[4]={ (UINT16)values[0], (UINT16)values[1], (UINT16)values[2], (UINT16)values[3] };
            std::memcpy(out, v, 8); break;
        }
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT: 
        {
            UINT16 v[2]={ (UINT16)values[0], (UINT16)values[1] };
            std::memcpy(out, v, 4); break;
        }
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT: 
        {
            UINT16 v = (UINT16)values[0];
            std::memcpy(out, &v, 2); break;
        }
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
            out[0]=(UINT8)values[0]; out[1]=(UINT8)values[1];
            out[2]=(UINT8)values[2]; out[3]=(UINT8)values[3]; break;
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SINT:
            out[0]=(UINT8)values[0]; out[1]=(UINT8)values[1]; break;
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
            out[0]=(UINT8)values[0]; break;
        default: break;
    }
}

template<typename T>
static void SetSlot(T*& slot, T* value)
{
    if (value)
    {
        value->AddRef();
    }
    if (slot)
    {
        slot->Release();
    }
    slot = value;
}

template<typename T, Usize N>
static void SetSlots(T* (&slots)[N], UINT start, UINT count, T* const* values)
{
    for (UINT i = 0; i < count; i++)
    {
        SetSlot(slots[start + i], values ? values[i] : nullptr);
    }
}

template<typename T, Usize N>
static void GetSlots(T* (&slots)[N], UINT start, UINT count, T** out)
{
    if (!out)
    {
        return;
    }

    for (UINT i = 0; i < count; i++)
    {
        out[i] = slots[start + i];
        if (out[i])
        {
            out[i]->AddRef();
        }
    }
}

template<typename Fn>
static void RunOnSWResource(ID3D11Resource* pResource, Fn&& fn)
{
    if (!pResource)
    {
        return;
    }

    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&dim);
    switch (dim)
    {
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            fn(static_cast<D3D11BufferSW*>(static_cast<ID3D11Buffer*>(pResource)));
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            fn(static_cast<D3D11Texture1DSW*>(static_cast<ID3D11Texture1D*>(pResource)));
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            fn(static_cast<D3D11Texture2DSW*>(static_cast<ID3D11Texture2D1*>(pResource)));
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            fn(static_cast<D3D11Texture3DSW*>(static_cast<ID3D11Texture3D1*>(pResource)));
            break;
        default:
            break;
    }
}

}

