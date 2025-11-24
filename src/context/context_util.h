#pragma once
#include "common/common.h"
#include "resources/subresource_layout.h"
#include "resources/buffer.h"
#include "resources/texture1d.h"
#include "resources/texture2d.h"
#include "resources/texture3d.h"

namespace d3d11sw {

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
        default: break;
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

