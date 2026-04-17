#pragma once
#include "common/common.h"
#include "resources/subresource_layout.h"
#include "resources/buffer.h"
#include "resources/texture1d.h"
#include "resources/texture2d.h"
#include "resources/texture3d.h"
#include <algorithm>
#include <cmath>
#include <bit>

namespace d3d11sw {

inline Float SrgbToLinear(Float s)
{
    if (s <= 0.04045f) { return s / 12.92f; }
    return std::pow((s + 0.055f) / 1.055f, 2.4f);
}

inline Float LinearToSrgb(Float l)
{
    if (l <= 0.0031308f) { return l * 12.92f; }
    return 1.055f * std::pow(l, 1.f / 2.4f) - 0.055f;
}

inline constexpr Uint16 FloatToHalf(Float f)
{
    Uint u = std::bit_cast<Uint>(f);
    Uint sign = (u >> 16) & 0x8000u;
    Int exp = ((u >> 23) & 0xFF) - 127;
    Uint frac = u & 0x007FFFFFu;
    if (exp > 15)
    {
        return (Uint16)(sign | 0x7C00u);
    }
    if (exp < -14)
    {
        return (Uint16)sign;
    }
    return (Uint16)(sign | (Uint)((exp + 15) << 10) | (frac >> 13));
}

inline constexpr Float HalfToFloat(Uint16 h)
{
    Uint sign = ((Uint)h & 0x8000u) << 16;
    Uint exp  = (h >> 10) & 0x1F;
    Uint frac = h & 0x03FFu;
    Uint f;
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
    return std::bit_cast<Float>(f);
}

inline constexpr Uint FloatToR11(Float f)
{
    if (f < 0.f) { f = 0.f; }
    Uint u = std::bit_cast<Uint>(f);
    Int exp = ((u >> 23) & 0xFF) - 127;
    Uint frac = u & 0x007FFFFFu;
    if (exp > 15) { return 0x7C0u; }
    if (exp < -14) { return 0; }
    return (Uint)((exp + 15) << 6) | (frac >> 17);
}

inline constexpr Uint FloatToR10(Float f)
{
    if (f < 0.f) { f = 0.f; }
    Uint u = std::bit_cast<Uint>(f);
    Int exp = ((u >> 23) & 0xFF) - 127;
    Uint frac = u & 0x007FFFFFu;
    if (exp > 15) { return 0x3E0u; }
    if (exp < -14) { return 0; }
    return (Uint)((exp + 15) << 5) | (frac >> 18);
}

inline constexpr Float R11ToFloat(Uint v)
{
    Uint exp  = (v >> 6) & 0x1F;
    Uint frac = v & 0x3F;
    if (exp == 0) { return 0.f; }
    if (exp == 31) { return std::bit_cast<Float>(0x7F800000u | (frac << 17)); }
    return std::bit_cast<Float>(((exp + 112) << 23) | (frac << 17));
}

inline constexpr Float R10ToFloat(Uint v)
{
    Uint exp  = (v >> 5) & 0x1F;
    Uint frac = v & 0x1F;
    if (exp == 0) { return 0.f; }
    if (exp == 31) { return std::bit_cast<Float>(0x7F800000u | (frac << 18)); }
    return std::bit_cast<Float>(((exp + 112) << 23) | (frac << 18));
}

template <typename F>
inline void ForEachPixel(Uint8* data, const D3D11SW_SUBRESOURCE_LAYOUT& layout, F fn)
{
    Uint stride = layout.PixelStride;
    for (Uint slice = 0; slice < layout.NumSlices; ++slice)
    {
        Uint8* slicePtr = data + (Uint64)slice * layout.DepthPitch;
        for (Uint row = 0; row < layout.NumRows; ++row)
        {
            Uint8* rowPtr  = slicePtr + (Uint64)row * layout.RowPitch;
            Uint numPixels = layout.RowPitch / stride;
            for (Uint px = 0; px < numPixels; ++px)
            {
                fn(rowPtr + (Uint64)px * stride);
            }
        }
    }
}

inline void PackColor(DXGI_FORMAT fmt, const Float rgba[4], Uint8 out[16])
{
    std::memset(out, 0, 16);
    auto u8  = [](Float f) -> Uint8  { return (Uint8) (std::clamp(f, 0.f, 1.f) * 255.f   + 0.5f); };
    auto s8  = [](Float f) -> Int8   { return (Int8)  (std::clamp(f,-1.f, 1.f) * 127.f   + 0.5f); };
    auto u16 = [](Float f) -> Uint16 { return (Uint16)(std::clamp(f, 0.f, 1.f) * 65535.f + 0.5f); };
    switch (fmt)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            out[0]=u8(rgba[0]); out[1]=u8(rgba[1]); out[2]=u8(rgba[2]); out[3]=u8(rgba[3]); break;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            out[0]=u8(LinearToSrgb(rgba[0])); out[1]=u8(LinearToSrgb(rgba[1])); out[2]=u8(LinearToSrgb(rgba[2])); out[3]=u8(rgba[3]); break;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            out[0]=(Uint8)s8(rgba[0]); out[1]=(Uint8)s8(rgba[1]);
            out[2]=(Uint8)s8(rgba[2]); out[3]=(Uint8)s8(rgba[3]); break;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            out[0]=(Uint8)rgba[0]; out[1]=(Uint8)rgba[1]; out[2]=(Uint8)rgba[2]; out[3]=(Uint8)rgba[3]; break;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            out[0]=(Uint8)(Int8)rgba[0]; out[1]=(Uint8)(Int8)rgba[1];
            out[2]=(Uint8)(Int8)rgba[2]; out[3]=(Uint8)(Int8)rgba[3]; break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            out[0]=u8(rgba[2]); out[1]=u8(rgba[1]); out[2]=u8(rgba[0]); out[3]=u8(rgba[3]); break;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            out[0]=u8(LinearToSrgb(rgba[2])); out[1]=u8(LinearToSrgb(rgba[1])); out[2]=u8(LinearToSrgb(rgba[0])); out[3]=u8(rgba[3]); break;
        case DXGI_FORMAT_R8G8_UNORM:
            out[0]=u8(rgba[0]); out[1]=u8(rgba[1]); break;
        case DXGI_FORMAT_R8_UNORM:
            out[0]=u8(rgba[0]); break;
        case DXGI_FORMAT_R16G16B16A16_UNORM: 
        {
            Uint16 v[4]={ u16(rgba[0]), u16(rgba[1]), u16(rgba[2]), u16(rgba[3]) };
            std::memcpy(out, v, 8); break;
        }
        case DXGI_FORMAT_R16G16_UNORM: 
        {
            Uint16 v[2]={ u16(rgba[0]), u16(rgba[1]) };
            std::memcpy(out, v, 4); break;
        }
        case DXGI_FORMAT_R16_UNORM: 
        {
            Uint16 v = u16(rgba[0]);
            std::memcpy(out, &v, 2); break;
        }
        case DXGI_FORMAT_R32G32B32A32_FLOAT: std::memcpy(out, rgba,        16); break;
        case DXGI_FORMAT_R32G32B32_FLOAT:    std::memcpy(out, rgba,        12); break;
        case DXGI_FORMAT_R32G32_FLOAT:       std::memcpy(out, rgba,         8); break;
        case DXGI_FORMAT_R32_FLOAT:          std::memcpy(out, rgba,         4); break;
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            Uint u[4]={ (Uint)rgba[0], (Uint)rgba[1], (Uint)rgba[2], (Uint)rgba[3] };
            std::memcpy(out, u, 16); break;
        }
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        {
            Uint u[2]={ (Uint)rgba[0], (Uint)rgba[1] };
            std::memcpy(out, u, 8); break;
        }
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        {
            Uint u = (Uint)rgba[0];
            std::memcpy(out, &u, 4); break;
        }
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        {
            Uint16 v[4]={ FloatToHalf(rgba[0]), FloatToHalf(rgba[1]), FloatToHalf(rgba[2]), FloatToHalf(rgba[3]) };
            std::memcpy(out, v, 8); break;
        }
        case DXGI_FORMAT_R16G16_FLOAT:
        {
            Uint16 v[2]={ FloatToHalf(rgba[0]), FloatToHalf(rgba[1]) };
            std::memcpy(out, v, 4); break;
        }
        case DXGI_FORMAT_R16_FLOAT:
        {
            Uint16 v = FloatToHalf(rgba[0]);
            std::memcpy(out, &v, 2); break;
        }
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        {
            Uint r = (Uint)(std::clamp(rgba[0], 0.f, 1.f) * 1023.f + 0.5f);
            Uint g = (Uint)(std::clamp(rgba[1], 0.f, 1.f) * 1023.f + 0.5f);
            Uint b = (Uint)(std::clamp(rgba[2], 0.f, 1.f) * 1023.f + 0.5f);
            Uint a = (Uint)(std::clamp(rgba[3], 0.f, 1.f) * 3.f    + 0.5f);
            Uint packed = r | (g << 10) | (b << 20) | (a << 30);
            std::memcpy(out, &packed, 4); break;
        }
        case DXGI_FORMAT_R10G10B10A2_UINT:
        {
            Uint r = (Uint)rgba[0] & 0x3FFu;
            Uint g = (Uint)rgba[1] & 0x3FFu;
            Uint b = (Uint)rgba[2] & 0x3FFu;
            Uint a = (Uint)rgba[3] & 0x3u;
            Uint packed = r | (g << 10) | (b << 20) | (a << 30);
            std::memcpy(out, &packed, 4); break;
        }
        case DXGI_FORMAT_R11G11B10_FLOAT:
        {
            Uint packed = FloatToR11(rgba[0]) | (FloatToR11(rgba[1]) << 11) | (FloatToR10(rgba[2]) << 22);
            std::memcpy(out, &packed, 4); break;
        }
        default: break;
    }
}

