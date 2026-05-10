#pragma once
#include <cmath>
#include "core/pipeline/sw_pipeline_types.h"

namespace d3dsw {

inline Uint8 ApplyLogicOp(SWLogicOp op, Uint8 src, Uint8 dst)
{
    switch (op)
    {
        case SWLogicOp::Clear:        return 0;
        case SWLogicOp::Set:          return 0xFF;
        case SWLogicOp::Copy:         return src;
        case SWLogicOp::CopyInverted: return ~src;
        case SWLogicOp::Noop:         return dst;
        case SWLogicOp::Invert:       return ~dst;
        case SWLogicOp::And:          return src & dst;
        case SWLogicOp::Nand:         return ~(src & dst);
        case SWLogicOp::Or:           return src | dst;
        case SWLogicOp::Nor:          return ~(src | dst);
        case SWLogicOp::Xor:          return src ^ dst;
        case SWLogicOp::Equiv:        return ~(src ^ dst);
        case SWLogicOp::AndReverse:   return src & ~dst;
        case SWLogicOp::AndInverted:  return ~src & dst;
        case SWLogicOp::OrReverse:    return src | ~dst;
        case SWLogicOp::OrInverted:   return ~src | dst;
        default:                      return src;
    }
}

inline Float ComputeBlendFactor(SWBlend factor, const Float src[4], const Float dst[4],
                                const Float blendFactor[4], Int comp, const Float src1[4])
{
    switch (factor)
    {
        case SWBlend::Zero:           return 0.f;
        case SWBlend::One:            return 1.f;
        case SWBlend::SrcColor:       return src[comp];
        case SWBlend::InvSrcColor:    return 1.f - src[comp];
        case SWBlend::SrcAlpha:       return src[3];
        case SWBlend::InvSrcAlpha:    return 1.f - src[3];
        case SWBlend::DestAlpha:      return dst[3];
        case SWBlend::InvDestAlpha:   return 1.f - dst[3];
        case SWBlend::DestColor:      return dst[comp];
        case SWBlend::InvDestColor:   return 1.f - dst[comp];
        case SWBlend::BlendFactor:    return blendFactor[comp];
        case SWBlend::InvBlendFactor: return 1.f - blendFactor[comp];
        case SWBlend::SrcAlphaSat:
        {
            Float f = std::fmin(src[3], 1.f - dst[3]);
            return comp == 3 ? 1.f : f;
        }
        case SWBlend::Src1Color:      return src1[comp];
        case SWBlend::InvSrc1Color:   return 1.f - src1[comp];
        case SWBlend::Src1Alpha:      return src1[3];
        case SWBlend::InvSrc1Alpha:   return 1.f - src1[3];
        default: return 1.f;
    }
}

inline Float ComputeBlendOp(SWBlendOp op, Float srcTerm, Float dstTerm)
{
    switch (op)
    {
        case SWBlendOp::Add:         return srcTerm + dstTerm;
        case SWBlendOp::Subtract:    return srcTerm - dstTerm;
        case SWBlendOp::RevSubtract: return dstTerm - srcTerm;
        case SWBlendOp::Min:         return std::fmin(srcTerm, dstTerm);
        case SWBlendOp::Max:         return std::fmax(srcTerm, dstTerm);
        default:                     return srcTerm + dstTerm;
    }
}

}
