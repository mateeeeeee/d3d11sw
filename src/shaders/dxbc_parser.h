#pragma once
#include "shaders/dxbc.h"
#include "shaders/sm4.h"
#include <vector>
#include <string>

namespace d3d11sw {

enum class D3D11SW_ShaderType : Uint8
{
    Vertex   = 0,
    Pixel    = 1,
    Geometry = 2,
    Hull     = 3,
    Domain   = 4,
    Compute  = 5,
    Unknown  = 0xFF,
};

struct D3D11SW_ShaderSignatureElement
{
    Char   name[64];
    Uint32 semanticIndex;
    Uint32 reg;     //shader register index
    Uint8  mask;    //component mask bits 0-3 = xyzw
    Uint32 svType;  //system-value type (0 = user semantic)
};

struct D3D11SW_CBufBinding
{
    Uint32 slot;
    Uint32 sizeVec4s;
};

struct D3D11SW_TexBinding
{
    Uint32 slot;
    Uint32 dimension;
};

struct D3D11SW_TGSMDecl
{
    Uint32 slot;
    Uint32 size;
    Uint32 stride;
};

struct D3D11SW_IndexableTempDecl
{
    Uint32 regIndex;
    Uint32 numRegs;
    Uint32 numComps;
};

struct D3D11SW_HSPhase
{
    std::vector<SM4Instruction>        instrs;
    Uint32                             numTemps = 0;
    Uint32                             instanceCount = 1;
    std::vector<D3D11SW_IndexableTempDecl> indexableTemps;
};

struct D3D11SW_ParsedShader
{
    D3D11SW_ShaderType                           type;
    std::vector<D3D11SW_ShaderSignatureElement>  inputs;
    std::vector<D3D11SW_ShaderSignatureElement>  outputs;
    std::vector<D3D11SW_CBufBinding>             cbufs;
    std::vector<D3D11SW_TexBinding>              textures;
    std::vector<SM4Instruction>                  instrs;
    Uint32                                       numTemps;
    Uint32                                       threadGroupX;
    Uint32                                       threadGroupY;
    Uint32                                       threadGroupZ;
    std::vector<D3D11SW_TGSMDecl>                tgsm;
    std::vector<D3D11SW_IndexableTempDecl>       indexableTemps;
    Bool                                         usesDiscard    = false;
    Bool                                         writesSVDepth  = false;
    Bool                                         usesUAVs       = false;
    Bool                                         needsQuad      = false;
    Uint32                                       numClipDistances = 0;
    Uint32                                       numCullDistances = 0;
    Uint32                                       gsInputPrimitive   = 0;
    Uint32                                       gsOutputTopology   = 0;
    Uint32                                       gsMaxOutputVertices = 0;
    Uint32                                       gsInstanceCount    = 1;

    std::vector<D3D11SW_ShaderSignatureElement>  patchConstants;
    Uint32                                       hsInputControlPointCount  = 0;
    Uint32                                       hsOutputControlPointCount = 0;
    Uint32                                       hsDomain          = 0;
    Uint32                                       hsPartitioning    = 0;
    Uint32                                       hsOutputPrimitive = 0;
    Float                                        hsMaxTessFactor   = 0.f;
    D3D11SW_HSPhase                              hsControlPointPhase;
    std::vector<D3D11SW_HSPhase>                 hsForkPhases;
    std::vector<D3D11SW_HSPhase>                 hsJoinPhases;
};

D3D11SW_API Bool DXBCParse(const void* bytecode, Usize len, D3D11SW_ParsedShader& out);
D3D11SW_API Bool DXBCParseReflection(const void* bytecode, Usize len, D3D11SW_ParsedShader& out);

}