inline void UnpackColor(DXGI_FORMAT fmt, const Uint8* src, FLOAT rgba[4])
{
    rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0.f;
    switch (fmt)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            rgba[0] = src[0] / 255.f;
            rgba[1] = src[1] / 255.f;
            rgba[2] = src[2] / 255.f;
            rgba[3] = src[3] / 255.f;
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            rgba[0] = SrgbToLinear(src[0] / 255.f);
            rgba[1] = SrgbToLinear(src[1] / 255.f);
            rgba[2] = SrgbToLinear(src[2] / 255.f);
            rgba[3] = src[3] / 255.f;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            rgba[0] = src[2] / 255.f;
            rgba[1] = src[1] / 255.f;
            rgba[2] = src[0] / 255.f;
            rgba[3] = src[3] / 255.f;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            rgba[0] = SrgbToLinear(src[2] / 255.f);
            rgba[1] = SrgbToLinear(src[1] / 255.f);
            rgba[2] = SrgbToLinear(src[0] / 255.f);
            rgba[3] = src[3] / 255.f;
            break;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        {
            rgba[0] = std::fmax((Int8)src[0] / 127.f, -1.f);
            rgba[1] = std::fmax((Int8)src[1] / 127.f, -1.f);
            rgba[2] = std::fmax((Int8)src[2] / 127.f, -1.f);
            rgba[3] = std::fmax((Int8)src[3] / 127.f, -1.f);
            break;
        }
        case DXGI_FORMAT_R8G8B8A8_UINT:
            rgba[0] = (Float)src[0];
            rgba[1] = (Float)src[1];
            rgba[2] = (Float)src[2];
            rgba[3] = (Float)src[3];
            break;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            rgba[0] = (Float)(Int8)src[0];
            rgba[1] = (Float)(Int8)src[1];
            rgba[2] = (Float)(Int8)src[2];
            rgba[3] = (Float)(Int8)src[3];
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
            Uint16 v[4];
            std::memcpy(v, src, 8);
            rgba[0] = v[0] / 65535.f;
            rgba[1] = v[1] / 65535.f;
            rgba[2] = v[2] / 65535.f;
            rgba[3] = v[3] / 65535.f;
            break;
        }
        case DXGI_FORMAT_R16G16_UNORM:
        {
            Uint16 v[2];
            std::memcpy(v, src, 4);
            rgba[0] = v[0] / 65535.f;
            rgba[1] = v[1] / 65535.f;
            break;
        }
        case DXGI_FORMAT_R16_UNORM:
        {
            Uint16 v;
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
            Uint v[4];
            std::memcpy(v, src, 16);
			rgba[0] = (Float)v[0]; rgba[1] = (Float)v[1];
			rgba[2] = (Float)v[2]; rgba[3] = (Float)v[3];
            break;
        }
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        {
            Uint v[2];
            std::memcpy(v, src, 8);
            rgba[0] = (Float)v[0]; rgba[1] = (Float)v[1];
            break;
        }
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        {
            Uint v;
            std::memcpy(&v, src, 4);
            rgba[0] = (Float)v;
            break;
        }
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        {
            Uint16 v[4];
            std::memcpy(v, src, 8);
            rgba[0] = HalfToFloat(v[0]);
            rgba[1] = HalfToFloat(v[1]);
            rgba[2] = HalfToFloat(v[2]);
            rgba[3] = HalfToFloat(v[3]);
            break;
        }
        case DXGI_FORMAT_R16G16_FLOAT:
        {
            Uint16 v[2];
            std::memcpy(v, src, 4);
            rgba[0] = HalfToFloat(v[0]);
            rgba[1] = HalfToFloat(v[1]);
            break;
        }
        case DXGI_FORMAT_R16_FLOAT:
        {
            Uint16 v;
            std::memcpy(&v, src, 2);
            rgba[0] = HalfToFloat(v);
            break;
        }
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        {
            Uint packed;
            std::memcpy(&packed, src, 4);
            rgba[0] = (packed         & 0x3FFu) / 1023.f;
            rgba[1] = ((packed >> 10) & 0x3FFu) / 1023.f;
            rgba[2] = ((packed >> 20) & 0x3FFu) / 1023.f;
            rgba[3] = ((packed >> 30) & 0x3u)   / 3.f;
            break;
        }
        case DXGI_FORMAT_R10G10B10A2_UINT:
        {
            Uint packed;
            std::memcpy(&packed, src, 4);
            rgba[0] = (Float)(packed         & 0x3FFu);
            rgba[1] = (Float)((packed >> 10) & 0x3FFu);
            rgba[2] = (Float)((packed >> 20) & 0x3FFu);
            rgba[3] = (Float)((packed >> 30) & 0x3u);
            break;
        }
        case DXGI_FORMAT_R11G11B10_FLOAT:
        {
            Uint packed;
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

inline void PackUAVUint(DXGI_FORMAT fmt, const Uint values[4], Uint8 out[16])
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
            Uint16 v[4]={ (Uint16)values[0], (Uint16)values[1], (Uint16)values[2], (Uint16)values[3] };
            std::memcpy(out, v, 8); break;
        }
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT: 
        {
            Uint16 v[2]={ (Uint16)values[0], (Uint16)values[1] };
            std::memcpy(out, v, 4); break;
        }
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT: 
        {
            Uint16 v = (Uint16)values[0];
            std::memcpy(out, &v, 2); break;
        }
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
			out[0] = (Uint8)values[0]; out[1] = (Uint8)values[1];
			out[2] = (Uint8)values[2]; out[3] = (Uint8)values[3]; break;
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SINT:
            out[0] = (Uint8)values[0]; out[1]=(Uint8)values[1]; break;
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
            out[0]=(Uint8)values[0]; break;
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
static void SetSlots(T* (&slots)[N], Uint start, Uint count, T* const* values)
{
    for (Uint i = 0; i < count; i++)
    {
        SetSlot(slots[start + i], values ? values[i] : nullptr);
    }
}

