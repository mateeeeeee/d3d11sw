#pragma once
#include <vector>
#include "core/shaders/sm4.h"

namespace d3dsw {

enum class D3DSW_ShaderType : Uint8
{
    Vertex   = 0,
    Pixel    = 1,
    Geometry = 2,
    Hull     = 3,
    Domain   = 4,
    Compute  = 5,
    Unknown  = 0xFF,
};

struct D3DSW_ShaderSignatureElement
{
    Char   name[64];
    Uint32 semanticIndex;
    Uint32 reg;     //shader register index
    Uint8  mask;    //component mask bits 0-3 = xyzw
    Uint32 svType;  //system-value type (0 = user semantic)
};

struct D3DSW_CBufBinding
{
    Uint32 slot;
    Uint32 sizeVec4s;
};

struct D3DSW_TexBinding
{
    Uint32 slot;
    Uint32 dimension;
};

struct D3DSW_TGSMDecl
{
    Uint32 slot;
    Uint32 size;
    Uint32 stride;
};

struct D3DSW_IndexableTempDecl
{
    Uint32 regIndex;
    Uint32 numRegs;
    Uint32 numComps;
};

struct D3DSW_HSPhase
{
    std::vector<SM4Instruction>          instrs;
    Uint32                               numTemps      = 0;
    Uint32                               instanceCount = 1;
    std::vector<D3DSW_IndexableTempDecl> indexableTemps;
};

// Parsed shader IR consumed by codegen + JIT. Populated by a frontend parse
// callback (D3D11 = DXBCParse, D3D9 = SM3Parse)
struct D3DSW_ParsedShader
{
    D3DSW_ShaderType                           type;
    std::vector<D3DSW_ShaderSignatureElement>  inputs;
    std::vector<D3DSW_ShaderSignatureElement>  outputs;
    std::vector<D3DSW_CBufBinding>             cbufs;
    std::vector<D3DSW_TexBinding>              textures;
    std::vector<SM4Instruction>                instrs;
    Uint32                                     numTemps         = 0;
    std::vector<D3DSW_IndexableTempDecl>       indexableTemps;
    Bool                                       usesDiscard      = false;
    Bool                                       writesSVDepth    = false;
    Bool                                       usesUAVs         = false;
    Bool                                       usesSampleIndex  = false;
    Bool                                       needsQuad        = false;
    Uint32                                     numClipDistances = 0;
    Uint32                                     numCullDistances = 0;

    Uint32                                     csThreadGroupX   = 1;
    Uint32                                     csThreadGroupY   = 1;
    Uint32                                     csThreadGroupZ   = 1;
    std::vector<D3DSW_TGSMDecl>                csTgsm;

    Uint32                                     gsInputPrimitive    = 0;
    Uint32                                     gsOutputTopology    = 0;
    Uint32                                     gsMaxOutputVertices = 0;
    Uint32                                     gsInstanceCount     = 1;

    std::vector<D3DSW_ShaderSignatureElement>  patchConstants;
    Uint32                                     hsInputControlPointCount  = 0;
    Uint32                                     hsOutputControlPointCount = 0;
    Uint32                                     hsDomain          = 0;
    Uint32                                     hsPartitioning    = 0;
    Uint32                                     hsOutputPrimitive = 0;
    Float                                      hsMaxTessFactor   = 0.f;
    D3DSW_HSPhase                              hsControlPointPhase;
    std::vector<D3DSW_HSPhase>                 hsForkPhases;
    std::vector<D3DSW_HSPhase>                 hsJoinPhases;
};

}
