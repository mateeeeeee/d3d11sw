#pragma once
#include "common/macros.h"
#include <d3d11_4.h>
#include <cstring>
#include <algorithm>
#include <vector>

namespace d3d11sw {

struct HiZBuffer
{
    std::vector<Float> data;
    Uint8* dsv = nullptr;
    Int tilesX = 0;
    Int tilesY = 0;
    Int width = 0;
    Int height = 0;
    Int tileSize = 0;

    void Clear(Uint8* dsvData, Int w, Int h, Int tile, Float clearDepth)
    {
        dsv = dsvData;
        width = w;
        height = h;
        tileSize = tile;
        tilesX = (w + tile - 1) / tile;
        tilesY = (h + tile - 1) / tile;
        data.assign(static_cast<Usize>(tilesX) * tilesY, clearDepth);
    }

    Bool Matches(Uint8* dsvData) const { return dsv == dsvData && !data.empty(); }

    Float Read(Int tileX, Int tileY) const
    {
        Int gCol = tileX / tileSize;
        Int gRow = tileY / tileSize;
        return data[static_cast<Usize>(gRow) * tilesX + gCol];
    }

    void Write(Int tileX, Int tileY, Float val)
    {
        Int gCol = tileX / tileSize;
        Int gRow = tileY / tileSize;
        data[static_cast<Usize>(gRow) * tilesX + gCol] = val;
    }
};

inline UINT DepthPixelStride(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D32_FLOAT:            return 4;
        case DXGI_FORMAT_D16_UNORM:            return 2;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:    return 4;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return 8;
        default:                               return 4;
    }
}

inline Bool FormatHasStencil(DXGI_FORMAT fmt)
{
    return fmt == DXGI_FORMAT_D24_UNORM_S8_UINT ||
           fmt == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
}

inline Float ReadDepthValue(DXGI_FORMAT fmt, const Uint8* src)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D32_FLOAT:
        {
            Float d;
            std::memcpy(&d, src, 4);
            return d;
        }
        case DXGI_FORMAT_D16_UNORM:
        {
            Uint16 u;
            std::memcpy(&u, src, 2);
            return u / 65535.f;
        }
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            Uint32 u;
            std::memcpy(&u, src, 4);
            return (u & 0x00FFFFFF) / 16777215.f;
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        {
            Float d;
            std::memcpy(&d, src, 4);
            return d;
        }
        default:
            return 1.f;
    }
}

inline void WriteDepthValue(DXGI_FORMAT fmt, Uint8* dst, Float depth)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D32_FLOAT:
            std::memcpy(dst, &depth, 4);
            break;
        case DXGI_FORMAT_D16_UNORM:
        {
            Uint16 u = static_cast<Uint16>(std::clamp(depth, 0.f, 1.f) * 65535.f + 0.5f);
            std::memcpy(dst, &u, 2);
            break;
        }
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            Uint32 existing;
            std::memcpy(&existing, dst, 4);
            Uint32 d24 = static_cast<Uint32>(std::clamp(depth, 0.f, 1.f) * 16777215.f + 0.5f);
            existing = (existing & 0xFF000000) | (d24 & 0x00FFFFFF);
            std::memcpy(dst, &existing, 4);
            break;
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            std::memcpy(dst, &depth, 4);
            break;
        default:
            break;
    }
}

inline Bool CompareDepth(D3D11_COMPARISON_FUNC func, Float src, Float dst)
{
    switch (func)
    {
        case D3D11_COMPARISON_NEVER:         return false;
        case D3D11_COMPARISON_LESS:          return src < dst;
        case D3D11_COMPARISON_EQUAL:         return src == dst;
        case D3D11_COMPARISON_LESS_EQUAL:    return src <= dst;
        case D3D11_COMPARISON_GREATER:       return src > dst;
        case D3D11_COMPARISON_NOT_EQUAL:     return src != dst;
        case D3D11_COMPARISON_GREATER_EQUAL: return src >= dst;
        case D3D11_COMPARISON_ALWAYS:        return true;
        default:                             return true;
    }
}

inline Uint8 ReadStencilValue(DXGI_FORMAT fmt, const Uint8* src)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            Uint32 u;
            std::memcpy(&u, src, 4);
            return static_cast<Uint8>(u >> 24);
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return src[4];
        default:
            return 0;
    }
}

inline void WriteStencilValue(DXGI_FORMAT fmt, Uint8* dst, Uint8 val)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            Uint32 u;
            std::memcpy(&u, dst, 4);
            u = (u & 0x00FFFFFFu) | (static_cast<Uint32>(val) << 24);
            std::memcpy(dst, &u, 4);
            break;
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            dst[4] = val;
            break;
        default:
            break;
    }
}

inline Bool CompareStencil(D3D11_COMPARISON_FUNC func, Uint8 ref, Uint8 val)
{
    switch (func)
    {
        case D3D11_COMPARISON_NEVER:         return false;
        case D3D11_COMPARISON_LESS:          return ref < val;
        case D3D11_COMPARISON_EQUAL:         return ref == val;
        case D3D11_COMPARISON_LESS_EQUAL:    return ref <= val;
        case D3D11_COMPARISON_GREATER:       return ref > val;
        case D3D11_COMPARISON_NOT_EQUAL:     return ref != val;
        case D3D11_COMPARISON_GREATER_EQUAL: return ref >= val;
        case D3D11_COMPARISON_ALWAYS:        return true;
        default:                             return true;
    }
}

inline Uint8 ApplyStencilOp(D3D11_STENCIL_OP op, Uint8 curVal, Uint8 ref)
{
    switch (op)
    {
        case D3D11_STENCIL_OP_KEEP:     return curVal;
        case D3D11_STENCIL_OP_ZERO:     return 0;
        case D3D11_STENCIL_OP_REPLACE:  return ref;
        case D3D11_STENCIL_OP_INCR_SAT: return curVal < 255 ? curVal + 1 : 255;
        case D3D11_STENCIL_OP_DECR_SAT: return curVal > 0 ? curVal - 1 : 0;
        case D3D11_STENCIL_OP_INVERT:   return ~curVal;
        case D3D11_STENCIL_OP_INCR:     return static_cast<Uint8>(curVal + 1);
        case D3D11_STENCIL_OP_DECR:     return static_cast<Uint8>(curVal - 1);
        default:                        return curVal;
    }
}

}