template<typename T, Usize N>
static void GetSlots(T* (&slots)[N], Uint start, Uint count, T** out)
{
    if (!out)
    {
        return;
    }

    for (Uint i = 0; i < count; i++)
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

static D3D11_USAGE GetSWResourceUsage(ID3D11Resource* pResource)
{
    D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
    RunOnSWResource(pResource, [&](auto* res) { usage = res->GetUsage(); });
    return usage;
}

static UINT GetSWResourceCPUAccessFlags(ID3D11Resource* pResource)
{
    UINT flags = 0;
    RunOnSWResource(pResource, [&](auto* res) { flags = res->GetCPUAccessFlags(); });
    return flags;
}

static UINT GetSWResourceBindFlags(ID3D11Resource* pResource)
{
    UINT flags = 0;
    RunOnSWResource(pResource, [&](auto* res) { flags = res->GetBindFlags(); });
    return flags;
}

inline void CopySubresourceData(
    Uint8* dstBase, Uint dstRowPitch, Uint dstDepthPitch,
    const Uint8* srcBase, Uint srcRowPitch, Uint srcDepthPitch,
    const D3D11SW_SUBRESOURCE_LAYOUT& layout, const D3D11_BOX* box)
{
    if (!box)
    {
        for (Uint z = 0; z < layout.NumSlices; ++z)
        {
            for (Uint y = 0; y < layout.NumRows; ++y)
            {
                std::memcpy(
                    dstBase + (Uint64)z * dstDepthPitch + (Uint64)y * dstRowPitch,
                    srcBase + (Uint64)z * srcDepthPitch + (Uint64)y * srcRowPitch,
                    layout.RowPitch);
            }
        }
    }
    else
    {
        Uint bx0         = box->left  / layout.BlockSize;
        Uint by0         = box->top   / layout.BlockSize;
        Uint bz0         = box->front;
        Uint copyBlocksX = (box->right  - box->left)  / layout.BlockSize;
        Uint copyBlocksY = (box->bottom - box->top)   / layout.BlockSize;
        Uint copySlices  =  box->back   - box->front;
        Uint copyBytes   = copyBlocksX * layout.PixelStride;
        for (Uint z = 0; z < copySlices; ++z)
        {
            for (Uint y = 0; y < copyBlocksY; ++y)
            {
                std::memcpy(
                    dstBase + (Uint64)(bz0 + z) * dstDepthPitch + (Uint64)(by0 + y) * dstRowPitch + bx0 * layout.PixelStride,
                    srcBase + (Uint64)z          * srcDepthPitch + (Uint64)y          * srcRowPitch,
                    copyBytes);
            }
        }
    }
}

}

