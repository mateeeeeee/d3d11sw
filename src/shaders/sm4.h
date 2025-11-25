#pragma once
#include "common/common.h"
#include <d3d11TokenizedProgramFormat.hpp>
#include <vector>

namespace d3d11sw {

using SM4OpCode      = D3D10_SB_OPCODE_TYPE;
using SM4OperandType = D3D10_SB_OPERAND_TYPE;
using SM4CompMode    = D3D10_SB_OPERAND_4_COMPONENT_SELECTION_MODE;

struct SM4Operand
{
    SM4OperandType type         = D3D10_SB_OPERAND_TYPE_NULL;
    SM4CompMode    compMode     = D3D10_SB_OPERAND_4_COMPONENT_SWIZZLE_MODE;
    Uint8          writeMask    = 0xF;
    Uint8          swizzle[4]   = {0,1,2,3};
    Uint8          indexDim     = 0;
    Uint32         indices[2]   = {};
    Float          imm[4]       = {};
    Bool           negate       = false;
    Bool           absolute     = false;
    Bool           intContext   = false;
};

struct SM4Instruction
{
    SM4OpCode               op            = D3D10_SB_OPCODE_NOP;
    Bool                    saturate      = false;
    Bool                    testNonZero   = true;
    Uint32                  threadGroupSize[3] = {1, 1, 1};
    Uint32                  tgsmSlot      = 0;
    Uint32                  tgsmSize      = 0;
    Uint32                  tgsmStride    = 0;
    std::vector<SM4Operand> operands;
};

}
