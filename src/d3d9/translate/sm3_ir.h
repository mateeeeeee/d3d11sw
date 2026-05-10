#pragma once
#include <vector>
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"

namespace d3dsw {

enum class SM3OperandKind : Uint8
{
    Destination,   //uses writeMask / dstMod / dstShift
    Source,        //uses swizzle / srcMod
};

struct SM3Operand
{
    D3DSHADER_PARAM_REGISTER_TYPE   regType   = D3DSPR_TEMP;
    Uint32                          regNum    = 0;
    SM3OperandKind                  kind      = SM3OperandKind::Source;
    Bool                            relative  = false;

    D3DSHADER_PARAM_REGISTER_TYPE   relRegType = D3DSPR_ADDR;
    Uint32                          relRegNum  = 0;
    Uint8                           relSwizzle = 0;  

    Uint8                           writeMask = 0xF;  //4 bits (.xyzw)
    D3DSHADER_PARAM_DSTMOD_TYPE     dstMod    = D3DSPDM_NONE;
    Int8                            dstShift  = 0;    //4-bit two's complement scale

    Uint8                           swizzle[4] = {0, 1, 2, 3};
    D3DSHADER_PARAM_SRCMOD_TYPE     srcMod    = D3DSPSM_NONE;
};

struct SM3Instruction
{
    D3DSHADER_INSTRUCTION_OPCODE_TYPE op          = D3DSIO_NOP;
    Uint8                             controlBits = 0;     //opcode-specific (bits 16-23 of header; e.g. IFC compare, texld flags)
    Bool                              predicated  = false;
    Bool                              coissue     = false;
    std::vector<SM3Operand>           operands;            //[dst, src0, src1, ...] per opcode shape

    Uint8                             declUsage       = 0;  //D3DDECLUSAGE_*
    Uint8                             declUsageIndex  = 0;
    D3DSAMPLER_TEXTURE_TYPE           declSamplerType = D3DSTT_UNKNOWN;

    //DEF / DEFI / DEFB — immediate payload (interpret per opcode)
    Float                             defFloat[4] = {};
    Int32                             defInt[4]   = {};
    Uint32                            defBool     = 0;
};

struct SM3DecodedShader
{
    Uint8                       majorVersion   = 0;
    Uint8                       minorVersion   = 0;
    Bool                        isPixelShader  = false;
    std::vector<SM3Instruction> instructions;
};

}
