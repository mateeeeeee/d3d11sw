#pragma once
#include "common/common.h"
#include <d3d11TokenizedProgramFormat.hpp>
#include <vector>

namespace d3d11sw {

struct SM4Operand
{
    D3D10_SB_OPERAND_TYPE type  = D3D10_SB_OPERAND_TYPE_NULL;
    Uint8          writeMask    = 0xF;
    Uint8          swizzle[4]   = {0,1,2,3};
    Uint32         indices[2]   = {};
    Float          imm[4]       = {};
    Bool           negate       = false;
    Bool           absolute     = false;
    Bool           intContext   = false;
    Int            relativeReg  = -1;
    Uint8          relativeComp = 0;
};

struct SM4Instruction
{
    D3D10_SB_OPCODE_TYPE    op            = D3D10_SB_OPCODE_NOP;
    Bool                    saturate      = false;
    Bool                    testNonZero   = true;
    Uint8                   resInfoReturn = 0;
    std::vector<SM4Operand> operands;
};

}
