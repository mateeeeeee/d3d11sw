#pragma once
#include "shaders/dxbc_parser.h"
#include <string>

namespace d3d11sw {

class ShaderCodeGen
{
public:
    std::string Emit(const D3D11SW_ParsedShader& shader,
                     const std::string& runtimeHeaderPath) const;

private:
    std::string EmitVS(const D3D11SW_ParsedShader& shader) const;
    std::string EmitPS(const D3D11SW_ParsedShader& shader) const;
    std::string EmitCS(const D3D11SW_ParsedShader& shader) const;

    std::string EmitInstructions(const D3D11SW_ParsedShader& shader) const;
    std::string EmitInstr(const SM4Instruction& instr,
                          const D3D11SW_ParsedShader& shader) const;

    std::string EmitSrc(const SM4Operand& op) const;
    std::string EmitDstBase(const SM4Operand& op) const;
    std::string EmitWrite(const std::string& dstBase, Uint8 mask,
                          const std::string& srcExpr, Bool sat) const;

    static const Char* Comp(Int i);
    Uint8 FindSVPositionReg(const D3D11SW_ParsedShader& shader) const;
};

}
