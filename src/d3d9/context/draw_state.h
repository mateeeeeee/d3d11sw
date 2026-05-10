#pragma once
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"
#include "core/shaders/parsed_shader.h"

namespace d3dsw {

class D3D9VertexBufferSW;
class D3D9IndexBufferSW;
class D3D9VertexDeclarationSW;
class D3D9VertexShaderSW;
class D3D9PixelShaderSW;
class D3D9SurfaceSW;
struct SWPipelineStatistics;

constexpr UINT SW_D3D9_MAX_STREAMS = 16;
constexpr UINT SW_D3D9_MAX_RTS     = 4;

struct D3D9SW_DRAW_STATE
{
    struct Stream
    {
        D3D9VertexBufferSW* buffer = nullptr;
        UINT                offset = 0;
        UINT                stride = 0;
    };

    Stream                   streams[SW_D3D9_MAX_STREAMS];
    UINT                     streamFreq[SW_D3D9_MAX_STREAMS] = {};
    D3D9IndexBufferSW*       indices       = nullptr;
    D3D9VertexDeclarationSW* vertexDecl    = nullptr;
    DWORD                    fvf           = 0;
    D3DPRIMITIVETYPE         primitiveType = D3DPT_TRIANGLELIST;

    D3D9VertexShaderSW*      vs            = nullptr;
    D3D9PixelShaderSW*       ps            = nullptr;
    const Float*             vsConstF      = nullptr;   
    const Float*             psConstF      = nullptr;   

    void*                    ffVsFn        = nullptr;
    const D3DSW_ParsedShader* ffVsRefl     = nullptr;
    void*                    ffPsFn        = nullptr;
    const D3DSW_ParsedShader* ffPsRefl     = nullptr;

    static constexpr UINT    SW_D3D9_MAX_TEX = 8;
    IDirect3DBaseTexture9*   textures[SW_D3D9_MAX_TEX]          = {};
    DWORD                    samplerStates[SW_D3D9_MAX_TEX][14]  = {};

    D3D9SurfaceSW*           renderTargets[SW_D3D9_MAX_RTS] = {};
    UINT                     numRenderTargets = 0;   // max-populated-slot + 1
    D3D9SurfaceSW*           depthStencil  = nullptr;

    D3DVIEWPORT9             viewport      = {};
    RECT                     scissor       = {};
    BOOL                     scissorEnabled = FALSE;
    const DWORD*             rs            = nullptr;

    SWPipelineStatistics*    stats         = nullptr;
};

}
