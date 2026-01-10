#pragma once
#include "context/depth_stencil_util.h"
#include "context/pipeline_state.h"
#include "shaders/shader_abi.h"
#include <d3d11_4.h>

namespace d3d11sw {

struct RTInfo
{
    Uint8*                          data;
    DXGI_FORMAT                     fmt;
    Uint                            rowPitch;
    Uint                            pixStride;
    D3D11_RENDER_TARGET_BLEND_DESC1 blendDesc;
};

struct OMState
{
    Bool depthEnabled;
    D3D11_COMPARISON_FUNC depthFunc;
    D3D11_DEPTH_WRITE_MASK depthWriteMask;
    Uint8* dsvData;
    DXGI_FORMAT dsvFmt;
    Uint dsvRowPitch;
    Uint dsvPixStride;

    Bool stencilEnabled;
    Uint8 stencilReadMask;
    Uint8 stencilWriteMask;
    Uint8 stencilRef;
    D3D11_DEPTH_STENCILOP_DESC stencilFront;
    D3D11_DEPTH_STENCILOP_DESC stencilBack;

    RTInfo rtInfos[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    Uint activeRTCount;
    const Float* blendFactor;
};

OMState InitOM(D3D11SW_PIPELINE_STATE& state);
Bool TestDepth(const OMState& om, Int px, Int py, Float depth);
void WriteDepth(OMState& om, Int px, Int py, Float depth);
void WritePixel(OMState& om, Int px, Int py, const SW_PSOutput& psOut);
void BlendAndWrite(const OMState& om, Int px, Int py, Uint rtIdx,
                   const SW_float4& color, const SW_float4& src1Color);
UINT8 ReadStencil(const OMState& om, Int px, Int py);
void WriteStencil(OMState& om, Int px, Int py, Uint8 val);

}
