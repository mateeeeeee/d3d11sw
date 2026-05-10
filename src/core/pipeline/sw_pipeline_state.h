#pragma once
#include <dxgiformat.h>
#include "core/pipeline/sw_pipeline_types.h"
#include "core/shaders/shader_abi.h"
#include "core/shaders/shader_constants.h"
#include "core/shaders/parsed_shader.h"

namespace d3dsw {

constexpr Uint SW_MAX_VERTEX_BUFFERS   = 32;
constexpr Uint SW_MAX_INPUT_ELEMENTS   = 32;
constexpr Uint SW_MAX_VIEWPORTS        = 16;
constexpr Uint SW_MAX_SCISSOR_RECTS    = 16;
constexpr Uint SW_MAX_RENDER_TARGETS   = 8;
constexpr Uint SW_MAX_SO_TARGETS       = 4;
constexpr Uint SW_MAX_SO_DECLARATIONS  = 64;
constexpr Uint SW_SEMANTIC_NAME_MAX    = 64;
constexpr Uint SW_MAX_HS_FORK_PHASES   = 16;
constexpr Uint SW_MAX_HS_JOIN_PHASES   = 16;

struct SWPipelineStatistics
{
    Uint64 iaVertices                = 0;
    Uint64 iaPrimitives              = 0;
    Uint64 vsInvocations             = 0;
    Uint64 gsInvocations             = 0;
    Uint64 gsPrimitives              = 0;
    Uint64 cInvocations              = 0;
    Uint64 cPrimitives               = 0;
    Uint64 psInvocations             = 0;
    Uint64 hsInvocations             = 0;
    Uint64 dsInvocations             = 0;
    Uint64 csInvocations             = 0;
    Uint64 occlusionCount            = 0;
    Uint64 soNumPrimitivesWritten    = 0;
    Uint64 soPrimitivesStorageNeeded = 0;
};

struct SWInputElement
{
    char             semanticName[SW_SEMANTIC_NAME_MAX];
    Uint32           semanticIndex        = 0;
    DXGI_FORMAT      format               = DXGI_FORMAT_UNKNOWN;
    Uint32           inputSlot            = 0;
    Uint32           alignedByteOffset    = 0;
    SWInputSlotClass inputSlotClass       = SWInputSlotClass::PerVertexData;
    Uint32           instanceDataStepRate = 0;
};

struct SWVertexBufferBinding
{
    const Uint8* data   = nullptr;
    Uint32       stride = 0;
    Uint32       offset = 0;
};

struct SWIndexBufferBinding
{
    const Uint8* data   = nullptr;
    DXGI_FORMAT  format = DXGI_FORMAT_UNKNOWN;
    Uint32       offset = 0;
};

struct SWShader
{
    void*                     fn         = nullptr;
    const D3DSW_ParsedShader* reflection = nullptr;
};

struct SWStreamOutDeclaration
{
    char   semanticName[SW_SEMANTIC_NAME_MAX];
    Uint32 semanticIndex;
    Uint8  stream;
    Uint8  startComponent;
    Uint8  componentCount;
    Uint8  outputSlot;
};

struct SWRenderTargetSnapshot
{
    Uint8*      data          = nullptr;
    DXGI_FORMAT format        = DXGI_FORMAT_UNKNOWN;
    Uint32      rowPitch      = 0;
    Uint32      pixStride     = 0;
    Uint32      slicePitch    = 0;
    Uint32      sampleCount   = 1;
    Uint32      sampleQuality = 0;
    Uint32      arraySize     = 1;
};

struct SWDepthStencilSnapshot
{
    Uint8*      data          = nullptr;
    DXGI_FORMAT format        = DXGI_FORMAT_UNKNOWN;
    Uint32      rowPitch      = 0;
    Uint32      numRows       = 0;
    Uint32      pixStride     = 0;
    Uint32      sampleCount   = 1;
    Uint32      sampleQuality = 0;
};

struct SWStreamOutTarget
{
    Uint8*  data        = nullptr;
    Uint32  sizeBytes   = 0;
    Uint32  writeOffset = 0;
    Uint32  vertexCount = 0;
};

struct SWPipelineState
{
    SWInputElement        inputElements[SW_MAX_INPUT_ELEMENTS];
    Uint32                numInputElements = 0;
    SWIndexBufferBinding  indexBuffer;
    SWVertexBufferBinding vertexBuffers[SW_MAX_VERTEX_BUFFERS];
    SWTopology            topology = SWTopology::Undefined;

    SWShader      vs;
    SW_Resources  vsRes;

    SWShader      ps;
    SW_Resources  psRes;

    SWShader      gs;
    SW_Resources  gsRes;
    Bool                   gsHasSO              = false;
    Uint32                 gsRasterizedStream   = 0;
    SWStreamOutDeclaration gsStreamOutDecls[SW_MAX_SO_DECLARATIONS];
    Uint32                 gsStreamOutDeclCount = 0;
    Uint32                 gsStreamOutStrides[SW_MAX_SO_TARGETS] = {};
    
    SWStreamOutTarget      soTargets[SW_MAX_SO_TARGETS];

    SWShader      hs;
    SW_Resources  hsRes;
    SW_HSForkJoinFn hsForkFns[SW_MAX_HS_FORK_PHASES] = {};
    SW_HSForkJoinFn hsJoinFns[SW_MAX_HS_JOIN_PHASES] = {};

    SWShader      ds;
    SW_Resources  dsRes;

    SWShader      cs;
    SW_Resources  csRes;

    SWRasterizerDesc rasterizerDesc;
    SWViewport       viewports[SW_MAX_VIEWPORTS];
    Uint32           numViewports = 0;
    SWRect           scissorRects[SW_MAX_SCISSOR_RECTS];
    Uint32           numScissorRects = 0;

    SWRenderTargetSnapshot renderTargets[SW_MAX_RENDER_TARGETS];
    Uint32                 numRenderTargets = 0;
    SWDepthStencilSnapshot depthStencilView;
    SWBlendDesc            blendDesc;
    Float                  blendFactor[4] = {};
    Uint32                 sampleMask = 0xFFFFFFFFu;
    SWDepthStencilDesc     depthStencilDesc;
    Uint8                  stencilRef = 0;

    //Alpha test (D3D9 fixed-function)
    Bool         alphaTestEnable = false;
    SWComparison alphaTestFunc   = SWComparison::Always;
    Uint8        alphaRef        = 0;

    // Fog (D3D9 fixed-function and custom VS oFog output).
    enum class SWFogMode : Uint8 { None = 0, Linear = 1, Exp = 2, Exp2 = 3 };
    struct SWFogDesc
    {
        Bool      enable     = false;
        SWFogMode vertexMode = SWFogMode::None;
        SWFogMode tableMode  = SWFogMode::None;
        Float     color[4]   = { 0.f, 0.f, 0.f, 1.f };
        Float     start      = 0.f;
        Float     end        = 1.f;
        Float     density    = 1.f;
    } fogDesc;

    SWPipelineStatistics* stats = nullptr;
};

struct SWDrawCommand
{
    Uint32 vertexCount   = 0;
    Uint32 startVertex   = 0;
    Uint32 indexCount    = 0;
    Uint32 startIndex    = 0;
    Int32  baseVertex    = 0;
    Uint32 instanceCount = 1;
    Uint32 startInstance = 0;
    Bool   indexed       = false;
};

}
