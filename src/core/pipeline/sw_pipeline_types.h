#pragma once

namespace d3dsw {

enum class SWComparison : Uint8
{
    Never        = 1,
    Less         = 2,
    Equal        = 3,
    LessEqual    = 4,
    Greater      = 5,
    NotEqual     = 6,
    GreaterEqual = 7,
    Always       = 8,
};

enum class SWStencilOp : Uint8
{
    Keep    = 1,
    Zero    = 2,
    Replace = 3,
    IncrSat = 4,
    DecrSat = 5,
    Invert  = 6,
    Incr    = 7,
    Decr    = 8,
};

enum class SWBlend : Uint8
{
    Zero           = 1,
    One            = 2,
    SrcColor       = 3,
    InvSrcColor    = 4,
    SrcAlpha       = 5,
    InvSrcAlpha    = 6,
    DestAlpha      = 7,
    InvDestAlpha   = 8,
    DestColor      = 9,
    InvDestColor   = 10,
    SrcAlphaSat    = 11,
    BlendFactor    = 14,
    InvBlendFactor = 15,
    Src1Color      = 16,
    InvSrc1Color   = 17,
    Src1Alpha      = 18,
    InvSrc1Alpha   = 19,
};

enum class SWBlendOp : Uint8
{
    Add         = 1,
    Subtract    = 2,
    RevSubtract = 3,
    Min         = 4,
    Max         = 5,
};

enum class SWLogicOp : Uint8
{
    Clear        = 0,
    Set          = 1,
    Copy         = 2,
    CopyInverted = 3,
    Noop         = 4,
    Invert       = 5,
    And          = 6,
    Nand         = 7,
    Or           = 8,
    Nor          = 9,
    Xor          = 10,
    Equiv        = 11,
    AndReverse   = 12,
    AndInverted  = 13,
    OrReverse    = 14,
    OrInverted   = 15,
};

enum class SWCullMode : Uint8
{
    None  = 1,
    Front = 2,
    Back  = 3,
};

enum class SWFillMode : Uint8
{
    Wireframe = 2,
    Solid     = 3,
};

enum class SWDepthWriteMask : Uint8
{
    Zero = 0,
    All  = 1,
};

enum class SWTextureAddress : Uint8
{
    Wrap       = 1,
    Mirror     = 2,
    Clamp      = 3,
    Border     = 4,
    MirrorOnce = 5,
};

enum class SWInputSlotClass : Uint8
{
    PerVertexData   = 0,
    PerInstanceData = 1,
};

enum class SWIndexFormat : Uint8
{
    Uint16 = 0,
    Uint32 = 1,
};

enum class SWTopology : Uint16
{
    Undefined        = 0,
    PointList        = 1,
    LineList         = 2,
    LineStrip        = 3,
    TriangleList     = 4,
    TriangleStrip    = 5,
    TriangleFan      = 6,
    LineListAdj      = 10,
    LineStripAdj     = 11,
    TriangleListAdj  = 12,
    TriangleStripAdj = 13,
    // PatchListN = 32 + N for N in 1..32; PatchList1 = 33, PatchList32 = 64.
    PatchList1       = 33,
    PatchList32      = 64,
};

using SWFilter = Uint32;

struct SWViewport
{
    Float topLeftX;
    Float topLeftY;
    Float width;
    Float height;
    Float minDepth;
    Float maxDepth;
};

struct SWRect
{
    Int32 left;
    Int32 top;
    Int32 right;
    Int32 bottom;
};

struct SWStencilOpDesc
{
    SWStencilOp  failOp;
    SWStencilOp  depthFailOp;
    SWStencilOp  passOp;
    SWComparison func;
};

struct SWDepthStencilDesc
{
    Bool             depthEnable;
    SWDepthWriteMask depthWriteMask;
    SWComparison     depthFunc;
    Bool             stencilEnable;
    Uint8            stencilReadMask;
    Uint8            stencilWriteMask;
    SWStencilOpDesc  frontFace;
    SWStencilOpDesc  backFace;
};

struct SWRenderTargetBlendDesc
{
    Bool      blendEnable;
    Bool      logicOpEnable;
    SWBlend   srcBlend;
    SWBlend   destBlend;
    SWBlendOp blendOp;
    SWBlend   srcBlendAlpha;
    SWBlend   destBlendAlpha;
    SWBlendOp blendOpAlpha;
    SWLogicOp logicOp;
    Uint8     renderTargetWriteMask;
};

struct SWBlendDesc
{
    Bool                    alphaToCoverage;
    Bool                    independentBlend;
    SWRenderTargetBlendDesc renderTarget[8];
};

struct SWRasterizerDesc
{
    SWFillMode fillMode;
    SWCullMode cullMode;
    Bool       frontCCW;
    Int32      depthBias;
    Float      depthBiasClamp;
    Float      slopeScaledDepthBias;
    Bool       depthClipEnable;
    Bool       scissorEnable;
    Bool       multisampleEnable;
    Bool       antialiasedLineEnable;
};

struct SWSamplerDesc
{
    SWFilter         filter;
    SWTextureAddress addressU;
    SWTextureAddress addressV;
    SWTextureAddress addressW;
    Float            mipLODBias;
    Uint32           maxAnisotropy;
    SWComparison     comparisonFunc;
    Float            borderColor[4];
    Float            minLOD;
    Float            maxLOD;
};

}
