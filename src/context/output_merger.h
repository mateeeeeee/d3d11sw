#pragma once
#include "common/common.h"
#include "shaders/shader_abi.h"
#include <algorithm>
#include <cstring>

namespace d3d11sw {

struct D3D11SW_PIPELINE_STATE;

class OutputMerger
{
public:
    OutputMerger(D3D11SW_PIPELINE_STATE& state);

    Bool HasRenderTargets() const { return _activeRTCount > 0; }
    Bool DepthEnabled() const { return _depthEnabled; }
    Bool DepthWriteEnabled() const { return _depthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL; }
    UINT ActiveRTCount() const { return _activeRTCount; }

    Bool TestDepth(Int px, Int py, Float depth) const;
    void WriteDepth(Int px, Int py, Float depth);
    void WritePixel(Int px, Int py, const SW_PSOutput& psOut);

private:
    struct RTInfo
    {
        UINT8*                          data;
        DXGI_FORMAT                     fmt;
        UINT                            rowPitch;
        UINT                            pixStride;
        D3D11_RENDER_TARGET_BLEND_DESC1 blendDesc;
    };

    Bool _depthEnabled;
    D3D11_COMPARISON_FUNC _depthFunc;
    D3D11_DEPTH_WRITE_MASK _depthWriteMask;
    UINT8* _dsvData;
    DXGI_FORMAT _dsvFmt;
    UINT _dsvRowPitch;
    UINT _dsvPixStride;

    RTInfo _rtInfos[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    UINT _activeRTCount;
    const FLOAT* _blendFactor;

private:
    static Float ReadDepthValue(DXGI_FORMAT fmt, const UINT8* src);
    static void WriteDepthValue(DXGI_FORMAT fmt, UINT8* dst, Float depth);
    static UINT DepthPixelStride(DXGI_FORMAT fmt);
    static Bool CompareDepth(D3D11_COMPARISON_FUNC func, Float src, Float dst);
    static void UnpackColor(DXGI_FORMAT fmt, const UINT8* src, FLOAT rgba[4]);
    static Float ComputeBlendFactor(D3D11_BLEND factor, const FLOAT src[4], const FLOAT dst[4],
                                    const FLOAT blendFactor[4], Int comp);
    static Float ComputeBlendOp(D3D11_BLEND_OP op, Float srcTerm, Float dstTerm);

    void BlendAndWrite(Int px, Int py, UINT rtIdx, const SW_float4& color);
};

inline Bool OutputMerger::TestDepth(Int px, Int py, Float depth) const
{
    UINT8* dsvPx = _dsvData + (UINT64)py * _dsvRowPitch + (UINT64)px * _dsvPixStride;
    Float existing = ReadDepthValue(_dsvFmt, dsvPx);
    return CompareDepth(_depthFunc, depth, existing);
}

inline void OutputMerger::WriteDepth(Int px, Int py, Float depth)
{
    UINT8* dsvPx = _dsvData + (UINT64)py * _dsvRowPitch + (UINT64)px * _dsvPixStride;
    WriteDepthValue(_dsvFmt, dsvPx, depth);
}

inline Float OutputMerger::ReadDepthValue(DXGI_FORMAT fmt, const UINT8* src)
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
            UINT16 u;
            std::memcpy(&u, src, 2);
            return u / 65535.f;
        }
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            UINT32 u;
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

inline void OutputMerger::WriteDepthValue(DXGI_FORMAT fmt, UINT8* dst, Float depth)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D32_FLOAT:
            std::memcpy(dst, &depth, 4);
            break;
        case DXGI_FORMAT_D16_UNORM:
        {
            UINT16 u = static_cast<UINT16>(std::clamp(depth, 0.f, 1.f) * 65535.f + 0.5f);
            std::memcpy(dst, &u, 2);
            break;
        }
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            UINT32 existing;
            std::memcpy(&existing, dst, 4);
            UINT32 d24 = static_cast<UINT32>(std::clamp(depth, 0.f, 1.f) * 16777215.f + 0.5f);
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

inline UINT OutputMerger::DepthPixelStride(DXGI_FORMAT fmt)
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

inline Bool OutputMerger::CompareDepth(D3D11_COMPARISON_FUNC func, Float src, Float dst)
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

inline void OutputMerger::UnpackColor(DXGI_FORMAT fmt, const UINT8* src, FLOAT rgba[4])
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
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            std::memcpy(rgba, src, 16);
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
            std::memcpy(rgba, src, 8);
            break;
        case DXGI_FORMAT_R32_FLOAT:
            std::memcpy(rgba, src, 4);
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
        default:
            break;
    }
}

inline Float OutputMerger::ComputeBlendFactor(D3D11_BLEND factor, const FLOAT src[4], const FLOAT dst[4],
                                              const FLOAT blendFactor[4], Int comp)
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
        default: return 1.f;
    }
}

inline Float OutputMerger::ComputeBlendOp(D3D11_BLEND_OP op, Float srcTerm, Float dstTerm)
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
