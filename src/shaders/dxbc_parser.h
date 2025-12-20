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
    Bool                                         usesDiscard    = false;
    Bool                                         writesSVDepth  = false;
    Bool                                         usesUAVs       = false;
    Uint32                                       numClipDistances = 0;
};

D3D11SW_TODO(no state, should it be free functions?);
class D3D11SW_API DXBCParser
{
public:
    Bool Parse(const void* bytecode, Usize len, D3D11SW_ParsedShader& out);
    Bool ParseReflection(const void* bytecode, Usize len, D3D11SW_ParsedShader& out);

private:
    Bool ParseSignatureChunk(const Uint8* data, Usize size,
                             std::vector<D3D11SW_ShaderSignatureElement>& out);
    Bool ParseRdefChunk(const Uint8* data, Usize size, D3D11SW_ParsedShader& out);
    Bool ParseShaderChunk(const Uint8* data, Usize size, D3D11SW_ParsedShader& out);

    D3D11SW_ShaderType ShaderTypeFromRdef(Uint16 shaderType);
};

} 
