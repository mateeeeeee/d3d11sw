#pragma once
#include "shaders/sm4.h"
#include "shaders/dxbc_parser.h"
#include <vector>

namespace d3d11sw {

D3D11SW_API Bool SM4Decode(const Uint32* tokens, Uint32 numDwords,
            std::vector<SM4Instruction>& out,
            Uint32& numTempsOut,
            Uint32  threadGroupSizeOut[3],
            std::vector<D3D11SW_TGSMDecl>& tgsmOut);

}
