#pragma once
#include <dxgiformat.h>
#include <algorithm>
#include <bit>
#include <cstring>
#include "core/common/macros.h"
#include "core/pipeline/sw_pipeline_types.h"

namespace d3dsw {

inline Uint DepthPixelStride(DXGI_FORMAT fmt)
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

inline Bool CompareDepth(SWComparison func, Float src, Float dst)
{
    switch (func)
    {
        case SWComparison::Never:        return false;
        case SWComparison::Less:         return src < dst;
        case SWComparison::Equal:        return src == dst;
        case SWComparison::LessEqual:    return src <= dst;
        case SWComparison::Greater:      return src > dst;
        case SWComparison::NotEqual:     return src != dst;
        case SWComparison::GreaterEqual: return src >= dst;
        case SWComparison::Always:       return true;
        default:                         return true;
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

inline Bool CompareStencil(SWComparison func, Uint8 ref, Uint8 val)
{
    switch (func)
    {
        case SWComparison::Never:        return false;
        case SWComparison::Less:         return ref < val;
        case SWComparison::Equal:        return ref == val;
        case SWComparison::LessEqual:    return ref <= val;
        case SWComparison::Greater:      return ref > val;
        case SWComparison::NotEqual:     return ref != val;
        case SWComparison::GreaterEqual: return ref >= val;
        case SWComparison::Always:       return true;
        default:                         return true;
    }
}

inline Uint8 ApplyStencilOp(SWStencilOp op, Uint8 curVal, Uint8 ref)
{
    switch (op)
    {
        case SWStencilOp::Keep:    return curVal;
        case SWStencilOp::Zero:    return 0;
        case SWStencilOp::Replace: return ref;
        case SWStencilOp::IncrSat: return curVal < 255 ? curVal + 1 : 255;
        case SWStencilOp::DecrSat: return curVal > 0 ? curVal - 1 : 0;
        case SWStencilOp::Invert:  return ~curVal;
        case SWStencilOp::Incr:    return static_cast<Uint8>(curVal + 1);
        case SWStencilOp::Decr:    return static_cast<Uint8>(curVal - 1);
        default:                   return curVal;
    }
}

inline Uint32 ClearDepthBits(DXGI_FORMAT fmt, Uint32 pix, Float depth)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D32_FLOAT:
        {
            Uint32 bits = std::bit_cast<Uint32>(depth);
            return bits;
        }
        case DXGI_FORMAT_D16_UNORM:
        {
            Uint32 u = std::min(static_cast<Uint32>(std::clamp(depth, 0.f, 1.f) * 65535.f + 0.5f), 0xFFFFu);
            return static_cast<Uint32>(u);
        }
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            Uint32 d24 = std::min(static_cast<Uint32>(std::clamp(depth, 0.f, 1.f) * 16777215.f + 0.5f), 0x00FFFFFFu);
            return (pix & 0xFF000000u) | d24;
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        {
            Uint32 bits = std::bit_cast<Uint32>(depth);
            return bits;   
        }
        default:
            return pix;
    }
}

inline Uint32 ClearStencilBits(DXGI_FORMAT fmt, Uint32 pix, Uint8 val)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return (pix & 0x00FFFFFFu) | (static_cast<Uint32>(val) << 24);
        default:
            return pix;
    }
}

}
