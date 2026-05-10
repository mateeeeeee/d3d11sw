#pragma once
#include "core/common/common.h"
#include "core/format/subresource_layout.h"
#include "d3d11/resources/buffer.h"
#include "d3d11/resources/texture1d.h"
#include "d3d11/resources/texture2d.h"
#include "d3d11/resources/texture3d.h"
#include <d3d11_4.h>
#include <cstring>

namespace d3dsw {

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
    const D3DSW_SUBRESOURCE_LAYOUT& layout, const D3D11_BOX* box)
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

