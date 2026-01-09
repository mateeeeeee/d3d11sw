#pragma once
#include "common/macros.h"
#include <d3d11_4.h>
#include <algorithm>

namespace d3d11sw {

inline Uint8 ApplyLogicOp(D3D11_LOGIC_OP op, Uint8 src, Uint8 dst)
{
    switch (op)
    {
        case D3D11_LOGIC_OP_CLEAR:         return 0;
        case D3D11_LOGIC_OP_SET:           return 0xFF;
        case D3D11_LOGIC_OP_COPY:          return src;
        case D3D11_LOGIC_OP_COPY_INVERTED: return ~src;
        case D3D11_LOGIC_OP_NOOP:          return dst;
        case D3D11_LOGIC_OP_INVERT:        return ~dst;
        case D3D11_LOGIC_OP_AND:           return src & dst;
        case D3D11_LOGIC_OP_NAND:          return ~(src & dst);
        case D3D11_LOGIC_OP_OR:            return src | dst;
        case D3D11_LOGIC_OP_NOR:           return ~(src | dst);
        case D3D11_LOGIC_OP_XOR:           return src ^ dst;
        case D3D11_LOGIC_OP_EQUIV:         return ~(src ^ dst);
        case D3D11_LOGIC_OP_AND_REVERSE:   return src & ~dst;
        case D3D11_LOGIC_OP_AND_INVERTED:  return ~src & dst;
        case D3D11_LOGIC_OP_OR_REVERSE:    return src | ~dst;
        case D3D11_LOGIC_OP_OR_INVERTED:   return ~src | dst;
        default:                           return src;
    }
}

inline Float ComputeBlendFactor(D3D11_BLEND factor, const Float src[4], const Float dst[4],
                                const Float blendFactor[4], Int comp, const Float src1[4])
{
    switch (factor)
    {
        case D3D11_BLEND_ZERO:           return 0.f;
        case D3D11_BLEND_ONE:            return 1.f;
        case D3D11_BLEND_SRC_COLOR:      return src[comp];
        case D3D11_BLEND_INV_SRC_COLOR:  return 1.f - src[comp];
        case D3D11_BLEND_SRC_ALPHA:      return src[3];
        case D3D11_BLEND_INV_SRC_ALPHA:  return 1.f - src[3];
        case D3D11_BLEND_DEST_ALPHA:     return dst[3];
        case D3D11_BLEND_INV_DEST_ALPHA: return 1.f - dst[3];
        case D3D11_BLEND_DEST_COLOR:     return dst[comp];
        case D3D11_BLEND_INV_DEST_COLOR: return 1.f - dst[comp];
        case D3D11_BLEND_BLEND_FACTOR:     return blendFactor[comp];
        case D3D11_BLEND_INV_BLEND_FACTOR: return 1.f - blendFactor[comp];
        case D3D11_BLEND_SRC_ALPHA_SAT:
        {
            Float f = std::min(src[3], 1.f - dst[3]);
            return comp == 3 ? 1.f : f;
        }
        case D3D11_BLEND_SRC1_COLOR:      return src1[comp];
        case D3D11_BLEND_INV_SRC1_COLOR:  return 1.f - src1[comp];
        case D3D11_BLEND_SRC1_ALPHA:      return src1[3];
        case D3D11_BLEND_INV_SRC1_ALPHA:  return 1.f - src1[3];
        default: return 1.f;
    }
}

inline Float ComputeBlendOp(D3D11_BLEND_OP op, Float srcTerm, Float dstTerm)
{
    switch (op)
    {
        case D3D11_BLEND_OP_ADD:          return srcTerm + dstTerm;
        case D3D11_BLEND_OP_SUBTRACT:     return srcTerm - dstTerm;
        case D3D11_BLEND_OP_REV_SUBTRACT: return dstTerm - srcTerm;
        case D3D11_BLEND_OP_MIN:          return std::min(srcTerm, dstTerm);
        case D3D11_BLEND_OP_MAX:          return std::max(srcTerm, dstTerm);
        default:                          return srcTerm + dstTerm;
    }
}

}
