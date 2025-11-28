#pragma once
#include "shaders/sm4.h"
#include "shaders/dxbc_parser.h"
#include <vector>

namespace d3d11sw {

D3D11SW_TODO(no state, should it be free function?);
class SM4Decoder
{
public:
    D3D11SW_API Bool Decode(const Uint32* tokens, Uint32 numDwords,
                std::vector<SM4Instruction>& out,
                Uint32& numTempsOut,
                Uint32  threadGroupSizeOut[3],
                std::vector<D3D11SW_TGSMDecl>& tgsmOut);

private:
    Uint32 ReadOperand(const Uint32* tokens, Uint32 offset, SM4Operand& out) const;
};

} 
